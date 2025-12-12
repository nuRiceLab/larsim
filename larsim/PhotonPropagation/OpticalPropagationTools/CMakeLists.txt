cet_make_library(LIBRARY_NAME OpticalPropagationTools INTERFACE
  SOURCE IOpticalPropagation.h
)

cet_write_plugin_builder(phot::OpticalPropagationTools art::tool Modules
  INSTALL_BUILDER)

include(phot::OpticalPropagationTools)

cet_build_plugin(OpticalPropPDFastSimPAR phot::OpticalPropagationTools
  LIBRARIES PRIVATE
  lardataobj::Simulation
)

install_headers()
install_fhicl()
install_source()
