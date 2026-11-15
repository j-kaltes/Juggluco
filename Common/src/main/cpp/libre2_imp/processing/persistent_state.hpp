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
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <span>
namespace libre2_processing {
inline constexpr std::size_t state_size=13664,state_header_size=12,configuration_size=0x4e0;
inline constexpr std::size_t state_checksum_offset=0x4ea,persisted_result_offset=0x380,persisted_result_size=0x168;
inline constexpr std::size_t configuration_result_offset=persisted_result_offset-state_header_size;
inline constexpr unsigned processing_flags_offset=0x193;
inline constexpr std::uint8_t skip_current_blend=1,packet_samples_only=2,delayed_history=4;
static_assert(state_checksum_offset == state_header_size + configuration_size - 2);
static_assert(configuration_result_offset + persisted_result_size <= configuration_size - 2);
using PersistentState=std::array<std::uint8_t,state_size>;
using Configuration=std::array<std::uint8_t,configuration_size>;
inline constexpr std::array<std::uint8_t,12> state_header{0x38,0x78,0x22,0x14,1,0,12,0,1,0,0x20,5};
inline std::uint16_t read_state_u16(const std::uint8_t*p){return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8);}
inline std::uint32_t read_state_u32(const std::uint8_t*p){return read_state_u16(p)|(std::uint32_t(read_state_u16(p+2))<<16);}
inline float read_state_float(const std::uint8_t*p){return std::bit_cast<float>(read_state_u32(p));}
inline void write_state_u16(std::uint8_t*p,std::uint16_t v){p[0]=std::uint8_t(v);p[1]=std::uint8_t(v>>8);}
// Reflected 0x8408 CRC, initial ffff, followed by bit reversal; no final xor.
inline std::uint16_t state_crc(std::span<const std::uint8_t>bytes){
    std::uint16_t crc=0xffff;
    for(auto b:bytes){crc^=b;for(int i=0;i<8;++i)crc=(crc>>1)^((crc&1)?0x8408:0);}
    std::uint16_t r=0;for(int i=0;i<16;++i){r=std::uint16_t((r<<1)|(crc&1));crc>>=1;}return r;
}
inline bool valid_configuration(std::span<const std::uint8_t, configuration_size> configuration) {
    return state_crc(configuration.first(configuration_size - 2)) ==
           read_state_u16(configuration.data() + configuration_size - 2);
}
inline bool has_state_header(std::span<const std::uint8_t> state) {
    return state.size() == state_size &&
           std::equal(state_header.begin(), state_header.end(), state.begin());
}
inline bool valid_state(std::span<const std::uint8_t> state) {
    return has_state_header(state) &&
           valid_configuration(std::span<const std::uint8_t, configuration_size>(
                   state.data() + state_header_size, configuration_size));
}
inline void update_configuration_checksum(std::span<std::uint8_t, configuration_size> configuration) {
    const auto crc = state_crc(configuration.first(configuration_size - 2));
    write_state_u16(configuration.data() + configuration_size - 2, crc);
}
struct PersistedHistory {std::uint16_t glucose,quality,minute;float filtered_glucose;};
struct PersistedResult {
    std::uint16_t minute,glucose,quality,secondary_glucose;std::int16_t rate_hundredths;
    std::uint16_t history_count;float filtered_glucose,filtered_slope;
    std::uint16_t rate_quality;
    std::uint32_t trend,range_category;std::uint16_t temperature_tenths;
    std::array<PersistedHistory,31>history;
};
inline PersistedResult read_persisted_result(std::span<const std::uint8_t,configuration_size> configuration){
    const auto*p=configuration.data()+configuration_result_offset;PersistedResult r{};
    r.minute=read_state_u16(p);r.glucose=read_state_u16(p+2);r.quality=read_state_u16(p+4);r.secondary_glucose=read_state_u16(p+6);
    r.rate_hundredths=std::bit_cast<std::int16_t>(read_state_u16(p+8));r.history_count=read_state_u16(p+10);
    r.rate_quality=read_state_u16(p+14);
    r.filtered_glucose=read_state_float(p+16);r.filtered_slope=read_state_float(p+20);
    r.trend=read_state_u32(p+32);r.range_category=read_state_u32(p+36);r.temperature_tenths=read_state_u16(p+46);
    for(std::size_t i=0;i<31;++i){const auto*h=p+48+10*i;r.history[i]={read_state_u16(h),read_state_u16(h+2),read_state_u16(h+4),read_state_float(h+6)};}return r;
}
inline PersistedResult read_persisted_result(std::span<const std::uint8_t,state_size> state) {
    return read_persisted_result(state.subspan<state_header_size, configuration_size>());
}
// Insert the finalized configuration into the existing MESS envelope.
inline PersistentState replace_configuration(std::span<const std::uint8_t, state_size> previous,
        std::span<const std::uint8_t, configuration_size> configuration) {
    PersistentState out;
    std::copy(previous.begin(), previous.end(), out.begin());
    std::copy(configuration.begin(), configuration.end(), out.begin() + state_header_size);
    return out;
}
} // namespace libre2_processing
