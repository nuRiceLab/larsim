//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.h
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
//! \brief Implementation of the PDFastSimPAR optical simulation tool
//---------------------------------------------------------------------------//
#pragma once

#include "IOpticalPropagation.h"
#include "OpticalPropPDFastSimPARConfig.h"

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
#include "fhiclcpp/types/ConfigurationTable.h"
#include "fhiclcpp/types/DelegatedParameter.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/OptionalDelegatedParameter.h"
#include "fhiclcpp/types/OptionalTable.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/TableFragment.h"
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
  using Parameters = phot::OpticalPropPDFastSimParameters;

  // Construct with fcl parameters
  OpticalPropPDFastSimPAR(const Parameters& config);

  // Transfer RNG engines to this tool once the EDProducer constructs them
  template <class PhotonEngine, class PoissonEngine, class ScintEngine>
  void TransferRngs(PhotonEngine& photon_engine,
                    std::unique_ptr<PoissonEngine> poisson,
                    ScintEngine& scint_time);
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
  CLHEP::HepRandomEngine* fPhotonEngine;
  std::unique_ptr<CLHEP::RandPoissonQ> fRandPoissPhot;
  CLHEP::HepRandomEngine* fScintTimeEngine;

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
  const art::InputTag fSimTag;
  const bool fDoFastComponent;
  const bool fDoSlowComponent;
  const bool fDoReflectedLight;
  const bool fIncludeAnodeReflections;
  const bool fIncludePropTime;
  const bool fGeoPropTimeOnly;
  const bool fUseLitePhotons;
  const bool fOpaqueCathode;
  const bool fOnlyActiveVolume;
  const bool fOnlyOneCryostat;
  const bool fUseXeAbsorption;
};
