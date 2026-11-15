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
#include "numeric_pipeline.hpp"
namespace libre2_processing {
struct DecodedNumericInputs {
    NumericParameters parameters{};
    NumericState previous{};
    std::array<std::uint16_t,80>trend{};
    std::array<std::uint16_t,160>history{};
    std::uint16_t previous_history_index{},new_history_count{};
};
// Native 0x22fb58 before the numeric core, for chronological processing of an
// initialized EU configuration. Raw tables are the serialized, decoded cache.
inline DecodedNumericInputs prepare_decoded_inputs(
        std::span<const std::uint8_t,configuration_size> c,bool is_scan,unsigned wear_grace=wear_grace_minutes){
    DecodedNumericInputs out{};auto& p=out.parameters;auto word=[&](unsigned o){return read_state_u16(c.data()+o);};
    p.age=word(0x10);p.elapsed_minutes=configuration_elapsed_minutes(c);p.warmup_minutes=word(0x12);
    p.wear_minutes=processing_wear_minutes(word(0x14),wear_grace);
    p.calibration={std::int16_t(word(0x180)),std::int16_t(word(0x182)),word(0x1a),
        {read_state_float(c.data()+0x18c),read_state_float(c.data()+0x188)},c[0x191],c[0x192]};
    p.is_scan=is_scan;p.feature_bits=c[0x30];p.low=word(0x16);p.high=word(0x18);p.maximum=word(0x1c);p.minimum=word(0x1e);
    p.blend_previous_current=(c[processing_flags_offset]&skip_current_blend)==0;
    p.history_output_delay_minutes=(c[processing_flags_offset]&delayed_history)?2:0;
    for(unsigned i=0;i<3;++i)p.reference[i]=read_state_float(c.data()+0x34+4*i);
    out.previous.minute=word(0x374);out.previous.filtered_glucose=read_state_float(c.data()+0x384);
    out.previous.filtered_rate=read_state_float(c.data()+0x388);out.previous.rate_quality=word(0x382);
    for(unsigned i=0;i<31;++i){const auto h=0x3a4+10*i;out.previous.history[i]={word(h),word(h+2),word(h+4),read_state_float(c.data()+h+6)};}
    p.history_last_valid_index=completed_history_index(p.age);
    p.history_first_index=std::uint16_t(std::max(1,int(p.age/15)-31));
    out.previous_history_index=completed_history_index(out.previous.minute);
    p.new_history_first_index=std::max(p.history_first_index,std::uint16_t(out.previous_history_index+1));
    p.history_source_offset=std::uint16_t(p.new_history_first_index-p.history_first_index);
    out.new_history_count=std::uint16_t(p.history_last_valid_index-p.new_history_first_index+1);
    p.current_trend_index=std::uint8_t(std::min(15,p.age?int(p.age)-1:0));
    p.current_history_index=std::uint8_t(std::clamp(int(p.age/15)-int(p.history_first_index),0,31));
    for(unsigned i=0;i<80;++i)out.trend[i]=word(0x194+2*i);
    const unsigned source_first=unsigned(std::max(1,int(p.history_last_valid_index)-31));
    const unsigned shift=p.history_first_index-source_first;
    for(unsigned i=0;i<32;++i){
        for(unsigned j=0;j<5;++j)out.history[5*i+j]=i+shift<32?word(0x234+10*(i+shift)+2*j):0;
        if(i+shift>=32)out.history[5*i+4]=0x8000;
    }
    const unsigned latest=unsigned(std::max(0,int(p.history_last_valid_index)-int(p.history_first_index)));
    if(p.current_history_index>latest){
        auto offset=5*p.current_history_index;std::fill_n(out.history.begin()+offset,5,0);out.history[offset+4]=0x8000;
    }
    qualify_input_window(false,p.current_trend_index,15,out.trend.data(),p.age,p.elapsed_minutes,p.wear_minutes);
    qualify_input_window(true,latest,31,out.history.data(),p.age,p.elapsed_minutes,p.wear_minutes);
    return out;
}
} // namespace libre2_processing
