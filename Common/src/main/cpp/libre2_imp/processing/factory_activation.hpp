/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Sun Nov 15 13:57:27 CET 2026                                                 */

#pragma once
#include "recovered_components.hpp"
#include <bit>
namespace libre2_processing {
struct FactoryActivation {
    std::int16_t activation_day{};
    std::uint16_t storage_days{};
    FactoryCoefficients coefficients{};
};
// Native 0x221ecc. This timestamp is Unix seconds, not the algorithm's
// separately stored 2010-epoch timestamp. The native code narrows the computed
// day to int16 before comparing it with the unsigned manufacturing day.
inline FactoryActivation derive_factory_activation(const FactoryMetadata& metadata,
        std::uint32_t unix_seconds,const std::array<std::int32_t,4>& profile){
    const int day=int(unix_seconds/86400u)-15706;
    FactoryActivation out{};
    out.activation_day=std::bit_cast<std::int16_t>(std::uint16_t(day));
    if(int(out.activation_day)>int(metadata.factory_day))
        out.storage_days=std::uint16_t(int(out.activation_day)-int(metadata.factory_day));
    out.coefficients=calculate_factory_coefficients(metadata.index,out.storage_days,metadata.correction_code,profile);
    return out;
}
inline FactoryActivation derive_factory_activation(const FactoryMetadata& metadata,
        std::uint32_t unix_seconds){
    return derive_factory_activation(metadata,unix_seconds,native_factory_parameters);
}
} // namespace libre2_processing
