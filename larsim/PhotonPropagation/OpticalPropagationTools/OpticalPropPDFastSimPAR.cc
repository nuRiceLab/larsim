//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPAR.cc
//---------------------------------------------------------------------------//
#include "OpticalPropPDFastSimPAR.h"

#include "messagefacility/MessageLogger/MessageLogger.h"

//-------------------------------------------------------------------------//
/*!
 * Construct \c PDFastSimPAR tool with fcl parameters.
 */
phot::OpticalPropPDFastSimPAR::OpticalPropPDFastSimPAR(const fhicl::ParameterSet& p)
  : phot::IOpticalPropagation()
{
  mf::LogError("OpticalPropPDFastSimPAR") << "Not implemented";
}

//-------------------------------------------------------------------------//
/*!
 * Initalize fast simulation.
 */
void phot::OpticalPropPDFastSimPAR::beginJob()
{
  mf::LogError("OpticalPropPDFastSimPAR") << "Not implemented";
}

//-------------------------------------------------------------------------//
/*!
 * Apply fast simulation to a single \c art::Event .
 */
phot::OpticalPropPDFastSimPAR::UPVecBTR phot::OpticalPropPDFastSimPAR::executeEvent(
  VecSED const& edeps)
{
  mf::LogError("OpticalPropPDFastSimPAR") << "Not implemented";
  return {};
}

//-------------------------------------------------------------------------//
/*!
 * Finalize fast simulation.
 */
void phot::OpticalPropPDFastSimPAR::endJob()
{
  mf::LogError("OpticalPropPDFastSimPAR") << "Not implemented";
}
