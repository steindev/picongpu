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

namespace picongpu::photonTransport::scatterer
{
    /** Implementation of a photon scatterer functor that simply deflects photons to the right.
     *
     */
    template<typename T_AllRightParam>
    struct AllRight : public T_AllRightParam
    {
        using Params = T_AllRightParam;

        template<typename T_Photon>
        HDINLINE void operator()(T_Photon& photon)
        {
            // turn photons to the right by 1 deg.
            auto const deltaTheta = -Params::deltaTheta;
            float_X sinValue, cosValue;
            pmacc::math::sincos(precisionCast<float_X>(deltaTheta), sinValue, cosValue);

            using MomType = ::picongpu::momentum::type;
            MomType mom = photon[momentum_];
            // internal units, scaled

            auto new_mom_x = mom.x() * cosValue - mom.y() * sinValue;
            auto new_mom_y = mom.y() * cosValue + mom.x() * sinValue;

            photon[momentum_] = MomType(new_mom_x, new_mom_y, mom.z());
        }
    };
}