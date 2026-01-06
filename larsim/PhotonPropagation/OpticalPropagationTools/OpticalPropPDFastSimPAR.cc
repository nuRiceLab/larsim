//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.cc
//---------------------------------------------------------------------------//
#include "OpticalPropPDFastSimPAR.h"

//---------------------------------------------------------------------------//
/*!
 * Construct \c PDFastSimPAR tool with fcl parameters.
 */
phot::OpticalPropPDFastSimPAR::OpticalPropPDFastSimPAR(const Parameters& config)
  : phot::IOpticalPropagation()
  , fConfig(config)
  , fScintTime{art::make_tool<phot::ScintTime>(config().ScintTimeTool.get<fhicl::ParameterSet>())}
  , fOpticalPath{std::shared_ptr<phot::OpticalPath>(
      art::make_tool<phot::OpticalPath>(config().OpticalPathTool.get<fhicl::ParameterSet>()))}
  , fGeom(*(lar::providerFrom<geo::Geometry>()))
  , fISTPC{fGeom}
  , fNOpChannels(fGeom.NOpDets())
  , fActiveVolumes(fISTPC.extractActiveLArVolume(fGeom))
  , fNTPC(fGeom.NTPC())
  , fRestrictedTPCs(config().RestrictedTPCs())
  , fOpDetCenter(opDetCenters())
  , fSimTag(config().SimulationLabel())
  , fDoFastComponent(config().DoFastComponent())
  , fDoSlowComponent(config().DoSlowComponent())
  , fDoReflectedLight(config().DoReflectedLight())
  , fIncludeAnodeReflections(config().IncludeAnodeReflections())
  , fIncludePropTime(config().IncludePropTime())
  , fGeoPropTimeOnly(config().GeoPropTimeOnly())
  , fUseLitePhotons(config().UseLitePhotons())
  , fOpaqueCathode(config().OpaqueCathode())
  , fOnlyActiveVolume(config().OnlyActiveVolume())
  , fOnlyOneCryostat(config().OnlyOneCryostat())
  , fUseXeAbsorption(config().UseXeAbsorption())
{
  mf::LogInfo("OpticalPropPDFastSimPAR") << "Constructing tool" << std::endl;

  // Validate configuration options
  if (fIncludePropTime &&
      !config().VUVTiming.get_if_present<fhicl::ParameterSet>(VUVTimingParams)) {
    throw art::Exception(art::errors::Configuration)
      << "Propagation time simulation requested, but VUVTiming not specified."
      << "\n";
  }
  if ((fDoReflectedLight || fIncludeAnodeReflections) &&
      !config().VISHits.get_if_present<fhicl::ParameterSet>(VISHitsParams)) {
    throw art::Exception(art::errors::Configuration)
      << "Reflected light or anode reflections simulation requested, but VisHits not specified."
      << "\n";
  }
  if (fDoReflectedLight && fIncludePropTime &&
      !config().VISTiming.get_if_present<fhicl::ParameterSet>(VISTimingParams)) {
    throw art::Exception(art::errors::Configuration)
      << "Reflected light propagation time simulation requested, but VISTiming not specified."
      << "\n";
  }
  if (fGeom.Ncryostats() > 1U) {
    if (fOnlyOneCryostat) {
      mf::LogWarning("PDFastSimPAR")
        << std::string(80, '=') << "\nA detector with " << fGeom.Ncryostats()
        << " cryostats is configured"
        << " , and semi-analytic model is requested for scintillation photon propagation."
        << " THIS CONFIGURATION IS NOT SUPPORTED and it is open to bugs"
        << " (e.g. scintillation may be detected only in cryostat #0)."
        << "\nThis would be normally a fatal error, but it has been forcibly overridden."
        << "\n"
        << std::string(80, '=');
    }
    else {
      throw art::Exception(art::errors::Configuration)
        << "Photon propagation via semi-analytic model is not supported yet"
        << " on detectors with more than one cryostat.";
    }
  }

  mf::LogDebug("OpticalPropPDFastSimPAR") << "Only generating edeps in the following TPCs:";
  for (const auto& tpc : fRestrictedTPCs) {
    mf::LogDebug("OpticalPropPDFastSimPAR") << tpc;
  }

  mf::LogInfo("OpticalPropPDFastSimPAR")
    << "PDFastSimPAR: active volume boundaries from " << fActiveVolumes.size() << " volumes:";
  for (auto const& [iCryo, box] : ::ranges::views::enumerate(fActiveVolumes)) {
    mf::LogInfo("OpticalPropPDFastSimPAR")
      << "\n - C:" << iCryo << ": " << box.Min() << " -- " << box.Max() << " cm";
  }

  // determine drift distance
  fDriftDistance = fGeom.TPC().DriftDistance();
  // for multiple TPCs, use second TPC to skip small volume at edges of detector (DUNE)
  if (fNTPC > 1 && fDriftDistance < 50)
    fDriftDistance = fGeom.TPC(geo::TPCID{0, 1}).DriftDistance();

  mf::LogInfo("OpticalPropPDFastSimPAR")
    << "Initialized.\n"
    << "Simulate using semi-analytic model for number of hits." << std::endl;
}

