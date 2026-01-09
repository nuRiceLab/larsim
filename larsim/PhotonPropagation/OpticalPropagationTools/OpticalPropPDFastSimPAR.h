//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.h
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
//! \brief Implementation of the PDFastSimPAR optical simulation tool
//---------------------------------------------------------------------------//
#pragma once

#include "IOpticalPropagation.h"

// LArSoft libraries
#include "larcore/CoreUtils/ServiceUtil.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/BoxBoundedGeo.h"
#include "larcorealg/Geometry/OpDetGeo.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_vectors.h"
#include "lardataobj/Simulation/OpDetBacktrackerRecord.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "larsim/IonizationScintillation/ISTPC.h"
#include "larsim/PhotonPropagation/OpticalPathTools/OpticalPath.h"
#include "larsim/PhotonPropagation/PropagationTimeModel.h"
#include "larsim/PhotonPropagation/ScintTimeTools/ScintTime.h"
#include "larsim/PhotonPropagation/SemiAnalyticalModel.h"

#include "nurandom/RandomUtils/NuRandomService.h"

// Art libraries
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Utilities/make_tool.h"
#include "canvas/Utilities/Exception.h"
#include "canvas/Utilities/InputTag.h"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Comment.h"
#include "fhiclcpp/types/DelegatedParameter.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/OptionalDelegatedParameter.h"
#include "fhiclcpp/types/Sequence.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Random numbers
#include "CLHEP/Random/RandPoissonQ.h"

#include "range/v3/view/enumerate.hpp"

#include <cmath>
#include <ctime>
#include <map>
#include <memory>
#include <vector>

namespace phot {
  class OpticalPropPDFastSimPAR;
}

//-------------------------------------------------------------------------//
/*!
 * Implementation of the \c PDFastSimPAR optical simulation tool.
 */
class phot::OpticalPropPDFastSimPAR : public phot::IOpticalPropagation {
public:
  struct Config {
    //!@{
    //! Type aliases
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    using DP = fhicl::DelegatedParameter;
    using ODP = fhicl::OptionalDelegatedParameter;
    //!@}

    DP OpticalPropagationTool{Name("OpticalPropagationTool"),
                              Comment("Tool describing the optical propagation mechanism")};
    const std::vector<int> default_TPCs{};
    fhicl::Atom<art::InputTag> SimulationLabel{Name("SimulationLabel"),
                                               Comment("SimEnergyDeposit label.")};
    fhicl::Atom<bool> DoFastComponent{Name("DoFastComponent"),
                                      Comment("Simulate slow scintillation light, default true"),
                                      true};
    fhicl::Atom<bool> DoSlowComponent{Name("DoSlowComponent"),
                                      Comment("Simulate slow scintillation light")};
    fhicl::Atom<bool> DoReflectedLight{Name("DoReflectedLight"),
                                       Comment("Simulate reflected visible light")};
    fhicl::Atom<bool> IncludeAnodeReflections{Name("IncludeAnodeReflections"),
                                              Comment("Simulate anode reflections, default false"),
                                              false};
    fhicl::Atom<bool> IncludePropTime{Name("IncludePropTime"),
                                      Comment("Simulate light propagation time")};
    fhicl::Atom<bool> GeoPropTimeOnly{
      Name("GeoPropTimeOnly"),
      Comment("Simulate light propagation time geometric approximation, default false"),
      false};
    fhicl::Atom<bool> UseLitePhotons{
      Name("UseLitePhotons"),
      Comment("Store SimPhotonsLite/OpDetBTRs instead of SimPhotons")};
    fhicl::Atom<bool> OpaqueCathode{Name("OpaqueCathode"),
                                    Comment("Photons cannot cross the cathode")};
    fhicl::Atom<bool> OnlyActiveVolume{
      Name("OnlyActiveVolume"),
      Comment("PAR fast sim usually only for active volume, default true"),
      true};
    fhicl::Sequence<int> RestrictedTPCs{Name("RestrictedTPCs"),
                                        Comment("Simulate for EDeps only in these TPCs.\nDefault "
                                                "is empty which means simulate in all TPCs"),
                                        default_TPCs};
    fhicl::Atom<bool> OnlyOneCryostat{Name("OnlyOneCryostat"),
                                      Comment("Set to true if light is only supported in C:1")};
    DP ScintTimeTool{Name("ScintTimeTool"),
                     Comment("Tool describing scintillation time structure")};
    DP OpticalPathTool{
      Name("OpticalPathTool"),
      Comment(
        "Tool to determine visibility of optical detectors from scintillation emission points")};
    fhicl::Atom<bool> UseXeAbsorption{
      Name("UseXeAbsorption"),
      Comment("Use Xe absorption length instead of Ar, default false"),
      false};
    ODP VUVTiming{Name("VUVTiming"), Comment("Configuration for UV timing parameterization")};
    ODP VISTiming{Name("VISTiming"), Comment("Configuration for visible timing parameterization")};
    DP VUVHits{Name("VUVHits"), Comment("Configuration for UV visibility parameterization")};
    ODP VISHits{Name("VISHits"), Comment("Configuration for visibile visibility parameterization")};
    fhicl::Atom<bool> Verbose{Name("Verbose"), Comment("Print verbose information"), false};
  };

