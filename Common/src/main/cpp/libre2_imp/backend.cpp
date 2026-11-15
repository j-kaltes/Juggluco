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
/*      Sun Nov 15 11:43:27 CET 2026                                                 */

#include "backend.hpp"
#include "processing/activation.hpp"
#include "processing/ble_processing.hpp"
#include "jnisub.hpp"
#include "jnisubin.hpp"
#include <cstring>
#include <memory>
#include <span>
#include <cmath>
#include <limits>
namespace libre2_imp {
namespace ar=libre2_processing;
namespace {
using View=std::span<const std::uint8_t>;
View bytes(const data_t *a){
    if(!a||a->size()<=0)return {};
    return {reinterpret_cast<const std::uint8_t*>(a->data()),std::size_t(a->size())};
}
bool compatible_state(View state,unsigned nominal_wear){
    if(!ar::valid_state(state))return false;
    const auto *config=state.data()+ar::state_header_size;
    // Processing flags use the same layout across saved states.
    return ar::read_state_u32(config)!=0&&ar::read_state_u16(config+0x14)==nominal_wear;
}
template<std::size_t N> bool read_array(JNIEnv *env,jbyteArray a,std::array<std::uint8_t,N>&out){
    if(!a||env->GetArrayLength(a)!=N)return false;
    env->GetByteArrayRegion(a,0,N,reinterpret_cast<jbyte*>(out.data()));return !env->ExceptionCheck();
}
template<std::size_t N> jbyteArray array(JNIEnv *env,const std::array<std::uint8_t,N>&a){
    auto out=env->NewByteArray(N);
    if(out)env->SetByteArrayRegion(out,0,N,reinterpret_cast<const jbyte*>(a.data()));
    return out;
}
bool classic_ble_identity(JNIEnv *env,jbyteArray uid,jbyteArray patch,ar::SensorUid&u,ar::PatchInfo&p){
    return read_array(env,uid,u)&&read_array(env,patch,p)&&ar::classic_ble_patch(p);
}
bool identity(const data_t *uid,const data_t *info){return bytes(uid).size()==8&&bytes(info).size()==6;}
void restore_activation_start(View old,ar::PersistentState &state,unsigned start){
    // Legacy FAKE states can use a start shifted by 24 hours. Keep their
    // calibration and cached readings, and restore the database's start.
    std::copy(old.begin(),old.end(),state.begin());
    auto config=std::span(state).subspan<ar::state_header_size,ar::configuration_size>();
    ar::write_processing_u32(config.data(),start);
    ar::update_configuration_checksum(config);
}
void save(scanstate *next,const ar::PersistentState&state){
    auto *out=next->alloc(state.size());std::memcpy(out->data(),state.data(),state.size());next->setpos(MESS,out);
    next->makenull(COMP);next->makenull(ATTE);next->makenull(FAKE);
}
enum class ResultKind { scan, stream, initial };
AlgorithmResults *results(const ar::PersistentState&state,ResultKind kind=ResultKind::scan){
    auto h=new jniHistory;const auto r=ar::read_persisted_result(state);
    const bool initial=kind==ResultKind::initial;
    const unsigned count=initial?0u:std::min<unsigned>(r.history_count,r.history.size());
    if(count){
        const bool pending=kind==ResultKind::stream;
        h->reserve(count+pending);
        for(unsigned i=0;i<count;++i){const auto &v=r.history[i];h->push_back(new GlucoseValue{v.minute,v.quality,v.glucose});}
        // The native stream list includes the pending quarter-hour after the
        // persisted entries. Juggluco saves the preceding entry at len-2.
        if(pending)h->push_back(new GlucoseValue{r.history[count-1].minute+15,0x8000,0});
    }
    GlucoseValue value{initial?0:r.minute,initial?0x8000:r.quality,initial?0:r.glucose};
    // The public native result uses hundredths, with zero for a missing rate.
    const float rate=initial||r.rate_quality?0.f:float(r.rate_hundredths)/100.f;
    return new AlgorithmResults{.hist=h,.glu=new GlucoseNow(&value,initial?0:r.trend,rate),
        .alarm=initial?0:int(r.range_category),.isActionable=JNI_FALSE,.esaMinutesToWait=0,.lsaDetected=JNI_FALSE,.inserted=true,.removed=false};
}
}
bool bluetooth_supported(const data_t *info){
    const auto patch=bytes(info);
    return patch.size()==6&&ar::libre_security_generation(patch[2])!=0;
}
bool needs_activation(const data_t *fram){
    const auto raw=bytes(fram);
    if(raw.size()!=344||raw[4]!=1)return false;
    const auto decoded=ar::decode_nfc_payload(raw.first<344>());
    return decoded.checksums_valid&&decoded.pointers_valid&&decoded.fram_age==0;
}
bool usable_state(scanstate *state,const data_t *info){
    const auto patch=bytes(info);
    return state&&bluetooth_supported(info)&&
        compatible_state(bytes(state->get(MESS)),ar::patch_metadata(patch.first<6>()).wear_minutes);
}
AlgorithmResults *scan(const data_t *uid,const data_t *info,data_t *data, scanstate *previous,scanstate *next,unsigned now,outobj *start,outobj *end,int warmup,unsigned known_start){
    if(!identity(uid,info)||bytes(data).size()!=344||!next)return nullptr;
    const auto u=std::span<const std::uint8_t,8>(bytes(uid).data(),8);
    const auto p=std::span<const std::uint8_t,6>(bytes(info).data(),6);
    // processScan also returns decrypted FRAM in place. Juggluco subsequently
    // reads this buffer for sensor age, activation, raw history and logging.
    const auto generation=ar::libre_security_generation(p[2]);
    const auto format=generation==0?ar::NfcRecordFormat::libre1:ar::NfcRecordFormat::libre2;
    auto plain=bytes(data).first<344>();
    ar::Fram decrypted;
    // An authenticated patch can still carry an inner legacy ciphertext.
    if(ar::classic_payload_cipher(p[2])){
        decrypted=ar::crypt_nfc(u,p,plain);
        plain=decrypted;
    }else if(ar::encrypted_scan_only(p[2])){
        decrypted=ar::crypt_nfc(u,p,plain,ar::NfcCipher::libre_us14day);
        plain=decrypted;
    }
    const auto decoded=ar::decode_nfc_payload(plain,format);
    if(!decoded.checksums_valid||!decoded.pointers_valid)return nullptr;
    if(plain.data()==decrypted.data())std::memcpy(data->data(),plain.data(),plain.size());
    if(!decoded.readable())return nullptr;
    const auto old=bytes(previous?previous->get(MESS):nullptr);
    ar::ActivationOptions options{};
    options.warmup_minutes=std::uint16_t(warmup);
    options.wear_minutes=std::uint16_t(ar::patch_metadata(p).wear_minutes);
    if(old.empty()&&!known_start){
        const auto initial=ar::make_initial_state_eu(plain,options);if(!initial.accepted)return nullptr;
        start->ptr=reinterpret_cast<void*>(std::uintptr_t(ar::estimate_activation_start_eu(plain,now)));
        end->ptr=reinterpret_cast<void*>(std::uintptr_t(decoded.reading_time(now)));save(next,initial.state);return results(initial.state,ResultKind::initial);
    }
    ar::PersistentState migrated;
    auto active_state=old;
    if(!compatible_state(old,options.wear_minutes)){
        // Rebuild an unknown old state only from verified NFC factory data and
        // the sensor start already saved by Juggluco; never interpret its bytes.
        if(!known_start||now<known_start)return nullptr;
        const auto initial=ar::make_initial_state_eu(plain,options);
        if(!initial.accepted)return nullptr;
        const auto active=ar::activation_complete_eu(initial.state,ar::activation_day_from_start(known_start),known_start);
        if(!active.accepted)return nullptr;
        migrated=active.state;
        active_state=migrated;
    }else if(previous->datpos(FAKE)){
        if(!known_start)return nullptr;
        restore_activation_start(old,migrated,known_start);
        active_state=migrated;
    }
    const auto state=std::span<const std::uint8_t,ar::state_size>(active_state.data(),ar::state_size);
    const auto processed=ar::process_active_nfc(plain,state,now,format);if(!processed.accepted)return nullptr;
    start->ptr=reinterpret_cast<void*>(std::uintptr_t(ar::read_state_u32(processed.state.data()+ar::state_header_size)));
    end->ptr=reinterpret_cast<void*>(std::uintptr_t(decoded.reading_time(now)));
    save(next,processed.state);
    auto *result = results(processed.state);
    const auto &numeric = processed.numeric;

    if (format == ar::NfcRecordFormat::libre2 &&
        numeric.glucose && numeric.quality == 0) {
        auto &points = result->scanTrendMgL.emplace();

        for (std::size_t i = 0; i < points.size(); ++i) {
            const float value = 10.0f * numeric.trend_glucose[i];
            if (numeric.trend_raw[5 * i + 4] == 0 &&
                value >= 0.5f &&
                value <= std::numeric_limits<std::int16_t>::max()) {
                points[i] = static_cast<std::uint16_t>(std::lround(value));
            }
        }
    }
    return result;
}
AlgorithmResults *stream(const data_t *uid,const data_t *info,const data_t *packet,
        scanstate *previous,scanstate *next,unsigned now,unsigned known_start,bool plaintext){
    if(!identity(uid,info)||!bluetooth_supported(info)||bytes(packet).size()!=46||!previous||!next)return nullptr;
    const auto patch=bytes(info).first<6>();
    auto old=bytes(previous->get(MESS));if(!compatible_state(old,ar::patch_metadata(patch).wear_minutes))return nullptr;
    std::unique_ptr<ar::PersistentState> migrated;
    if(previous->datpos(FAKE)){
        if(!known_start)return nullptr;
        migrated=std::make_unique_for_overwrite<ar::PersistentState>();
        restore_activation_start(old,*migrated,known_start);
        old=*migrated;
    }
    const auto frame=bytes(packet).first<46>();
    const auto state=old.first<ar::state_size>();
    const bool classic=ar::classic_payload_cipher(patch[2]) &&
        (ar::libre_security_generation(patch[2])==1 ||
         ar::state_crc(frame.subspan<2,42>())!=ar::read_state_u16(frame.data()+44));
    const auto processed=!plaintext&&classic?
        ar::process_active_ble_eu(bytes(uid).first<8>(),patch,frame,state,now):
        ar::process_active_ble_plain(frame,state,now);
    if(!processed.accepted)return nullptr;
    save(next,processed.state);
    return results(processed.state,ResultKind::stream);
}
jint family(JNIEnv *env,jobject,jint,jbyteArray patch,jbyteArray){
    ar::PatchInfo p{};if(!read_array(env,patch,p))return 0;
    return ar::patch_metadata(p).family;
}
jboolean times(JNIEnv *env,jobject,jint,jbyteArray patch,jobject warmup,jobject wear,jbyteArray){
    ar::PatchInfo p{};if(!read_array(env,patch,p))return JNI_FALSE;
    const auto timing=ar::patch_metadata(p);
    reinterpret_cast<outobj*>(warmup)->ptr=reinterpret_cast<void*>(std::uintptr_t(timing.warmup_minutes));
    reinterpret_cast<outobj*>(wear)->ptr=reinterpret_cast<void*>(std::uintptr_t(timing.wear_minutes));return JNI_TRUE;
}
jbyte activation_command(JNIEnv *env,jobject,jint,jbyteArray patch,jbyteArray){
    ar::PatchInfo p{};if(!read_array(env,patch,p))return 0;
    return jbyte(ar::patch_metadata(p).activation_command);
}
jbyteArray activation_payload(JNIEnv *env,jobject,jint,jbyteArray uid,jbyteArray patch,jbyte,jbyteArray){
    ar::SensorUid u{};ar::PatchInfo p{};if(!read_array(env,uid,u)||!read_array(env,patch,p))return nullptr;
    if(ar::patch_metadata(p).activation_command==0xa1)return array(env,ar::activation_payload_eu(u));
    return array(env,std::array<std::uint8_t,4>{0xc2,0xad,0x75,0x21});
}
jbyteArray enable_payload(JNIEnv *env,jobject,jint,jbyteArray uid,jbyteArray patch,jbyte,jint code,jbyteArray){
    ar::SensorUid u{};ar::PatchInfo p{};if(!classic_ble_identity(env,uid,patch,u,p))return nullptr;
    return array(env,ar::enable_streaming_payload_eu(u,p,std::uint32_t(code)));
}
jbyteArray unlock_payload(JNIEnv *env,jobject,jint,jbyteArray uid,jbyteArray patch,jbyte,jint code,jint count,jbyteArray){
    ar::SensorUid u{};ar::PatchInfo p{};if(!classic_ble_identity(env,uid,patch,u,p))return nullptr;
    return array(env,ar::streaming_unlock_payload_eu(u,p,std::uint32_t(code),std::uint32_t(count)));
}
jbyteArray activate_state(JNIEnv *env,jobject,jbyteArray uid,jint day,jint start,jbyteArray mess,jbyteArray){
    ar::SensorUid u{};ar::PersistentState old{};if(!read_array(env,uid,u)||!read_array(env,mess,old))return nullptr;
    const auto r=ar::activation_complete_eu(old,day,std::uint32_t(start));return r.accepted?array(env,r.state):nullptr;
}
}
