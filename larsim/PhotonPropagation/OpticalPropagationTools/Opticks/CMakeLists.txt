
cet_write_plugin_builder(lar::OpticalPropagationTools::Opticks art::tool Modules INSTALL_BUILDER)

list(APPEND CMAKE_MODULE_PATH "$ENV{OPTICKS_HOME}/cmake/Modules")
set(OPTICKS_PREFIX "$ENV{OPTICKS_PREFIX}")
#include(OpticksBuildOptions)
find_dependency(PLog)
find_package(G4CX REQUIRED CONFIG)
find_package(U4 REQUIRED CONFIG)

include (lar::OpticalPropagationTools::Opticks)
add_compile_options(-gdwarf-4)
cet_build_plugin(Opticks
        lar::OpticalPropagationTools::Opticks
        IMPL_SOURCE
        OpticksInterface.cc
        MySensorIdentifier.cc
        OpticksHitHandler.cc
        LIBRARIES
        PUBLIC
        Opticks::G4CX
        Opticks::U4
        OpticalPropagationTools
        PRIVATE
        fhiclcpp::fhiclcpp
        messagefacility::MF_MessageLogger
        lardataobj::Simulation
)


install_headers()
install_fhicl()
install_source()