  //!@{
  //! Type aliases
  using Parameters = art::EDProducer::Table<Config>;
  //!@}

  // Construct with fcl parameters
  OpticalPropPDFastSimPAR(const Parameters& config,
                          CLHEP::HepRandomEngine& poisson,
                          CLHEP::HepRandomEngine& scint_time);

  // Default destructor
  ~OpticalPropPDFastSimPAR() = default;

  // Initialize fast simulation
  void beginJob() override;

  // Execute simulation on a single art::Event
  UPVecBTR executeEvent(VecSED const& edeps) override;

  // Finalize execution
  void endJob() override;

private:
  void detectedNumPhotons(std::vector<int>& DetectedNumPhotons,
                          const std::vector<double>& OpDetVisibilities,
                          const int NumPhotons) const;

  void AddOpDetBTR(std::vector<sim::OpDetBacktrackerRecord>& opbtr,
                   std::vector<int>& ChannelMap,
                   const sim::OpDetBacktrackerRecord& btr) const;
  void SimpleAddOpDetBTR(
    // std::vector<sim::OpDetBacktrackerRecord>& opbtr,
    std::map<int, sim::OBTRHelper>& opbtr,
    std::vector<int>& ChannelMap,
    size_t channel,
    int trackID,
    int time,
    double pos[3],
    double edeposit,
    int num_photons = 1);

  std::vector<geo::Point_t> opDetCenters() const;

private:
  // Store FHiCL parameters
  Parameters fConfig;

  // semi-analytical model
  std::unique_ptr<SemiAnalyticalModel> fVisibilityModel;

  // propagation time model
  std::unique_ptr<PropagationTimeModel> fPropTimeModel;

  // random numbers
  std::unique_ptr<CLHEP::RandPoissonQ> fRandPoissPhot;

  // Tool to retrieve timinig of scintillation
  std::unique_ptr<ScintTime> fScintTime;

  // Tool to to determine visibility of optical detectors from scintillation emission points
  std::shared_ptr<OpticalPath> fOpticalPath;

  // geometry properties
  geo::GeometryCore const& fGeom;
  larg4::ISTPC fISTPC;
  const size_t fNOpChannels;
  const std::vector<geo::BoxBoundedGeo> fActiveVolumes;
  const int fNTPC;
  const std::vector<int> fRestrictedTPCs;
  double fDriftDistance;
  const std::vector<geo::Point_t> fOpDetCenter;

  // Module behavior
  art::InputTag fSimTag;
  bool fDoFastComponent;
  bool fDoSlowComponent;
  bool fDoReflectedLight;
  bool fIncludeAnodeReflections;
  bool fIncludePropTime;
  bool fGeoPropTimeOnly;
  bool fUseLitePhotons;
  bool fOpaqueCathode;
  bool fOnlyActiveVolume;
  bool fOnlyOneCryostat;
  bool fUseXeAbsorption;
  fhicl::ParameterSet fVUVHitsParams;
  fhicl::ParameterSet fVUVTimingParams;
  fhicl::ParameterSet fVISHitsParams;
  fhicl::ParameterSet fVISTimingParams;
};
