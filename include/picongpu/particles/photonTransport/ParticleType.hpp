/* Copyright 2024-2026 Brian Marre, Klaus Steiniger
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
#include "picongpu/particles/photonTransport/ParticleTags.hpp"

#include <pmacc/particles/traits/FilterByFlag.hpp>
#include <pmacc/traits/GetFlagType.hpp>
#include <pmacc/traits/HasFlag.hpp>

#include <cstdint>

namespace picongpu::particles::photonTransport
{
    /** indicates species representing electrons and will participate in photonTransport
     *
     * @attention In addition a photonTransport Electron species requires the following particle attributes:
     * - momentum
     * - weighting
     */
    struct Electron
    {
        using Tag = Tags::Electron;
    };

    namespace traits
    {
        //! check if specified ParticleType has the specified ParticleTag
        //!@{
        //! not found default case
        template<typename T_ParticleType, typename T_ParticleTypeTag, typename = void>
        struct IsParticleType : std::false_type
        {
        };

        //! found case
        template<typename T_ParticleType, typename T_ParticleTypeTag>
        struct IsParticleType<
            T_ParticleType,
            T_ParticleTypeTag,
            std::enable_if_t<std::is_same_v<typename T_ParticleType::Tag, T_ParticleTypeTag>>> : std::true_type
        {
        };

        //! short hand
        template<typename T_ParticleType, typename T_ParticleTypeTag>
        using IsParticleType_t = typename IsParticleType<T_ParticleType, T_ParticleTypeTag>::type;

        //@}

        template<typename T_FrameType>
        struct GetParticleType
        {
            using type = typename pmacc::traits::Resolve<
                typename pmacc::traits::GetFlagType<T_FrameType, photonTransportParticle<>>::type>::type;
        };

        //! short hand
        template<typename T_FrameType>
        using GetParticleType_t = typename GetParticleType<T_FrameType>::type;

        //! check whether particle belongs to a species with given ParticleTypeTag
        //!@{
        template<typename T_ParticleTypeTag, typename T_Particle>
        HDINLINE bool hasParticleTypeTag(T_Particle const&)
        {
            return hasParticleTypeTag<T_ParticleTypeTag, T_Particle>();
        }

        template<typename T_ParticleTypeTag, typename T_Particle>
        constexpr bool hasParticleTypeTag()
        {
            using FrameType = typename std::decay_t<T_Particle>::FrameType;
            constexpr bool hasParticleType = pmacc::traits::HasFlag<FrameType, photonTransportParticle<>>::type::value;

            return hasParticleType && IsParticleType<GetParticleType_t<FrameType>, T_ParticleTypeTag>::value;
        }

        //!@}

        //! filter species list by ParticleTypeTag
        template<typename T_MPLSeq, typename T_ParticleTypeTag>
        struct FilterByParticleType
        {
            using PhotonTransportSpecies =
                typename pmacc::particles::traits::FilterByFlag<T_MPLSeq, photonTransportParticle<>>::type;

            template<typename T_Species>
            using IsTagedType = IsParticleType_t<GetParticleType_t<typename T_Species::FrameType>, T_ParticleTypeTag>;

            using type = pmacc::mp_copy_if<PhotonTransportSpecies, IsTagedType>;
        };

        //! short hand
        template<typename T_MPLSeq, typename T_ParticleTypeTag>
        using FilterByParticleType_t = typename FilterByParticleType<T_MPLSeq, T_ParticleTypeTag>::type;

    } // namespace traits
} // namespace picongpu::particles::photonTransport