//---------------------------------------------------------------------------//
/*!
 * Transfer ownership of the random number generator engines from the
 * EDProducer to this tool.
 */
template <class PhotonEngine, class PoissonEngine, class ScintEngine>
void TransferRngs(PhotonEngine& photon_engine,
                  std::unique_ptr<PoissonEngine> poisson,
                  ScintEngine& scint_time);
{
  fPhotonEngine = photon_engine;
  fRandPoissPhot = poisson;
  fScintTimeEngine = scint_time;
}

//---------------------------------------------------------------------------//
/*!
 * Initialize all objects that *cannot* be created at construction time.
 *
 * These are objects that dependent on the RNG engines created by the Producer
 * and had their ownership transferred to this tool via \c ::InitializeRng .
 */
void phot::OpticalPropPDFastSimPAR::beginJob()
{
  mf::LogTrace("OpticalPropPDFastSimPAR") << "beginJob()";

  // Parameterized Simulation
  fhicl::ParameterSet VUVHitsParams = config().VUVHits.get<fhicl::ParameterSet>();
  fhicl::ParameterSet VUVTimingParams;
  fhicl::ParameterSet VISHitsParams;
  fhicl::ParameterSet VISTimingParams;

  // Initialise the Scintillation Time RNG engine
  fScintTime->initRand(fScintTimeEngine);

  // Construct semi-analytical photo-detector visibility model
  fVisibilityModel = std::make_unique<SemiAnalyticalModel>(VUVHitsParams,
                                                           VISHitsParams,
                                                           fOpticalPath,
                                                           fDoReflectedLight,
                                                           fIncludeAnodeReflections,
                                                           fUseXeAbsorption);

  if (fIncludePropTime) {
    // Construt propagation time model
    fPropTimeModel = std::make_unique<PropagationTimeModel>(
      VUVTimingParams, VISTimingParams, fScintTimeEngine, fDoReflectedLight, fGeoPropTimeOnly);
  }
}

//---------------------------------------------------------------------------//
/*!
 * Apply fast simulation to a single \c art::Event .
 *
 * \todo Only one object (a \c vector<OpDetBacktrackerRecord> ) must be
 *  returned. Current copy/paste also generates a \c vector<SimPhotonsLite> and
 *  respective reflected results, which are not used and must be cleaned.
 */
