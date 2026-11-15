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
#include "persistent_state.hpp"
#include <cmath>
namespace libre2_processing {
// Native 0x23428c, stores at 0x23adec/0x23af7c. The anchors are the
// uncorrected factory values for history indices 3,4,5 (45,60,75 minutes
// with the usual zero source offset), retained after they leave the window.
inline std::array<float,3> refresh_reference_anchors(const float*glucose,
        const std::uint16_t*flags,unsigned prepared_first,unsigned source_offset,
        const std::array<float,3>&saved){
    auto out=saved;
    for(unsigned i=prepared_first;i<6;++i){
        if(i<3)continue;
        const unsigned slot=source_offset+i-prepared_first;
        if(slot<32)out[i-3]=flags[slot]?4294967296.f:glucose[slot];
    }
    return out;
}
// Native 0x225390: 320 compact factory-glucose samples, indexed in 15-minute
// slots. ff denotes a gap; the signed-word addition precedes byte clipping.
inline void cache_factory_history(std::span<std::uint8_t,configuration_size>c,
        std::int16_t glucose,std::uint16_t index,std::uint16_t quality=0){
    if(!index)return;
    const auto previous=read_state_u16(c.data()+0x32);
    if(index>previous){
        for(unsigned i=unsigned(previous)+1;i<unsigned(index);++i)c[0x40+(i-1)%320]=0xff;
        write_state_u16(c.data()+0x32,index);
    }
    const auto shifted=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(glucose)+128));
    c[0x40+(unsigned(index)-1)%320]=quality?0xff:static_cast<std::uint8_t>(std::clamp<int>(shifted,0,254));
}
// Core updates cover new indices only. The source flags are those immediately
// after calibration, before historical fitting changes their working status.
inline void cache_new_factory_history(std::span<std::uint8_t,configuration_size>c,
        const float*glucose,const std::uint16_t*flags,unsigned first,unsigned last){
    const unsigned previous=read_state_u16(c.data()+0x32);
    for(unsigned i=0;i<32&&first+i<=last;++i)if(first+i>previous){
        const auto rounded=static_cast<std::uint16_t>(std::lround(static_cast<double>(glucose[i])));
        cache_factory_history(c,std::bit_cast<std::int16_t>(rounded),static_cast<std::uint16_t>(first+i),flags[i]);
    }
}
} // namespace libre2_processing
