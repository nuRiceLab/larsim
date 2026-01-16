/**
 *  File: larsim/PhotonPropagation/OpticalPropagationTools/Opticks_tool.h
 *  Author: Ilker Parmaksiz
 *  Experiment: DUNE
 *  Institution: Rice University
 *  Date: 1/9/26
 *  Description: Photon Propagation on GPU by Opticks Library
 */
#pragma once

#ifndef OPTICKSINTERFACE_H
#define OPTICKSINTERFACE_H

#include "lardataobj/Simulation/OpDetBacktrackerRecord.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

#include "fhiclcpp/ParameterSet.h"
#include "larsim/PhotonPropagation/OpticalPropagationTools/Opticks/MySensorIdentifier.h"
#include "larsim/PhotonPropagation/OpticalPropagationTools/Opticks/OpticksHitHandler.h"
#include "larsim/PhotonPropagation/OpticalPropagationTools/IOpticalPropagation.h"


namespace fhicl {
  class ParameterSet;
}

namespace phot {

class MySensorIdentifier;
class OpticksHitHandler;

    class OpticksInterface : public IOpticalPropagation  {

    public:
      // Construct with fcl parameters
      OpticksInterface(fhicl::ParameterSet const& pset);

      // Default destructor
      ~OpticksInterface();

      void init();
      void CollectPhotons();
      void GetHitsFromGPU();
      void Simulate();
      void GetPhotonDetectors();

	  // Initialize fast simulation
	  void beginJob() override;

	  // Execute simulation on a single art::Event
	  UPVecBTR executeEvent(VecSED const& edeps) override;

	  // Finalize execution
	  void endJob() override;

	  private:
      std::string GDMLPath;
      MySensorIdentifier * OpticksSensorIdentifier;
      OpticksHitHandler* OpticksHits;
      std::map<G4String, G4int> DetectorIds;
  };
}

#endif //OPTICKSINTERFACE_H
