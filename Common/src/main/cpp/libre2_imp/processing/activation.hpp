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
#include "eu_crypto.hpp"
#include "factory_activation.hpp"
#include "nfc_processing.hpp"
namespace libre2_processing {
struct ActivationOptions {
    // Display limits (40/500), algorithm version (4) and disabled
    // non-actionable checks are fixed by this tested EU native profile.
    std::uint16_t warmup_minutes=60,low=70,high=240;
    std::uint8_t feature_bits=14;
    std::uint16_t wear_minutes=libre_wear_minutes;
};
struct InitialStateResult {bool accepted{};PersistentState state{};};
// Native NFC timestamp conversion, before the numerical processing window.
// Preserve the double operation order and the final floor(x+0.5).
inline std::uint32_t sensor_elapsed_seconds_eu(std::uint16_t minute,std::uint16_t clock_word,
        std::uint16_t wear_minutes=libre_wear_minutes){
    if(!clock_word)return 0;
    double clock_days=14.5;
    switch(wear_minutes){
        case 10*minutes_per_day:clock_days=10.36;break;
        case libre_plus_wear_minutes:clock_days=15.54;break;
        case 21*minutes_per_day:clock_days=21.75;break;
        case 30*minutes_per_day:clock_days=31.07;break;
    }
    double scale=clock_days*24.0;
    scale=scale*60.0;scale=scale/double(clock_word);
    double elapsed=double(minute)*scale;elapsed=elapsed*1.0075;elapsed=elapsed*60.0;
    return std::uint32_t(std::uint64_t(std::floor(elapsed+0.5)));
}
inline std::uint32_t estimate_activation_start_eu(std::span<const std::uint8_t,344>fram,std::uint32_t now){
    // The footer carries the factory type used by the native clock conversion.
    const auto wear=nominal_wear_minutes(fram[0x142]);
    return now-sensor_elapsed_seconds_eu(read_state_u16(fram.data()+0x13c),read_state_u16(fram.data()+0x146),wear);
}
// Current EU profile, including Juggluco's disabled non-actionable checks.
// No prior MESS, coefficients or expected glucose are inputs.
inline InitialStateResult make_initial_state_eu(std::span<const std::uint8_t,344>fram,
        const ActivationOptions&options={}){
    InitialStateResult out{};const auto p=decode_nfc_payload(fram);
    if(!p.checksums_valid||!p.pointers_valid)return out;
    auto& s=out.state;std::copy(state_header.begin(),state_header.end(),s.begin());auto*c=s.data()+state_header_size;
    auto word=[&](unsigned offset,std::uint16_t v){write_state_u16(c+offset,v);};
    write_processing_u32(c+8,10368);word(12,p.factory.factory_day);
    word(0x12,options.warmup_minutes);word(0x14,options.wear_minutes);word(0x16,options.low);word(0x18,options.high);
    word(0x1a,1045);word(0x1c,500);word(0x1e,40);
    word(0x26,500);word(0x28,70);word(0x2a,20);word(0x2c,std::uint16_t(-20));c[0x30]=options.feature_bits;
    for(unsigned i=0;i<3;++i)write_processing_float(c+0x34+4*i,4294967296.f);
    std::fill_n(c+0x40,320,0xff);
    word(0x180,std::uint16_t(p.factory.references.upper));word(0x182,std::uint16_t(p.factory.references.lower));
    word(0x184,read_state_u16(fram.data()+0x146));word(0x186,p.factory.references.temperature_reference);word(0x188,p.factory.index);
    c[0x190]=p.factory.correction_code;c[0x191]=p.factory.average_delta;c[0x192]=p.factory.peak_delta;
    c[processing_flags_offset]=delayed_history;
    for(unsigned i=0;i<48;++i)word(0x194+10*i+8,0x8000);
    for(unsigned i=0;i<31;++i){word(0x3a4+10*i+2,0xffff);word(0x3a4+10*i+4,std::uint16_t(15*(i+1)));}
    write_state_u16(s.data()+state_checksum_offset,state_crc(std::span<const std::uint8_t>(s).subspan(state_header_size,configuration_size-2)));
    out.accepted=true;return out;
}
struct ActivationCompletion {bool accepted{};PersistentState state{};};
// Native activationComplete's state transition. day is days since 2013-01-01;
// start is the processing epoch's seconds since 2010-01-01. Use an unactivated
// state: the factory index/day occupy slots later overwritten by coefficients.
inline ActivationCompletion activation_complete_eu(std::span<const std::uint8_t,state_size>initial,
        std::int32_t day,std::uint32_t start){
    ActivationCompletion out{};
    if(!valid_state(initial))return out;
    std::copy_n(initial.begin(),state_header_size+0x520,out.state.begin());
    auto*c=out.state.data()+state_header_size;
    if(day!=0&&start){
        FactoryMetadata m{};m.index=read_state_u16(c+0x188);m.factory_day=read_state_u16(c+12);m.correction_code=c[0x190];
        const auto unix_day_seconds=std::uint32_t(std::uint32_t(day)*86400u+1356998400u);
        const auto factory=derive_factory_activation(m,unix_day_seconds);
        write_processing_u32(c,start);write_state_u16(c+12,factory.storage_days);
        write_processing_float(c+0x188,factory.coefficients.offset);write_processing_float(c+0x18c,factory.coefficients.divisor);
    }
    std::copy(state_header.begin(),state_header.end(),out.state.begin());
    write_state_u16(out.state.data()+state_checksum_offset,state_crc(std::span<const std::uint8_t>(out.state).subspan(state_header_size,configuration_size-2)));
    out.accepted=true;return out;
}
inline std::int32_t activation_day_from_start(std::uint32_t start){return start>94694400u?std::int32_t((start-94694400u)/86400u):0;}
struct EuActivationPlan {
    bool accepted{};std::uint8_t command=0xa1;std::array<std::uint8_t,5>payload{};PersistentState initial_state{};
};
inline EuActivationPlan prepare_activation_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::span<const std::uint8_t,344>encrypted,
        ActivationOptions options={}){
    EuActivationPlan out{};const auto plain=decrypt_nfc_eu(uid,info,encrypted);if(!plain.valid)return out;
    options.wear_minutes=std::uint16_t(patch_metadata(info).wear_minutes);
    const auto initial=make_initial_state_eu(plain.bytes,options);if(!initial.accepted)return out;
    out.initial_state=initial.state;out.payload=activation_payload_eu(uid);out.accepted=true;return out;
}
struct EuBootstrapResult {bool accepted{};PersistentState state{};std::uint32_t start{},end{};};
inline EuBootstrapResult initialize_sensor_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::span<const std::uint8_t,344>encrypted,
        std::uint32_t algorithm_timestamp,ActivationOptions options={}){
    EuBootstrapResult out{};const auto plain=decrypt_nfc_eu(uid,info,encrypted);if(!plain.valid)return out;
    options.wear_minutes=std::uint16_t(patch_metadata(info).wear_minutes);
    const auto initial=make_initial_state_eu(plain.bytes,options);if(!initial.accepted)return out;
    out.state=initial.state;out.start=estimate_activation_start_eu(plain.bytes,algorithm_timestamp);out.end=algorithm_timestamp;out.accepted=true;return out;
}
} // namespace libre2_processing
