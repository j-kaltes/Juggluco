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
#include "nfc_processing.hpp"
namespace libre2_processing {
inline std::array<std::uint16_t,5> unpack_ble_record(std::uint32_t bits,std::uint16_t thermistor){
    const auto adc=std::uint16_t(bits&0x3fff),temperature=std::uint16_t((bits>>14)&0xfff);
    if(!adc)return {0,0,0,0,temperature?temperature:std::uint16_t(0x8000)};
    const int delta=int((bits>>26)&31);
    return {adc,temperature,0,std::uint16_t(int(thermistor)+((bits>>31)?-delta:delta)),0};
}
// Merge the seven sparse trend and three historical BLE records by sensor
// minute. Preserve the prior cache's unused tail, including its initial 8000
// records; an NFC-filled young cache may instead have zero-filled tails.
inline Configuration apply_ble_payload(Configuration c,std::span<const std::uint8_t,44>plain,
        std::uint32_t algorithm_timestamp){
    const unsigned minute=read_state_u16(plain.data()+40),old_minute=read_state_u16(c.data()+0x10);
    const unsigned nt=std::min(minute,16u);
    const int tf=minute<16?1:int(minute)-15,old_tf=old_minute<16?1:int(old_minute)-15;
    const unsigned last=completed_history_index(std::uint16_t(minute)),old_last=completed_history_index(std::uint16_t(old_minute));
    const unsigned nh=std::min(last,32u);
    const int hf=std::max(1,int(last)-31),old_hf=std::max(1,int(old_last)-31);
    std::array<std::uint16_t,80>trend{};std::array<std::uint16_t,160>history{};
    const bool reuse_cache=(c[processing_flags_offset]&packet_samples_only)==0;
    auto merge=[&](auto&out,unsigned n,unsigned old_n,int first,int old_first,unsigned offset){
        for(unsigned i=0;i<n;++i){
            const int source=first+int(i)-old_first;
            if(reuse_cache&&source>=0&&unsigned(source)<old_n)for(unsigned j=0;j<5;++j)out[5*i+j]=read_state_u16(c.data()+offset+10*unsigned(source)+2*j);
            else out[5*i+4]=0x8000;
        }
    };
    merge(trend,16,16,tf,old_tf,0x194);merge(history,32,32,hf,old_hf,0x234);
    constexpr int lags[]{0,2,4,6,7,12,15};const auto thermistor=read_state_u16(c.data()+0x186);
    for(unsigned i=0;i<10;++i){
        const auto record=unpack_ble_record(read_state_u32(plain.data()+4*i),thermistor);
        const int index=i<7?int(minute)-lags[i]-tf:int(last)-int(i-7)-hf;
        if(index>=0&&unsigned(index)<(i<7?nt:nh)){
            if(i<7)std::copy(record.begin(),record.end(),trend.begin()+5*index);
            else std::copy(record.begin(),record.end(),history.begin()+5*index);
        }
    }
    write_processing_u32(c.data()+4,algorithm_timestamp);write_state_u16(c.data()+0xe,std::uint16_t(minute));write_state_u16(c.data()+0x10,std::uint16_t(minute));
    for(unsigned i=0;i<80;++i)write_state_u16(c.data()+0x194+2*i,trend[i]);
    for(unsigned i=0;i<160;++i)write_state_u16(c.data()+0x234+2*i,history[i]);return c;
}
// Advance the portable calibration block. Invalid input leaves it unchanged.
inline bool calibrate_ble_payload(Configuration &configuration,
        std::span<const std::uint8_t,44> plain, std::uint32_t algorithm_timestamp,
        NumericResult &numeric) {
    if (!valid_configuration(configuration) ||
        state_crc(plain.first(42)) != read_state_u16(plain.data() + 42)) {
        return false;
    }
    auto next = apply_ble_payload(configuration, plain, algorithm_timestamp);
    next[processing_flags_offset] |= delayed_history;
    const auto inputs = prepare_decoded_inputs(next, false);
    if (inputs.parameters.age < inputs.previous.minute ||
        inputs.parameters.elapsed_minutes > inputs.parameters.wear_minutes) {
        return false;
    }
    numeric = evaluate_numeric(inputs.trend, inputs.history, inputs.parameters, inputs.previous);
    write_numeric_configuration(next, inputs.parameters, numeric);
    configuration = next;
    return true;
}
inline bool calibrate_plain_ble_frame(Configuration &configuration,
        std::span<const std::uint8_t,46> packet, std::uint32_t algorithm_timestamp,
        NumericResult &numeric) {
    return calibrate_ble_payload(configuration, packet.subspan<2,44>(), algorithm_timestamp, numeric);
}
inline NfcProcessingResult process_active_ble_payload(std::span<const std::uint8_t,44> plain,
        std::span<const std::uint8_t,state_size> previous, std::uint32_t algorithm_timestamp) {
    NfcProcessingResult out{};
    if (!has_state_header(previous)) return out;
    auto configuration = configuration_from_state(previous);
    if (!calibrate_ble_payload(configuration, plain, algorithm_timestamp, out.numeric)) return out;
    out.state = replace_configuration(previous, configuration);
    out.accepted = true;
    return out;
}
inline NfcProcessingResult process_active_ble_plain(std::span<const std::uint8_t,46>packet,
        std::span<const std::uint8_t,state_size>previous,std::uint32_t algorithm_timestamp){
    return process_active_ble_payload(std::span<const std::uint8_t,44>(packet.data()+2,44),previous,algorithm_timestamp);
}
inline NfcProcessingResult process_active_ble_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::span<const std::uint8_t,46>packet,
        std::span<const std::uint8_t,state_size>previous,std::uint32_t algorithm_timestamp){
    if(!classic_payload_cipher(info[2]))return {};
    const auto plain=decrypt_ble_eu(uid,packet);if(!plain.valid)return {};
    return process_active_ble_payload(plain.bytes,previous,algorithm_timestamp);
}
inline NfcProcessingResult process_encrypted_nfc_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::span<const std::uint8_t,344>encrypted,
        std::span<const std::uint8_t,state_size>previous,std::uint32_t algorithm_timestamp){
    const auto plain=decrypt_nfc_eu(uid,info,encrypted);
    if(!plain.valid)return {};return process_active_nfc(plain.bytes,previous,algorithm_timestamp);
}
} // namespace libre2_processing
