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
// Legacy Libre NFC and BLE cipher, composed with the recovered native pipeline.
// Prior public implementations: ivalkou/LibreTools and gui-dos/DiaBLE Libre2.
#include "nfc_payload.hpp"
namespace libre2_processing {
using SensorUid=std::array<std::uint8_t,8>;
using PatchInfo=std::array<std::uint8_t,6>;
using Fram=std::array<std::uint8_t,344>;
using CryptoWords=std::array<std::uint16_t,4>;
inline bool classic_ble_patch(std::span<const std::uint8_t,6>info){return libre_security_generation(info[2])==1;}
inline std::uint16_t eu_shift(std::uint16_t v){
    return std::uint16_t((v>>2)^((v&1)?0x6860:0)^((v&2)?0xa0c5:0));
}
inline CryptoWords eu_crypto_round(CryptoWords v){
    std::array<std::uint16_t,8>r{};
    for(unsigned i=0;i<4;++i)r[i]=eu_shift(i?r[i-1]:v[0])^v[3-i];
    r[4]=eu_shift(r[3]);
    for(unsigned i=5;i<8;++i)r[i]=eu_shift(r[i-1]^r[i-5]);
    return {std::uint16_t(r[3]^r[7]),std::uint16_t(r[2]^r[6]),
            std::uint16_t(r[1]^r[5]),std::uint16_t(r[0]^r[4])};
}
inline CryptoWords eu_crypto_seed(std::span<const std::uint8_t,8>uid,std::uint16_t x,std::uint16_t y){
    return {std::uint16_t(read_state_u16(uid.data()+4)+x+y),read_state_u16(uid.data()+2),
            std::uint16_t(read_state_u16(uid.data())+2*x),std::uint16_t(0x241a^0x14c6)};
}
inline CryptoWords eu_crypto_seed4(std::span<const std::uint8_t,8>uid,CryptoWords v){
    return {std::uint16_t(read_state_u16(uid.data()+4)+v[0]),
            std::uint16_t(read_state_u16(uid.data()+2)+v[1]),
            std::uint16_t(read_state_u16(uid.data())+v[2]),std::uint16_t(v[3]+0x14c6)};
}
inline std::array<std::uint8_t,8> eu_word_bytes(CryptoWords words){
    std::array<std::uint8_t,8>out{};
    for(unsigned i=0;i<4;++i)write_state_u16(out.data()+2*i,words[i]);return out;
}
inline std::array<std::uint8_t,4> eu_command_signature(std::span<const std::uint8_t,8>uid,
        std::uint16_t command,std::uint16_t argument){
    const auto w=eu_crypto_round(eu_crypto_seed(uid,command,argument));
    std::array<std::uint8_t,4>out{};write_state_u16(out.data(),w[0]^0x4163);write_state_u16(out.data()+2,w[1]^0x4344);return out;
}
enum class NfcCipher { libre2, libre_us14day };
// The US scan-only sensor uses the same transform with a fixed header and
// footer key and the unmodified patch word for the measurement blocks.
inline Fram crypt_nfc(std::span<const std::uint8_t,8>uid,std::span<const std::uint8_t,6>info,
        std::span<const std::uint8_t,344>input,NfcCipher cipher=NfcCipher::libre2){
    Fram out{};
    const bool us14day=cipher==NfcCipher::libre_us14day;
    const auto patch_word=read_state_u16(info.data()+4);
    const auto body_argument=std::uint16_t(us14day?patch_word:patch_word^0x44);
    for(unsigned block=0;block<43;++block){
        const auto argument=std::uint16_t(us14day&&(block<3||block>=40)?0xcadc:body_argument);
        const auto key=eu_word_bytes(eu_crypto_round(eu_crypto_seed(uid,std::uint16_t(block),argument)));
        for(unsigned j=0;j<8;++j)out[8*block+j]=input[8*block+j]^key[j];
    }
    return out;
}
struct DecryptedFram {bool valid{};Fram bytes{};};
inline DecryptedFram decrypt_nfc_eu(std::span<const std::uint8_t,8>uid,std::span<const std::uint8_t,6>info,
        std::span<const std::uint8_t,344>input){
    DecryptedFram out{};if(!classic_payload_cipher(info[2]))return out;out.bytes=crypt_nfc(uid,info,input);
    const auto p=decode_nfc_payload(out.bytes);out.valid=p.checksums_valid&&p.pointers_valid;return out;
}
inline std::array<std::uint8_t,5> activation_payload_eu(std::span<const std::uint8_t,8>uid){
    const auto signature=eu_command_signature(uid,0x1b,0x1b6a);
    return {0x1b,signature[0],signature[1],signature[2],signature[3]};
}
inline std::array<std::uint8_t,9> enable_streaming_payload_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::uint32_t unlock_code){
    std::array<std::uint8_t,9>out{};out[0]=0x1e;
    write_state_u16(out.data()+1,std::uint16_t(unlock_code));write_state_u16(out.data()+3,std::uint16_t(unlock_code>>16));
    const auto signature=eu_command_signature(uid,0x1e,std::uint16_t(unlock_code)^read_state_u16(info.data()+4));
    std::copy(signature.begin(),signature.end(),out.begin()+5);return out;
}
inline std::array<std::uint8_t,12> streaming_unlock_payload_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,6>info,std::uint32_t unlock_code,std::uint32_t unlock_count){
    const auto activation=eu_command_signature(uid,0x1b,0x1b6a);
    const auto enable=eu_command_signature(uid,0x1e,std::uint16_t(unlock_code)^read_state_u16(info.data()+4));
    // Native 0x1a7f84 narrows the connection counter to uint16 first.
    std::array<std::uint8_t,12>out{};const auto time=unlock_code+std::uint16_t(unlock_count);
    write_state_u16(out.data(),std::uint16_t(time));write_state_u16(out.data()+2,std::uint16_t(time>>16));
    const auto intermediate=eu_word_bytes(eu_crypto_round(eu_crypto_seed4(uid,
        {std::uint16_t(read_state_u16(enable.data())^std::uint16_t(time>>16)),read_state_u16(activation.data()),
         std::uint16_t(read_state_u16(enable.data()+2)^std::uint16_t(time)),read_state_u16(activation.data()+2)})));
    std::array<std::uint8_t,10>a{0xc1,0xc4,0xc3,0xc0,0xd4,0xe1,0xe7,0xba,intermediate[0],intermediate[1]};
    std::array<std::uint8_t,6>b{activation[0],activation[1],activation[2],activation[3],enable[0],enable[1]};
    std::array<std::uint8_t,6>c{enable[2],enable[3],out[0],out[1],out[2],out[3]};
    const CryptoWords checksums{state_crc(a),state_crc(std::span<const std::uint8_t>(intermediate).subspan(2,6)),state_crc(b),state_crc(c)};
    const auto response=eu_word_bytes(eu_crypto_round(eu_crypto_seed4(uid,checksums)));
    std::copy(response.begin(),response.end(),out.begin()+4);return out;
}
inline std::array<std::uint8_t,64> eu_ble_key(std::span<const std::uint8_t,8>uid,std::uint16_t nonce){
    const auto signature=eu_command_signature(uid,0x1b,0x1b6a);
    const auto x=std::uint16_t((read_state_u16(signature.data())^read_state_u16(signature.data()+2))|0x63);
    auto words=eu_crypto_round(eu_crypto_seed(uid,x,nonce^0x63));std::array<std::uint8_t,64>out{};
    for(unsigned block=0;block<8;++block){const auto bytes=eu_word_bytes(words);std::copy(bytes.begin(),bytes.end(),out.begin()+8*block);words=eu_crypto_round(words);}return out;
}
struct DecryptedBle {bool valid{};std::array<std::uint8_t,44>bytes{};};
inline DecryptedBle decrypt_ble_eu(std::span<const std::uint8_t,8>uid,std::span<const std::uint8_t,46>packet){
    DecryptedBle out{};const auto key=eu_ble_key(uid,read_state_u16(packet.data()));
    for(unsigned i=0;i<44;++i)out.bytes[i]=packet[i+2]^key[i];
    out.valid=state_crc(std::span<const std::uint8_t>(out.bytes).first(42))==read_state_u16(out.bytes.data()+42);return out;
}
inline std::array<std::uint8_t,46> encrypt_ble_eu(std::span<const std::uint8_t,8>uid,
        std::span<const std::uint8_t,44>plain,std::uint16_t nonce){
    std::array<std::uint8_t,46>out{};write_state_u16(out.data(),nonce);const auto key=eu_ble_key(uid,nonce);
    for(unsigned i=0;i<44;++i)out[i+2]=plain[i]^key[i];return out;
}
} // namespace libre2_processing
