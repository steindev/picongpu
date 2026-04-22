/* Copyright 2026 Axel Huebl, Rene Widera, Klaus Steiniger
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
#include "picongpu/traits/attribute/GetMass.hpp"

namespace picongpu
{
    namespace particles
    {
        namespace manipulators
        {
            namespace unary
            {
                namespace acc
                {
                    /** manipulate the momentum
                     *
                     * @tparam T_ParamClass picongpu::particles::manipulators::unary::param::DriftFromEnergyCfg,
                     *                      type with compile configuration
                     * @tparam T_ValueFunctor pmacc::math::operation, binary operator type to reduce current and new
                     * value
                     */
                    template<typename T_ParamClass, typename T_ValueFunctor>
                    struct DriftFromEnergy : private T_ValueFunctor
                    {
                        /** manipulate the momentum of the particle
                         *
                         * @tparam T_Particle pmacc::Particle, particle type
                         *
                         * @param particle particle to be manipulated
                         */
                        template<typename T_Particle>
                        HDINLINE void operator()(T_Particle& particle)
                        {
                            using ParamClass = T_ParamClass;
                            using ValueFunctor = T_ValueFunctor;

                            float_X const macroWeighting = particle[weighting_];
                            float_X const macroMass = picongpu::traits::attribute::getMass(macroWeighting, particle);

                            float_64 const energy_SI = float_64(1000) * ParamClass::energy * sim.si.get_eV();
                            float_64 const energy_PIC = energy_SI / sim.unit.energy();

                            float_64 const macroEnergyOverC = float_64(macroWeighting) * energy_PIC / float_64(sim.pic.getSpeedOfLight());
                            float_64 const macroRestEnergyOverC = float_64(macroMass) * float_64(sim.pic.getSpeedOfLight());

                            float_64 const momValue = math::sqrt(macroEnergyOverC*macroEnergyOverC - macroRestEnergyOverC*macroRestEnergyOverC);

                            float3_X const driftDirection = ParamClass::driftDirection;
                            float3_X const normDir = driftDirection / pmacc::math::l2norm(driftDirection);

                            float3_X const mom(normDir * float_X(momValue));

                            ValueFunctor::operator()(particle[momentum_], mom);
                        }
                    };

                } // namespace acc
            } // namespace unary
        } // namespace manipulators
    } // namespace particles
} // namespace picongpu
