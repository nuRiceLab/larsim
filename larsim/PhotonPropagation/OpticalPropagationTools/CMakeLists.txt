cet_make_library(LIBRARY_NAME OpticalPropagationTools INTERFACE
  SOURCE IOpticalPropagation.h
  LIBRARIES INTERFACE
  lardataobj::Simulation
)

cet_write_plugin_builder(phot::OpticalPropagationTools art::tool Modules
  INSTALL_BUILDER
  LIBRARIES CONDITIONAL
  lardataobj::Simulation
)

include(phot::OpticalPropagationTools)

cet_build_plugin(OpticalPropPDFastSimPAR phot::OpticalPropagationTools
  IMPL_SOURCE OpticalPropPDFastSimPAR.cc
  LIBRARIES PRIVATE
  lardataobj::Simulation
  fhiclcpp::fhiclcpp
  messagefacility::MF_MessageLogger
)

install_headers()
install_fhicl()
install_source()
