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

#pragma once

#include "picongpu/defines.hpp"
#include "picongpu/particles/param.hpp"
#include <pmacc/particles/traits/FilterByFlag.hpp>

namespace picongpu::simulation::stage
{
    //! Functor for the stage of the PIC loop performing photon transport
    struct PhotonTransport
    {
        // Deduce whether there is a species with a "photonTransport<>" flag. If so, set photonTransportActive to True.
        using SpeciesForPhotonTransport = typename pmacc::particles::traits::FilterByFlag<VectorAllSpecies, picongpu::photonScatterer<>>::type;

        static constexpr auto numberPhotonTransportSpecies = pmacc::mp_size<SpeciesForPhotonTransport>::value;

        static constexpr bool photonTransportActive = numberPhotonTransportSpecies > 0;

    public:
        PhotonTransport(picongpu::MappingDesc const);

        /** Perform scattering of photon species from background electron density
         *
         * @param cellDescription
         * @param currentStep index of time iteration
         */
        void operator()(picongpu::MappingDesc const cellDescription, uint32_t const currentStep) const;
    };
} // namespace picongpu::simulation::stage