phot::OpticalPropPDFastSimPAR::UPVecBTR phot::OpticalPropPDFastSimPAR::executeEvent(
  VecSED const& edeps)
{
  mf::LogTrace("OpticalPropPDFastSimPAR") << "Using IOpticalInterface tool";

  std::vector<int> PDChannelToSOCMapDirect(fNOpChannels, -1);  // Where each OpChan is.
  std::vector<int> PDChannelToSOCMapReflect(fNOpChannels, -1); // Where each OpChan is.

  // SimPhotonsLite
  auto phlit = std::make_unique<std::vector<sim::SimPhotonsLite>>();
  auto opbtr = std::make_unique<std::vector<sim::OpDetBacktrackerRecord>>();
  auto phlit_ref = std::make_unique<std::vector<sim::SimPhotonsLite>>();
  auto opbtr_ref = std::make_unique<std::vector<sim::OpDetBacktrackerRecord>>();

  //Helpers holding maps
  std::map<int, sim::OBTRHelper> opbtr_helper, opbtr_helper_ref;

  auto& dir_phlitcol(*phlit);
  auto& ref_phlitcol(*phlit_ref);
  // SimPhotons
  auto phot = std::make_unique<std::vector<sim::SimPhotons>>();
  auto phot_ref = std::make_unique<std::vector<sim::SimPhotons>>();
  auto& dir_photcol(*phot);
  auto& ref_photcol(*phot_ref);
  if (fUseLitePhotons) {
    dir_phlitcol.resize(fNOpChannels);
    ref_phlitcol.resize(fNOpChannels);
    for (unsigned int i = 0; i < fNOpChannels; i++) {
      dir_phlitcol[i].OpChannel = i;
      ref_phlitcol[i].OpChannel = i;
    }
  }
  else { // SimPhotons
    dir_photcol.resize(fNOpChannels);
    ref_photcol.resize(fNOpChannels);
    for (unsigned int i = 0; i < fNOpChannels; i++) {
      dir_photcol[i].fOpChannel = i;
      ref_photcol[i].fOpChannel = i;
    }
  }

  int num_points = 0;
  int num_fastph = 0;
  int num_slowph = 0;
  int num_fastdp = 0;
  int num_slowdp = 0;

  mf::LogTrace("OpticalPropPDFastSimPAR")
    << "Creating SimPhotonsLite/SimPhotons from " << edeps.size() << " energy deposits\n";

  std::map<int, size_t> skipped_edeps;

  for (auto const& edepi : edeps) {

    if (!(num_points % 1000)) {
      mf::LogTrace("OpticalPropPDFastSimPAR")
        << "SimEnergyDeposit: " << num_points << " " << edepi.TrackID() << " " << edepi.Energy()
        << "\nStart: " << edepi.Start() << "\nEnd: " << edepi.End()
        << "\nNF: " << edepi.NumFPhotons() << "\nNS: " << edepi.NumSPhotons()
        << "\nSYR: " << edepi.ScintYieldRatio() << "\n";
    }
    num_points++;

    int nphot_fast = edepi.NumFPhotons();
    int nphot_slow = edepi.NumSPhotons();

    num_fastph += nphot_fast;
    num_slowph += nphot_slow;

    if (!((nphot_fast > 0 && fDoFastComponent) || (nphot_slow > 0 && fDoSlowComponent))) continue;

    int trackID = edepi.TrackID();
    int nphot = edepi.NumPhotons();
    double edeposit = edepi.Energy() / nphot;
    double pos[3] = {edepi.MidPointX(), edepi.MidPointY(), edepi.MidPointZ()};
    geo::Point_t const ScintPoint = {pos[0], pos[1], pos[2]};

    //Check if we're within the active volume and sim or skip accordingly
    if (fOnlyActiveVolume && !fISTPC.isScintInActiveVolume(ScintPoint)) continue;

    //If we're in the active volume (and we want to simulate within it),
    //check which TPC we're in and whether we want to simulate within that TPC
    int scint_point_TPC = fGeom.PositionToTPCID(ScintPoint).TPC;
    auto in_valid_TPC =
      ((fRestrictedTPCs.size() == 0) || //If no TPCs specified, simulate in all
       std::find(fRestrictedTPCs.begin(), fRestrictedTPCs.end(), scint_point_TPC) !=
         fRestrictedTPCs.end());
    if (fOnlyActiveVolume && !(in_valid_TPC)) {
      skipped_edeps[scint_point_TPC]++;

      continue;
    }

    // direct light
    std::vector<int> DetectedNumFast(fNOpChannels);
    std::vector<int> DetectedNumSlow(fNOpChannels);

    std::vector<double> OpDetVisibilities;
    fVisibilityModel->detectedDirectVisibilities(OpDetVisibilities, ScintPoint);
    detectedNumPhotons(DetectedNumFast, OpDetVisibilities, nphot_fast);
    detectedNumPhotons(DetectedNumSlow, OpDetVisibilities, nphot_slow);

    if (fIncludeAnodeReflections) {
      std::vector<int> AnodeDetectedNumFast(fNOpChannels);
      std::vector<int> AnodeDetectedNumSlow(fNOpChannels);

      std::vector<double> OpDetVisibilitiesAnode;
      fVisibilityModel->detectedReflectedVisibilities(OpDetVisibilitiesAnode, ScintPoint, true);
      detectedNumPhotons(AnodeDetectedNumFast, OpDetVisibilitiesAnode, nphot_fast);
      detectedNumPhotons(AnodeDetectedNumSlow, OpDetVisibilitiesAnode, nphot_slow);

      // add to existing count
      for (size_t i = 0; i < AnodeDetectedNumFast.size(); ++i) {
        DetectedNumFast[i] += AnodeDetectedNumFast[i];
      }
      for (size_t i = 0; i < AnodeDetectedNumSlow.size(); ++i) {
        DetectedNumSlow[i] += AnodeDetectedNumSlow[i];
      }
    }

    // reflected light, if enabled
    std::vector<int> ReflDetectedNumFast(fNOpChannels);
    std::vector<int> ReflDetectedNumSlow(fNOpChannels);
    if (fDoReflectedLight) {
      std::vector<double> OpDetVisibilitiesRefl;
      fVisibilityModel->detectedReflectedVisibilities(OpDetVisibilitiesRefl, ScintPoint, false);
      detectedNumPhotons(ReflDetectedNumFast, OpDetVisibilitiesRefl, nphot_fast);
      detectedNumPhotons(ReflDetectedNumSlow, OpDetVisibilitiesRefl, nphot_slow);
    }

    // loop through direct photons then reflected photons cases
    size_t DoReflected = (fDoReflectedLight) ? 1 : 0;
    for (size_t Reflected = 0; Reflected <= DoReflected; ++Reflected) {
      for (size_t channel = 0; channel < fNOpChannels; channel++) {

        if (fOpaqueCathode && !fOpticalPath->isOpDetVisible(ScintPoint, fOpDetCenter[channel]))
          continue;

        int ndetected_fast = DetectedNumFast[channel];
        int ndetected_slow = DetectedNumSlow[channel];
        if (Reflected) {
          ndetected_fast = ReflDetectedNumFast[channel];
          ndetected_slow = ReflDetectedNumSlow[channel];
        }
        if (!((ndetected_fast > 0 && fDoFastComponent) || (ndetected_slow > 0 && fDoSlowComponent)))
          continue;

        // calculate propagation time, does not matter whether fast or slow photon
        std::vector<double> transport_time;
        if (fIncludePropTime) {
          transport_time.resize(ndetected_fast + ndetected_slow);
          fPropTimeModel->propagationTime(transport_time, ScintPoint, channel, Reflected);
        }

        // SimPhotonsLite case
        if (fUseLitePhotons) {
          if (ndetected_fast > 0 && fDoFastComponent) {
            int n = ndetected_fast;
            num_fastdp += n;
            for (int i = 0; i < n; ++i) {
              // calculates the time at which the photon was produced
              double dtime = edepi.StartT() + fScintTime->fastScintTime();
              if (fIncludePropTime) dtime += transport_time[i];

              int time = static_cast<int>(std::round(dtime));
              if (Reflected) {
                ++ref_phlitcol[channel].DetectedPhotons[time];
                SimpleAddOpDetBTR(
                  // *opbtr_ref, PDChannelToSOCMapReflect, channel, trackID, time, pos, edeposit, 1);
                  opbtr_helper_ref,
                  PDChannelToSOCMapReflect,
                  channel,
                  trackID,
                  time,
                  pos,
                  edeposit,
                  1);
              }
              else {
                ++dir_phlitcol[channel].DetectedPhotons[time];
                SimpleAddOpDetBTR(
                  // *opbtr, PDChannelToSOCMapDirect, channel, trackID, time, pos, edeposit, 1);
                  opbtr_helper,
                  PDChannelToSOCMapDirect,
                  channel,
                  trackID,
                  time,
                  pos,
                  edeposit,
                  1);
              }
            }
          }
          if (ndetected_slow > 0 && fDoSlowComponent) {
            int n = ndetected_slow;
            num_slowdp += n;
            for (int i = 0; i < n; ++i) {
              // calculates the time at which the photon was produced
              double dtime = edepi.StartT() + fScintTime->slowScintTime();
              if (fIncludePropTime) dtime += transport_time[ndetected_fast + i];
              int time = static_cast<int>(std::round(dtime));
              if (Reflected) {
                ++ref_phlitcol[channel].DetectedPhotons[time];
                SimpleAddOpDetBTR(
                  // *opbtr_ref, PDChannelToSOCMapReflect, channel, trackID, time, pos, edeposit, 1);
                  opbtr_helper_ref,
                  PDChannelToSOCMapReflect,
                  channel,
                  trackID,
                  time,
                  pos,
                  edeposit,
                  1);
              }
              else {
                ++dir_phlitcol[channel].DetectedPhotons[time];
                SimpleAddOpDetBTR(
                  // *opbtr, PDChannelToSOCMapDirect, channel, trackID, time, pos, edeposit, 1);
                  opbtr_helper,
                  PDChannelToSOCMapDirect,
                  channel,
                  trackID,
                  time,
                  pos,
                  edeposit,
                  1);
              }
            }
          }
        }
        // SimPhotons case
        else {
          sim::OnePhoton photon;
          photon.SetInSD = false;
          photon.InitialPosition = edepi.End();
          photon.MotherTrackID = edepi.TrackID();
          if (Reflected)
            photon.Energy = 2.9 * CLHEP::eV; // 430 nm
          else
            photon.Energy = 9.7 * CLHEP::eV; // 128 nm
          // TODO: un-hardcode and add another energy for Xe scintillation
          if (ndetected_fast > 0 && fDoFastComponent) {
            int n = ndetected_fast;
            num_fastdp += n;
            for (int i = 0; i < n; ++i) {
              // calculates the time at which the photon was produced
              double dtime = edepi.StartT() + fScintTime->fastScintTime();
              if (fIncludePropTime) dtime += transport_time[i];
              photon.Time = dtime;
              if (Reflected)
                ref_photcol[channel].insert(ref_photcol[channel].end(), 1, photon);
              else
                dir_photcol[channel].insert(dir_photcol[channel].end(), 1, photon);
            }
          }
          if (ndetected_slow > 0 && fDoSlowComponent) {
            int n = ndetected_slow;
            num_slowdp += n;
            for (int i = 0; i < n; ++i) {
              double dtime = edepi.StartT() + fScintTime->slowScintTime();
              if (fIncludePropTime) dtime += transport_time[ndetected_fast + i];
              photon.Time = dtime;
              if (Reflected)
                ref_photcol[channel].insert(ref_photcol[channel].end(), 1, photon);
              else
                dir_photcol[channel].insert(dir_photcol[channel].end(), 1, photon);
            }
          }
        }
      }
    }
  }

  mf::LogTrace("OpticalPropPDFastSimPAR")
    << "Total points: " << num_points << ", total fast photons: " << num_fastph
    << ", total slow photons: " << num_slowph << "\ndetected fast photons: " << num_fastdp
    << ", detected slow photons: " << num_slowdp;

  mf::LogDebug("OpticalPropPDFastSimPAR") << "Number of entries in opbtrs";
  for (auto& iopbtr : *opbtr) {
    mf::LogDebug("OpticalPropPDFastSimPAR")
      << "OpDet: " << iopbtr.OpDetNum() << " " << iopbtr.timePDclockSDPsMap().size();
  }
  mf::LogDebug("OpticalPropPDFastSimPAR") << "Number of entries in opbtrs refelected";
  for (auto& iopbtr : *opbtr_ref) {
    mf::LogDebug("OpticalPropPDFastSimPAR")
      << "OpDet: " << iopbtr.OpDetNum() << " " << iopbtr.timePDclockSDPsMap().size();
  }

  for (const auto& [tpc, nskipped] : skipped_edeps) {
    mf::LogDebug("OpticalPropPDFastSimPAR") << "Skipped " << nskipped << " edeps in TPC " << tpc;
  }

  if (fUseLitePhotons) {

    for (auto& iopbtr : opbtr_helper) {
      opbtr->emplace_back(iopbtr.second);
    }
  }
  return opbtr;
}

