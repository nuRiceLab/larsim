//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.h
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
//! \brief Implementation of the PDFastSimPAR optical simulation tool
//---------------------------------------------------------------------------//
#pragma once

#include "IOpticalPropagation.h"

namespace phot {
  //-------------------------------------------------------------------------//
  /*!
   * Implementation of the \c PDFastSimPAR optical simulation tool.
   */
  class OpticalPropPDFastSimPAR : public IOpticalPropagation {
  public:
    // Construct with fcl parameters
    OpticalPropPDFastSimPAR();

    // Default destructor
    ~OpticalPropPDFastSimPAR() = default;

    // Initialize fast simulation
    void beginJob() override;

    // Execute simulation on a single art::Event
    UPVecBTR executeEvent(VecSED const& edeps) override;

    // Finalize module execution
    void endJob() override;
  };
} // namespace phot
