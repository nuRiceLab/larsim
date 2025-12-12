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
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "OpticalPropagationTools/IOpticalPropagation.h"

#include <memory>

namespace phot {
  class OpticalPropagation;
}

class phot::OpticalPropagation : public art::EDProducer {
public:
  explicit OpticalPropagation(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OpticalPropagation(OpticalPropagation const&) = delete;
  OpticalPropagation(OpticalPropagation&&) = delete;
  OpticalPropagation& operator=(OpticalPropagation const&) = delete;
  OpticalPropagation& operator=(OpticalPropagation&&) = delete;

  // Initialize optical simulation library
  void beginJob() override;

  // Run full optical simulation
  void produce(art::Event& e) override;

  // Tear down optical simulation library
  void endJob() override;

private:
  std::unique_ptr<IOpticalPropagation> fOpticalPropagationTool;
};

//......................................................................
/*!
 * Construct with fhicl parameters: Initialize optical simulation tool.
 */
phot::OpticalPropagation::OpticalPropagation(fhicl::ParameterSet const& p) : EDProducer{p}
{
  // Initialize optical simulation library tool
  fhicl::ParameterSet tool = p.get<fhicl::ParameterSet>("OpticalPropagationTool");
  fOpticalPropagationTool = art::make_tool<IOpticalPropagation>(tool);
}

//......................................................................
/*!
 * Initialize optical simulation library object based on tool choice.
 */
void phot::OpticalPropagation::beginJob()
{
  // Allow for user-defined actions before event processing begins
  fOpticalPropagationTool->beginJob();
}

//......................................................................
/*!
 * Produce \c sim::OpDetBacktrackerRecord objects and add to \c art::Event.
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

//......................................................................
/*!
 * Tear down simulation library.
 */
void phot::OpticalPropagation::endJob()
{
  // Allow for user-defined actions after event processing ends
  fOpticalPropagationTool->endJob();
}

//......................................................................
//! Register module in the framework
DEFINE_ART_MODULE(phot::OpticalPropagation)
