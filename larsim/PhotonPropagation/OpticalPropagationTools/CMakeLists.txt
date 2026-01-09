cet_make_library(LIBRARY_NAME OpticalPropagationTools INTERFACE
  SOURCE IOpticalPropagation.h
  LIBRARIES INTERFACE
  lardataobj::Simulation
)

cet_write_plugin_builder(phot::OpticalPropagationTools art::tool Modules
  INSTALL_BUILDER
  LIBRARIES CONDITIONAL
  lardataobj::Simulation
  larcore::ServiceUtil
)

include(phot::OpticalPropagationTools)

cet_build_plugin(OpticalPropPDFastSimPAR phot::OpticalPropagationTools
  IMPL_SOURCE OpticalPropPDFastSimPAR.cc
  LIBRARIES PRIVATE
  larsim::PhotonPropagation
  larsim::IonizationScintillation
  larcore::ServiceUtil
  larcorealg::Geometry
  larcoreobj::geo_vectors
  lardataobj::Simulation
  nurandom::RandomUtils_NuRandomService_service
  art::Framework_Principal
  art_plugin_support::toolMaker
  canvas::canvas
  messagefacility::MF_MessageLogger
  fhiclcpp::types
  fhiclcpp::fhiclcpp
  cetlib_except::cetlib_except
  CLHEP::Random
)

install_headers()
install_fhicl()
install_source()
