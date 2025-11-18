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
#include "picongpu/particles/photonTransport/kernel/UpdatePhotonMomentum.kernel"
#include "picongpu/particles/photonTransport/AllLeft.hpp"

#include <pmacc/Environment.hpp>
#include <pmacc/mappings/kernel/AreaMapping.hpp>
#include <pmacc/particles/meta/FindByNameOrType.hpp>
#include <pmacc/type/Area.hpp>

namespace picongpu::simulation::stage
{
    namespace detail
    {
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
            using TransportAlias = typename pmacc::traits::GetFlagType<FrameType, picongpu::photonScatterer<>>::type;

            //! this resolves the alias into the actual object type, a list of photons
            using TransportMethod = typename pmacc::traits::Resolve<TransportAlias>::type;

            //! apply momentum update for species, requires known momentum change per macro-particle
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
                    UpdatePhotonMomentumKernel<PhotonSpecies>;
                // macro for call of kernel on every superCell, see pull request #4321
                PMACC_LOCKSTEP_KERNEL(UpdatePhotonMomentum())
                    .config(mapper.getGridDim(), photons)(
                        mapper,
                        photons.getDeviceParticlesBox());
            }

            /** Functor implementation
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
