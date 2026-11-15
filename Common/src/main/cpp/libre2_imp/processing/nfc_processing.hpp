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
#include "nfc_payload.hpp"
#include "decoded_inputs.hpp"
namespace libre2_processing {
inline void write_processing_u32(std::uint8_t*p,std::uint32_t value){write_state_u16(p,std::uint16_t(value));write_state_u16(p+2,std::uint16_t(value>>16));}
inline void write_processing_float(std::uint8_t*p,float value){write_processing_u32(p,std::bit_cast<std::uint32_t>(value));}
inline Configuration configuration_from_state(std::span<const std::uint8_t,state_size>state){
    Configuration c;std::copy_n(state.begin()+state_header_size,configuration_size,c.begin());return c;
}
inline Configuration apply_nfc_payload(Configuration c,const NfcPayload& payload,std::uint32_t algorithm_timestamp){
    const auto minute=nfc_processing_minute(payload.reading_age(),payload.trend_pointer);const auto raw=nfc_raw_cache(payload);
    write_processing_u32(c.data()+4,algorithm_timestamp);write_state_u16(c.data()+0xe,payload.fram_age);write_state_u16(c.data()+0x10,minute);
    write_state_u16(c.data()+0x180,std::uint16_t(payload.factory.references.upper));write_state_u16(c.data()+0x182,std::uint16_t(payload.factory.references.lower));
    const auto coefficients=calculate_factory_coefficients(payload.factory.index,read_state_u16(c.data()+12),payload.factory.correction_code);
    write_processing_float(c.data()+0x188,coefficients.offset);write_processing_float(c.data()+0x18c,coefficients.divisor);
    c[0x190]=payload.factory.correction_code;c[0x191]=payload.factory.average_delta;c[0x192]=payload.factory.peak_delta;
    for(unsigned i=0;i<80;++i)write_state_u16(c.data()+0x194+2*i,raw.trend[i]);
    for(unsigned i=0;i<160;++i)write_state_u16(c.data()+0x234+2*i,raw.history[i]);
    return c;
}
inline void write_numeric_configuration(Configuration &configuration,
        const NumericParameters &parameters, const NumericResult &numeric) {
    for (unsigned i = 0; i < 3; ++i) {
        write_processing_float(configuration.data() + 0x34 + 4 * i, numeric.reference[i]);
    }
    if (!numeric.input_rejected) {
        cache_new_factory_history(configuration, numeric.history_glucose.data(),
                numeric.history_source_status.data(), parameters.history_first_index,
                parameters.history_last_valid_index);
    }

    // Persist the result directly instead of building the native work area.
    auto *result = configuration.data() + configuration_result_offset;
    std::fill_n(result, persisted_result_size, 0);
    const auto word = [result](unsigned offset, std::uint16_t value) {
        write_state_u16(result + offset, value);
    };
    word(0, parameters.age);
    word(2, numeric.glucose);
    word(4, numeric.quality);
    word(6, numeric.secondary_glucose);
    word(8, std::uint16_t(numeric.rate_hundredths));
    word(10, numeric.history_count);
    if(parameters.is_scan) {
        const auto footer_age=read_state_u16(configuration.data()+0x184);
        const auto fram_age=read_state_u16(configuration.data()+0xe);
        if(footer_age&&fram_age>footer_age)word(12,fram_age-footer_age);
    }
    word(14, numeric.rate_quality);
    write_processing_float(result + 16, numeric.current.filtered_glucose);
    write_processing_float(result + 20, numeric.current.filtered_rate);
    write_processing_u32(result + 24, numeric.input_rejected ? 1u : 0u);
    write_processing_u32(result + 32, numeric.arrow);
    write_processing_u32(result + 36, numeric.range_category);
    write_processing_u32(result + 40, 1);
    word(46, numeric.temperature_tenths);
    for (unsigned i = 0; i < 31; ++i) {
        const auto &entry = numeric.state.history[i];
        const unsigned offset = 48 + 10 * i;
        word(offset, entry.glucose);
        word(offset + 2, entry.quality);
        word(offset + 4, entry.minute);
        write_processing_float(result + offset + 6, entry.filtered_glucose);
    }
    update_configuration_checksum(configuration);
}
struct NfcProcessingResult {
    bool accepted{};
    NumericResult numeric{};
    PersistentState state{};
};
// Active EU decrypted NFC path with an initialized state for this same sensor.
// Timestamp is the native algorithm's seconds since 2010-01-01.
inline NfcProcessingResult process_active_nfc(std::span<const std::uint8_t,344>fram,
        std::span<const std::uint8_t,state_size>previous,std::uint32_t algorithm_timestamp,NfcRecordFormat format=NfcRecordFormat::libre2){
    NfcProcessingResult out{};const auto payload=decode_nfc_payload(fram,format);
    if(!valid_state(previous)||!payload.checksums_valid||!payload.pointers_valid||!payload.readable())return out;
    auto c=configuration_from_state(previous);
    c=apply_nfc_payload(c,payload,algorithm_timestamp);
    // Give final NFC records the same recovery window as the old last-day
    // scan path. BLE still uses the 12-hour grace and MESS keeps nominal wear.
    constexpr unsigned final_scan_grace=24*60;
    const auto inputs=prepare_decoded_inputs(c,true,payload.final_readings()?final_scan_grace:wear_grace_minutes);
    if(inputs.parameters.age<inputs.previous.minute||
       inputs.parameters.elapsed_minutes>inputs.parameters.wear_minutes)return out;
    out.numeric=evaluate_numeric(inputs.trend,inputs.history,inputs.parameters,inputs.previous);
    write_numeric_configuration(c, inputs.parameters, out.numeric);
    out.state = replace_configuration(previous, c);
    out.accepted = true;
    return out;
}
} // namespace libre2_processing
