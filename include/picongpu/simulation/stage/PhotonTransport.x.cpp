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
            using SpeciesType = pmacc::particles::meta::FindByNameOrType_t<VectorAllSpecies, T_SpeciesType>;
            using FrameType = typename SpeciesType::FrameType;

            //! the following line only fetches the alias
            using TransportAlias = typename pmacc::traits::GetFlagType<FrameType, picongpu::photonTransport<>>::type;

            //! this now resolves the alias into the actual object type, a list of photons
            using TransportMethod = typename pmacc::traits::Resolve<TransportAlias>::type;

            /** Functor implementation
             *
             * @tparam T_CellDescription contains the number of blocks and blocksize
             *                           that is later passed to the kernel
             * @param cellDesc logical block information like dimension and cell sizes
             * @param currentStep The current time step
             */
            template<typename T_CellDescription>
            HINLINE void operator()(T_CellDescription cellDesc, uint32_t const currentStep);
        };
    }

    PhotonTransport::PhotonTransport()
    {
        if constexpr(photonTransportActive)
        {
            // init photonTransport fields and buffers
        }
    }

    /** Indirection of work to ScatterPhotons class in 'detail' namespace in order to avoid compilation of code
     * in case there are species for photon transport.
     *
     * Execute ScatterPhotons functor for each species with photonTransport flag.
     *
     * @param cellDescription
     * @param currentStep
     */
    void PhotonTransport::operator()(MappingDesc const cellDescription, uint32_t const currentStep) const
    {
        if constexpr(photonTransportActive)
        {
            meta::ForEach<SpeciesForPhotonTransport, detail::ScatterPhotons<boost::mpl::_1>> scatterPhotons;
            scatterPhotons(cellDescription, currentStep);
        }
    }
} // namespace picongpu::simulation::stage