//---------------------------------------------------------------------------//
/*!
 * Finalize fast simulation.
 */
void phot::OpticalPropPDFastSimPAR::endJob()
{
  mf::LogTrace("OpticalPropPDFastSimPAR") << "beginJob() called but not required";
}

//---------------------------------------------------------------------------//
// PRIVATE
//---------------------------------------------------------------------------//
/*!
* \todo Add documentation.
*/
void phot::OpticalPropPDFastSimPAR::AddOpDetBTR(std::vector<sim::OpDetBacktrackerRecord>& opbtr,
                                                std::vector<int>& ChannelMap,
                                                const sim::OpDetBacktrackerRecord& btr) const
{
  int iChan = btr.OpDetNum();
  if (ChannelMap[iChan] < 0) {
    ChannelMap[iChan] = opbtr.size();
    opbtr.emplace_back(std::move(btr));
  }
  else {
    size_t idtest = ChannelMap[iChan];
    auto const& timePDclockSDPsMap = btr.timePDclockSDPsMap();
    for (auto const& timePDclockSDP : timePDclockSDPsMap) {
      for (auto const& sdp : timePDclockSDP.second) {
        double xyz[3] = {sdp.x, sdp.y, sdp.z};
        opbtr.at(idtest).AddScintillationPhotons(
          sdp.trackID, timePDclockSDP.first, sdp.numPhotons, xyz, sdp.energy);
      }
    }
  }
}

