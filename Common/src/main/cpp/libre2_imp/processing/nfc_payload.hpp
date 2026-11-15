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
// Decrypted 344-byte NFC payload and raw-cache ordering. Encryption and the
// activation protocol are separate. See the cross-sensor verification.
#include "recovered_components.hpp"
#include "persistent_state.hpp"
namespace libre2_processing {
enum class NfcRecordFormat { libre2, libre1 };
struct NfcPayload {
    std::array<std::uint16_t,80> trend{};
    std::array<std::uint16_t,160> history{};
    FactoryMetadata factory{};
    std::uint16_t fram_age{},footer_age{};
    std::uint8_t phase{},trend_pointer{},history_pointer{};
    bool checksums_valid{},pointers_valid{};

    bool readable() const {return phase>=2&&phase<=4;}
    bool final_readings() const {return footer_age&&fram_age>=footer_age;}
    std::uint16_t reading_age() const {return final_readings()?footer_age:fram_age;}
    std::uint32_t reading_time(std::uint32_t now) const {
        return now-std::min(now,60u*(fram_age-reading_age()));
    }
};
// The ring points to the next write slot. The age word can lead it by a
// minute while NFC reads span an update. Retain the latest matching minute.
inline std::uint16_t nfc_processing_minute(std::uint16_t fram_age,std::uint8_t trend_pointer){
    return std::uint16_t(fram_age-((unsigned(fram_age)-unsigned(trend_pointer))&15u));
}
struct NfcRawCache {std::array<std::uint16_t,80>trend{};std::array<std::uint16_t,160>history{};};
// Serialized native raw cache: young rings contain only completed records at
// the beginning, with zero-filled unused tail. Working-window shifts happen
// afterwards. This operates on an active decrypted payload, before expiry.
inline NfcRawCache nfc_raw_cache(const NfcPayload& payload){
    NfcRawCache out{};const unsigned minute=nfc_processing_minute(payload.reading_age(),payload.trend_pointer);
    const unsigned nt=std::min(minute,16u),nh=std::min(minute<2?0u:(minute-2)/15u,32u);
    const unsigned tstart=minute<16?(16u-payload.trend_pointer)%16u:0u;
    const unsigned hstart=minute<482?(32u-payload.history_pointer)%32u:0u;
    for(unsigned i=0;i<nt;++i)for(unsigned j=0;j<5;++j)out.trend[5*i+j]=payload.trend[5*((tstart+i)%16)+j];
    for(unsigned i=0;i<nh;++i)for(unsigned j=0;j<5;++j)out.history[5*i+j]=payload.history[5*((hstart+i)%32)+j];
    return out;
}
inline std::array<std::uint16_t,5> unpack_nfc_record(
        std::span<const std::uint8_t,6> bytes,std::uint16_t thermistor_reference,NfcRecordFormat format=NfcRecordFormat::libre2){
    std::uint64_t bits=0;
    for(unsigned i=0;i<6;++i)bits|=std::uint64_t(bytes[i])<<(8*i);
    const int delta=int((bits>>38)&0x1ff);
    const int denominator=int(thermistor_reference)+((bits>>47)?-delta:delta);
    const auto high=std::uint16_t((bits>>14)&0xc00),low=std::uint16_t((bits>>14)&0x3ff);
    const bool legacy=format==NfcRecordFormat::libre1;
    return {std::uint16_t(bits&0x3fff),std::uint16_t((bits>>26)&0xfff),
        std::uint16_t(legacy?(high?high:low):high),std::uint16_t(denominator),
        std::uint16_t(legacy?(high?low&0x1ff:0):low)};
}
inline NfcPayload decode_nfc_payload(std::span<const std::uint8_t,344> fram,NfcRecordFormat format=NfcRecordFormat::libre2){
    NfcPayload out{};out.factory=decode_factory_metadata(fram);
    out.fram_age=read_state_u16(fram.data()+316);
    out.footer_age=read_state_u16(fram.data()+0x146);
    out.phase=fram[4];
    out.trend_pointer=fram[26];out.history_pointer=fram[27];
    // The footer caps the age of the records even while the sensor clock
    // continues. Align the trend ring against that cap after wear ends.
    const unsigned age=out.reading_age();
    const unsigned lag=(age-out.trend_pointer)&15u;
    out.pointers_valid=out.trend_pointer<16&&out.history_pointer<32&&lag<=std::min(age,5u);
    out.checksums_valid=true;
    constexpr unsigned starts[]{0,24,320},ends[]{24,320,344};
    for(unsigned i=0;i<3;++i)out.checksums_valid&=
        state_crc(fram.subspan(starts[i]+2,ends[i]-starts[i]-2))==read_state_u16(fram.data()+starts[i]);
    if(!out.pointers_valid)return out;
    auto unpack=[&](unsigned start,unsigned count,unsigned pointer,auto& records){
        for(unsigned i=0;i<count;++i){
            const auto offset=start+6*((pointer+i)%count);
            const auto record=unpack_nfc_record(std::span<const std::uint8_t,6>(fram.data()+offset,6),out.factory.references.temperature_reference,format);
            std::copy(record.begin(),record.end(),records.begin()+5*i);
        }
    };
    unpack(28,16,out.trend_pointer,out.trend);
    unpack(124,32,out.history_pointer,out.history);
    return out;
}
} // namespace libre2_processing
