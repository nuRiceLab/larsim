////////////////////////////////////////////////////////////////////////
// Class:       OpticalPropagation
// Plugin Type: producer
// File:        OpticalPropagation_module.cc
//
// Description: This modules adds sim::OpDetBacktrackerRecord objects to an
// art::Event from sim::SimEnergyDeposit . The optical simulation is performed
// using one of three art tools:
// - PDFastSimPAR: Semi-analytical model
// - Opticks: Uses NVIDIA OptiX to perform photon propagation on GPU
// - Celeritas: Full Monte Carlo photon propagation on CPU or GPU
//
// Generated at Tue Dec  9 09:10:34 2025 by Stefano Tognini using cetskelgen
// from cetlib version 3.18.02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Utilities/make_tool.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/DelegatedParameter.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larsim/PhotonPropagation/OpticalPropagationTools/IOpticalPropagation.h"
#include "nurandom/RandomUtils/NuRandomService.h"
#include <memory>

namespace phot {
  class OpticalPropagation;
}

class phot::OpticalPropagation : public art::EDProducer {
public:
  //! FHiCL configuration parameter
  struct Config {
    fhicl::DelegatedParameter OpticalPropagationTool{
      fhicl::Name("OpticalPropagationTool"),
      fhicl::Comment("Tool configuration block with tool_type and parameters")};
  };

  //!@{
  //! \name Type aliases
  using Parameters = art::EDProducer::Table<Config>;
  //!@}

  //! Construct with fcl parameters
  explicit OpticalPropagation(Parameters const& config);

  //! Initialize optical simulation library
  void beginJob() override;

  //! Run full optical simulation
  void produce(art::Event& e) override;

  //! Tear down optical simulation library
  void endJob() override;

  //!@{
  //! Disable class copy and move semantics
  OpticalPropagation(OpticalPropagation const&) = delete;
  OpticalPropagation(OpticalPropagation&&) = delete;
  OpticalPropagation& operator=(OpticalPropagation const&) = delete;
  OpticalPropagation& operator=(OpticalPropagation&&) = delete;
  //!@}

private:
  // Propagation tool
  std::unique_ptr<IOpticalPropagation> fOpticalPropagationTool;
};

//---------------------------------------------------------------------------//
/*!
 * Construct with fhicl parameters.
 */
phot::OpticalPropagation::OpticalPropagation(Parameters const& config) : EDProducer{config}
{
  using IOP = phot::IOpticalPropagation;

  auto const& ps = config().OpticalPropagationTool.get<fhicl::ParameterSet>();
  fOpticalPropagationTool =
    std::unique_ptr<phot::IOpticalPropagation>(art::make_tool<phot::IOpticalPropagation>(ps));

  fOpticalPropagationTool->InitializeTools(
    art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine(
      createEngine(0, "HepJamesRandom", "photon"),
      "HepJamesRandom",
      "photon",
      config.get_PSet(),
      "SeedPhoton"),
    art::ServiceHandle<rndm::NuRandomService>()->registerAndSeedEngine(
      createEngine(0, "HepJamesRandom", "scinttime"),
      "HepJamesRandom",
      "scinttime",
      config.get_PSet(),
      "SeedScintTime"));
}

//---------------------------------------------------------------------------//
/*!
 * Initialize optical simulation based on the tool choice.
 */
void phot::OpticalPropagation::beginJob()
{
  // Allow for user-defined actions before event processing begins
  fOpticalPropagationTool->beginJob();
}

//---------------------------------------------------------------------------//
/*!
 * Generate and add \c sim::OpDetBacktrackerRecord objects to \c art::Event .
 */
void phot::OpticalPropagation::produce(art::Event& event)
{
  art::Handle<std::vector<sim::SimEnergyDeposit>> edepHandle;
  if (!event.getByLabel("IonAndScint", edepHandle)) {
    mf::LogError("OpticalPropagation") << "Missing IonAndScint label in art::Event";
    return;
  }

  // Execute optical simulation and add result to event
  auto result = fOpticalPropagationTool->executeEvent(*(edepHandle.product()));
  event.put(std::move(result));
}

//---------------------------------------------------------------------------//
/*!
 * Tear down simulations.
 */
void phot::OpticalPropagation::endJob()
{
  // Allow for user-defined actions after event processing ends
  fOpticalPropagationTool->endJob();
}

//---------------------------------------------------------------------------//
//! Register module in the framework
DEFINE_ART_MODULE(phot::OpticalPropagation)
