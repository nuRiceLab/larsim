//---------------------------------------------------------------------------//
//! \file OpticalPropPDFastSimPARConfig.h
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
//! \brief Configuration table for the PDFastSimPAR tool
//---------------------------------------------------------------------------//
/*!
* TODO: For now, the Config is defined here. Not sure yet where it will live (in
* the module directly (bad) or in the tool itself.
*
* Ideally we would cascade different configurations based on the tool type from
* the fcl. With each tool having its own fcl with its own set of configuration
* parameters. Not sure how/if this is possible.
*/
//---------------------------------------------------------------------------//
#pragma once

#include "art/Framework/Core/EDProducer.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Comment.h"
#include "fhiclcpp/types/DelegatedParameter.h"
#include "fhiclcpp/types/Name.h"
#include "fhiclcpp/types/OptionalDelegatedParameter.h"
#include "fhiclcpp/types/Sequence.h"

#include <vector>

namespace phot {
  //-------------------------------------------------------------------------//
  /*!
   * FHiCL configuration data used by the OpticalPropPDFastSimPAR tool.
   */
  struct Config {

    //!@{
    //! Type aliases
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    using DP = fhicl::DelegatedParameter;
    using ODP = fhicl::OptionalDelegatedParameter;
    //!@}

    DP OpticalPropagationTool{Name("OpticalPropagationTool"),
                              Comment("Tool describing the optical propagation mechanism")};
    const std::vector<int> default_TPCs{};
    fhicl::Atom<art::InputTag> SimulationLabel{Name("SimulationLabel"),
                                               Comment("SimEnergyDeposit label.")};
    fhicl::Atom<bool> DoFastComponent{Name("DoFastComponent"),
                                      Comment("Simulate slow scintillation light, default true"),
                                      true};
    fhicl::Atom<bool> DoSlowComponent{Name("DoSlowComponent"),
                                      Comment("Simulate slow scintillation light")};
    fhicl::Atom<bool> DoReflectedLight{Name("DoReflectedLight"),
                                       Comment("Simulate reflected visible light")};
    fhicl::Atom<bool> IncludeAnodeReflections{Name("IncludeAnodeReflections"),
                                              Comment("Simulate anode reflections, default false"),
                                              false};
    fhicl::Atom<bool> IncludePropTime{Name("IncludePropTime"),
                                      Comment("Simulate light propagation time")};
    fhicl::Atom<bool> GeoPropTimeOnly{
      Name("GeoPropTimeOnly"),
      Comment("Simulate light propagation time geometric approximation, default false"),
      false};
    fhicl::Atom<bool> UseLitePhotons{
      Name("UseLitePhotons"),
      Comment("Store SimPhotonsLite/OpDetBTRs instead of SimPhotons")};
    fhicl::Atom<bool> OpaqueCathode{Name("OpaqueCathode"),
                                    Comment("Photons cannot cross the cathode")};
    fhicl::Atom<bool> OnlyActiveVolume{
      Name("OnlyActiveVolume"),
      Comment("PAR fast sim usually only for active volume, default true"),
      true};
    fhicl::Sequence<int> RestrictedTPCs{Name("RestrictedTPCs"),
                                        Comment("Simulate for EDeps only in these TPCs.\nDefault "
                                                "is empty which means simulate in all TPCs"),
                                        default_TPCs};
    fhicl::Atom<bool> OnlyOneCryostat{Name("OnlyOneCryostat"),
                                      Comment("Set to true if light is only supported in C:1")};
    DP ScintTimeTool{Name("ScintTimeTool"),
                     Comment("Tool describing scintillation time structure")};
    DP OpticalPathTool{
      Name("OpticalPathTool"),
      Comment(
        "Tool to determine visibility of optical detectors from scintillation emission points")};
    fhicl::Atom<bool> UseXeAbsorption{
      Name("UseXeAbsorption"),
      Comment("Use Xe absorption length instead of Ar, default false"),
      false};
    ODP VUVTiming{Name("VUVTiming"), Comment("Configuration for UV timing parameterization")};
    ODP VISTiming{Name("VISTiming"), Comment("Configuration for visible timing parameterization")};
    DP VUVHits{Name("VUVHits"), Comment("Configuration for UV visibility parameterization")};
    ODP VISHits{Name("VISHits"), Comment("Configuration for visibile visibility parameterization")};
    fhicl::Atom<bool> Verbose{Name("Verbose"), Comment("Print verbose information"), false};
  };

  //-------------------------------------------------------------------------//
  //! Type aliases
  using OpticalPropPDFastSimParameters = art::EDProducer::Table<Config>;

  //-------------------------------------------------------------------------//
} // namespace phot
