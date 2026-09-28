/* Copyright 2026 Uwe Hernandez Acosta, Klaus Steiniger
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "picongpu/simulation/stage/PhotonTransport.hpp"

#include "picongpu/defines.hpp"
#include "picongpu/particles/atomicPhysics/stage/BinElectrons.hpp"
#include "picongpu/particles/atomicPhysics/debug/stage/DumpSuperCellDataToConsole.hpp"
#include "picongpu/particles/photonTransport/kernel/UpdatePhotonMomentum.kernel"

#include <pmacc/Environment.hpp>
#include <pmacc/mappings/kernel/AreaMapping.hpp>
#include <pmacc/particles/meta/FindByNameOrType.hpp>
#include <pmacc/type/Area.hpp>

namespace picongpu::simulation::stage
{
    namespace detail
    {
        namespace enums
        {
            enum struct Loop : uint8_t
            {
                SubStep,
                ChooseTransition,
                RejectOverSubscription,
                ApplyInstantTransitions
            };
        } // namespace enums

        namespace debug = picongpu::atomicPhysics::debug;
        namespace localHelperFields = picongpu::particles::atomicPhysics::localHelperFields;
        namespace electronDistribution = picongpu::particles::atomicPhysics::electronDistribution;


        /** Extract scattering method and execute it for a single species
         *
         * @tparam T_SpeciesType type or name as PMACC_CSTRING of particle species that is checked for photonTransport
         */
        template<typename T_SpeciesType>
        struct ScatterPhotons
        {
            using PhotonSpecies = pmacc::particles::meta::FindByNameOrType_t<VectorAllSpecies, T_SpeciesType>;
            using FrameType = typename PhotonSpecies::FrameType;

            //! the following line only fetches the alias
            using ScattererAlias = typename pmacc::traits::GetFlagType<FrameType, picongpu::photonScatterer<>>::type;

            //! this resolves the alias into the actual object type, the scatter functor
            using PhotonScatterer = typename pmacc::traits::Resolve<ScattererAlias>::type;

            // Repeat determination of electrons species to scatter from
            //! TODO: Repeating is better than dragging it around, isn't it?
            using PhotonTransportElectronSpecies = typename particles::photonTransport::traits::
            FilterByParticleType_t<VectorAllSpecies, picongpu::particles::photonTransport::Tags::Electron>;
#if 0
            using BinSelection = electronDistribution::enums::BinSelection;

        private:
            //! debug print to console
            //!@{

            //! control multiple location debug prints depending on debug output setting
            template<enums::Loop T_Loop>
            static constexpr bool debugPrintActive()
            {
                constexpr bool isSubStepLoop = (T_Loop == enums::Loop::SubStep);
                constexpr bool isChooseTransition = (T_Loop == enums::Loop::ChooseTransition);
                constexpr bool isRejectOverSubscription = (T_Loop == enums::Loop::RejectOverSubscription);

                constexpr bool isActive
                    = (isSubStepLoop)
                      || (isChooseTransition && debug::kernel::recordSuggestedChanges::PRINT_DEBUG_TO_CONSOLE)
                      || (isRejectOverSubscription && debug::kernel::rollForOverSubscription::PRINT_DEBUG_TO_CONSOLE);

                return isActive;
            }

            //! print electron histogram to console, debug only
            template<BinSelection T_BinSelection, enums::Loop T_Loop>
            HINLINE static void printHistogramToConsole(picongpu::MappingDesc const& mappingDesc, std::string name)
            {
                constexpr bool printActive = debug::electronHistogram::PRINT_TO_CONSOLE && debugPrintActive<T_Loop>();
                if constexpr(printActive)
                {
                    std::cout << name << std::endl;
                    picongpu::particles::atomicPhysics::stage::DumpSuperCellDataToConsole<
                        electronDistribution::
                            LocalHistogramField<picongpu::atomicPhysics::ElectronHistogram, picongpu::MappingDesc>,
                        electronDistribution::PrintHistogramToConsole<T_BinSelection>>{}(
                        mappingDesc,
                        "Electron_HistogramField");
                }
            }

            //!@}

            HINLINE static void binElectronsToEnergyHistogram(picongpu::MappingDesc const& mappingDesc)
            {
                //! TODO: Figure out how to make use of this histogram.
                //! TODO: For now, I actually doe not need to create a histogram, as I want a one-to-one relation
                using ForEachElectronSpeciesBinElectrons = pmacc::meta::ForEach<
                    PhotonTransportElectronSpecies,
                    particles::atomicPhysics::stage::BinElectrons<boost::mpl::_1>>;
                ForEachElectronSpeciesBinElectrons{}(mappingDesc);

                printHistogramToConsole<BinSelection::All, enums::Loop::SubStep>(mappingDesc, "[after binning]");
            }

            //! @attention assumes that all macro ions' choose a transition have been updated
            HINLINE static void updateElectrons(picongpu::MappingDesc const& mappingDesc, uint32_t const currentStep)
            {
                //! TODO: This is the end of the current implementation
                //! TODO: Here, update the electron from which the photon scattered
                //! TODO: However, this is not what is implemented. I need a one-to-one relation between photon and electron
                //! TODO: while this goes over all electrons at once.
                //! TODO: So I need to write a kernel that does that and passes one electron and one photon to a functor.
                /** @note DecelerateElectrons must be called before SpawnIonizationElectrons such that we only
                 * change electrons that actually contributed to the histogram*/
                using ForEachElectronSpeciesDecelerateElectrons = pmacc::meta::ForEach<
                    PhotonTransportElectronSpecies,
                    particles::atomicPhysics::stage::DecelerateElectrons<boost::mpl::_1>>;
                ForEachElectronSpeciesDecelerateElectrons{}(mappingDesc);

                /// @details all ions choose a transition -> no need to check that
                spawnIonizationElectrons</*checkForAccepted*/ std::false_type>(mappingDesc, currentStep);
            }
#endif
            //! TODO: I do have electrons now.
            //! TODO: But they are "hidden" in the list of all electron species PhotonTransportElectronSpecies
            //! apply momentum update to all photons in photon species
            HINLINE static void updatePhotons(picongpu::MappingDesc const& mappingDesc, uint32_t const currentStep)
            {
                if (currentStep < uint32_t(180))
                    return;
                // call kernel for every supercell

                // TODO: can be moved to constructor, since photons are the same in all methods of this class
                // full local domain, no guards
                pmacc::AreaMapping<CORE + BORDER, MappingDesc> mapper(mappingDesc);
                pmacc::DataConnector& dc = pmacc::Environment<>::get().DataConnector();
                // pointer to memory, we will only work on device, no sync required
                auto& photons = *dc.get<PhotonSpecies>(FrameType::getName());

                using UpdatePhotonMomentum = picongpu::particles::photonTransport::kernel ::
                    UpdatePhotonMomentumKernel<PhotonSpecies, PhotonScatterer>;
                // macro for call of kernel on every superCell, see pull request #4321
                PMACC_LOCKSTEP_KERNEL(UpdatePhotonMomentum{})
                    .config(mapper.getGridDim(), photons)(
                        mapper,
                        photons.getDeviceParticlesBox());
            }

        public:
            ScatterPhotons() = default;

            /** photon transport stage sub-stage calls
             *
             * @param mappingDesc logical block information like dimension and cell sizes
             * @param currentStep The current time step
             */
            HINLINE void operator()(MappingDesc const mappingDesc, uint32_t const currentStep)
            {
                // no data, i.e. fields, from the environment needed
                //pmacc::DataConnector& dc = pmacc::Environment<>::get().DataConnector();

                // There is space here for more class methods which may implement
                // additional steps such as computing an interaction probability or
                // the applied momentum change according to TransportMethod, see AtomicPhysics::operator()

                updatePhotons(mappingDesc, currentStep);
            } // end ScatterPhotons::operator()
        }; // end struct ScatterPhotons
    }

    PhotonTransport::PhotonTransport(picongpu::MappingDesc const)
    {
        if constexpr(photonTransportActive)
        {
            // init photonTransport fields and buffers
            // However, data will usually be required in detail::ScatterPhotons and not in this
            // PhotonTransport stage implementation which mainly calls detail::ScatterPhotons
        }
    }

    /** Indirection of work to ScatterPhotons class in 'detail' namespace in order to avoid compilation of code
     * in case there are no species for photon transport.
     *
     * Execute ScatterPhotons functor for each species with photonTransport flag.
     *
     * @param mappingDesc
     * @param currentStep
     */
    void PhotonTransport::operator()(MappingDesc const mappingDesc, uint32_t const currentStep) const
    {
        if constexpr(photonTransportActive)
        {
            meta::ForEach<SpeciesForPhotonTransport, detail::ScatterPhotons<boost::mpl::_1>> scatterPhotons;
            scatterPhotons(mappingDesc, currentStep);
        }
    }
} // namespace picongpu::simulation::stage