//---------------------------------------------------------------------------//
/*!
* \todo Add documentation.
*/
void phot::OpticalPropPDFastSimPAR::
  SimpleAddOpDetBTR( //std::vector<sim::OpDetBacktrackerRecord>& opbtr,
    std::map<int, sim::OBTRHelper>& opbtr,
    std::vector<int>& ChannelMap,
    size_t channel,
    int trackID,
    int time,
    double pos[3],
    double edeposit,
    int num_photons)
{
  if (ChannelMap[channel] < 0) {
    ChannelMap[channel] = opbtr.size();
    // opbtr.push_back(sim::OpDetBacktrackerRecord(channel));
    opbtr.emplace(channel, channel);
  }
  // size_t idtest = ChannelMap[channel];
  // opbtr.at(idtest).AddScintillationPhotonsToMap(trackID, time, num_photons, pos, edeposit);
  opbtr.at(channel).AddScintillationPhotonsToMap(trackID, time, num_photons, pos, edeposit);
}

//---------------------------------------------------------------------------//
/*!
* Calculates number of photons detected given visibility and emitted number of
* photons
*/
void phot::OpticalPropPDFastSimPAR::detectedNumPhotons(std::vector<int>& DetectedNumPhotons,
                                                       const std::vector<double>& OpDetVisibilities,
                                                       const int NumPhotons) const
{
  for (size_t i = 0; i < OpDetVisibilities.size(); ++i) {
    DetectedNumPhotons[i] = fRandPoissPhot->fire(OpDetVisibilities[i] * NumPhotons);
  }
}

//---------------------------------------------------------------------------//
/*!
* \todo Add documentation.
*/
std::vector<geo::Point_t> phot::OpticalPropPDFastSimPAR::opDetCenters() const
{
  std::vector<geo::Point_t> opDetCenter;
  for (size_t const i : ::ranges::views::ints(size_t(0), fNOpChannels)) {
    geo::OpDetGeo const& opDet = fGeom.OpDetGeoFromOpDet(i);
    opDetCenter.push_back(opDet.GetCenter());
  }
  return opDetCenter;
}
