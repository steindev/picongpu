See changes in github: (https://github.com/ComputationalRadiationPhysics/picongpu/compare/dev...steindev:picongpu:2025-11_photon-transport)

Rene said I should follow along the ParticlePush implementation.


Questions
=========
* The ParticlePush has a communication event `commEvent` as an `operator()` argument.
  What is it doing with it?
  Probably communicating particles after they have been pushed such that they are transferred between units.
  However, PhotonTransport only changes momentum and not position.
  Therefore, it should not be necessary to communicate.

* in ScatterPhotons::updatePhotons() I do the following:
  `auto& photons = *dc.get<PhotonSpecies>(FrameType::getName());`
  but in PushSpecies::operator()() we do
  `auto species = dc.get<SpeciesType>(FrameType::getName());`
  Where is the difference and is it important?

* Warum wird in KernelMoveAndMarkParticles in Particles.kernel T_DataDomain als Template and die Klasse übergeben
  und nicht als Argument an die operator() Methode?
  Weil es keine Instanz von T_DataDomain gibt die and Kernel übergeben wird.
  -> Man bräuchte auch keine Instanz des FrameSolver (= particleFunctor) übergeben sondern könnte den auch als
  Template-Argument an die Klasse des Kernel übergeben.
  
* Die PushParticlePerFrame::operator()() Methode hat als erstes Argument einen `worker`, benutzt ihn aber garnicht.
  Kann weg, oder?


Hook: include/picongpu/simulation/control/Simulation.hpp
========================================================
## `init()`
* create PhotonTransport stage (== Functor!) and pass required data for **instantion** (not running) of this stage, e.g.
* not all stages are initialized here, some are directly instantiated in `runOneStep()`, why?


## `runOneStep()`
* call PhotonTransport stage as a functor and pass required data for running this stage


include/picongpu/simulation/stage/PhotonTransport.hpp
=====================================================
deduces species for photon transport `SpeciesForPhotonTransport` from type alias photonTransport

* Do I need the `cellDescription` in the constructor of `PhotonTransport`?
* I could also save the cellDescription as an member of the `PhotonTransport` class...
* Do I need `cellDescription` as an argument of the `operator()` method of PhotonTransport?
  If it is a member of the `PhotonTransport` then definitely not?
  atomicPhysics has it in the constructor and operator(), but does not define it as a member



include/picongpu/simulation/stage/PhotonTransport.x.cpp
=======================================================
implements `PhotonTransport::operator()` and the `ScatterPhoton` functor

## `PhotonTransport::operator()`
Calls ScatterPhoton functor for each SpeciesForPhotonTransport.



Use from AtomicPhysics.x.cpp: `binElectronsToEnergyHistogram`

