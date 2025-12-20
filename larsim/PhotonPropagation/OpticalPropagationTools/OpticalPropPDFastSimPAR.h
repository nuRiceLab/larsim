//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.h
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
//! \brief Implementation of the PDFastSimPAR optical simulation tool
//---------------------------------------------------------------------------//
#pragma once

#include "IOpticalPropagation.h"

#include "fhiclcpp/ParameterSet.h"
#include "lardataobj/Simulation/OpDetBacktrackerRecord.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

namespace phot {
  class OpticalPropPDFastSimPAR;
}

//-------------------------------------------------------------------------//
/*!
 * Implementation of the \c PDFastSimPAR optical simulation tool.
 */
class phot::OpticalPropPDFastSimPAR : public phot::IOpticalPropagation {
public:
  // Construct with fcl parameters
  OpticalPropPDFastSimPAR(const fhicl::ParameterSet& p);

  // Default destructor
  ~OpticalPropPDFastSimPAR() = default;

  // Initialize fast simulation
  void beginJob() override;

  // Execute simulation on a single art::Event
  UPVecBTR executeEvent(VecSED const& edeps) override;

  // Finalize execution
  void endJob() override;
};
