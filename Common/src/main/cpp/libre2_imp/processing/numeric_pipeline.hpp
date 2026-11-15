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
#include "temporal_current.hpp"
#include "temporal_history.hpp"
#include "reference_cache.hpp"
#include "input_window.hpp"
namespace libre2_processing {
// Inputs are unpacked/decrypted native records. nfc_payload.hpp unpacks a
// decrypted NFC ring; decoded_inputs.hpp supplies the recovered input windows.
// Encryption, activation protocol and lifecycle/JNI error wrappers remain
// separate layers. Reference-anchor refresh and compact factory-history cache
// updates are implemented below.
// Indices are the preprocessor's absolute 15-minute history indices.
struct NumericParameters {
    CalibrationParameters calibration;
    std::uint16_t age{},history_first_index{},history_last_valid_index{};
    std::uint16_t new_history_first_index{},history_source_offset{};
    std::uint8_t current_trend_index=15,current_history_index=31,feature_bits=14;
    bool is_scan=true;
    // Legacy version-0 states report history on the sensor's quarter hour.
    std::uint8_t history_output_delay_minutes=2;
    std::uint16_t maximum=500,minimum=40,low=70,high=240;
    // Elapsed minutes use the activation/system clock, distinct from age.
    // ffffffff retains the age-based assumption used by the old mature corpus.
    std::uint32_t elapsed_minutes=0xffffffffu;
    std::uint16_t wear_minutes=20160;
    std::uint16_t warmup_minutes=60;
    std::array<float,3>reference{4294967296.f,4294967296.f,4294967296.f};
    bool blend_previous_current=true;
};
struct NumericState {
    std::uint16_t minute{};
    float filtered_glucose{},filtered_rate{};
    std::uint16_t rate_quality{};
    std::array<PersistedHistory,31>history{};
};
struct NumericResult {
    std::array<std::uint16_t,80>trend_raw;
    std::array<std::uint16_t,160>history_raw;
    std::array<float,80>trend_temperature{};
    std::array<float,160>history_temperature{};
    std::array<float,16>trend_glucose{};
    std::array<float,32>history_glucose{},history_correction{};
    std::array<std::uint16_t,32>history_source_status{};
    std::array<TemporalSample,32>filtered_history{};
    std::array<float,3>reference{};
    float current_correction{};
    CurrentEstimates current{};
    float reported_glucose{},reported_rate{};
    std::uint16_t glucose{},secondary_glucose{};
    std::uint16_t history_count{};
    std::uint16_t quality{},rate_quality{};
    std::uint16_t temperature_tenths=999;
    bool input_rejected{};
    bool display_range_valid{};
    unsigned arrow{},range_category{};
    std::int16_t rate_hundredths{};
    NumericState state;
};
inline NumericResult evaluate_numeric(std::span<const std::uint16_t,80>trend,
        std::span<const std::uint16_t,160>history,const NumericParameters&p,const NumericState&previous){
    NumericResult r{};r.reference=p.reference;std::copy(trend.begin(),trend.end(),r.trend_raw.begin());
    r.history_count=std::uint16_t(std::min(31u,p.history_last_valid_index>1?unsigned(p.history_last_valid_index-1):0u));
    std::copy(history.begin(),history.end(),r.history_raw.begin());
    calibrate_raw_records(16,3,r.trend_temperature.data(),r.trend_raw.data(),r.trend_glucose.data(),p.calibration);
    calibrate_raw_records(32,1,r.history_temperature.data(),r.history_raw.data(),r.history_glucose.data(),p.calibration);
    std::array<std::uint16_t,16>tq{};std::array<std::uint16_t,32>hq{};
    for(int i=0;i<16;++i)tq[i]=r.trend_raw[5*i+4];
    for(int i=0;i<32;++i)hq[i]=r.history_source_status[i]=r.history_raw[5*i+4];
    const unsigned latest=p.history_last_valid_index>=p.history_first_index?p.history_last_valid_index-p.history_first_index:0;
    if(reject_history_changes(r.history_glucose.data(),hq.data(),latest)){
        r.input_rejected=true;r.quality=0x400;r.rate_quality=0x8000;r.history_count=0;
        r.state.minute=p.age;r.state.rate_quality=r.rate_quality;return r;
    }
    const unsigned new_first=p.new_history_first_index?p.new_history_first_index:p.history_first_index;
    r.reference=refresh_reference_anchors(r.history_glucose.data(),hq.data(),new_first,p.history_source_offset,p.reference);
    const float reference=reference_glucose(r.reference);const bool early=(p.feature_bits&4)!=0,late=(p.feature_bits&8)!=0;
    for(unsigned i=0;i<32&&p.history_first_index+i<=p.history_last_valid_index;++i){
        const auto index=static_cast<std::uint16_t>(p.history_first_index+i);const auto age=static_cast<std::uint16_t>(15*index);
        float correction=hq[i]?0.f:early_signal_correction(index,reference,r.history_glucose[i],early);
        correction=age_correction(age,early)+correction;
        r.history_correction[i]=late_age_correction(age,late)+correction;
    }
    float correction=0.f;
    if(latest<32&&!hq[latest])correction=early_signal_correction(p.history_last_valid_index,reference,r.history_glucose[latest],early);
    correction=age_correction(p.age,early)+correction;r.current_correction=late_age_correction(p.age,late)+correction;
    r.filtered_history=filter_calibrated_history(r.history_glucose.data(),hq.data());
    for(unsigned i=p.current_history_index+1;i<32;++i)r.filtered_history[i]={0.f,0x8000,0};
    // A successful historical fit consumes the temporary raw-record error.
    for(unsigned i=0;i<=p.current_history_index&&i<32;++i){
        if(!r.filtered_history[i].status)r.history_raw[5*i+4]=0;
        else r.history_raw[5*i+4]|=r.filtered_history[i].status;
    }
    const std::uint16_t gap=previous.minute?std::uint16_t(p.age-previous.minute):0xffff;
    r.current=combine_current_projections(r.trend_glucose.data(),tq.data(),r.filtered_history.data(),
        p.age,p.current_trend_index,p.current_history_index,p.blend_previous_current?previous.filtered_glucose:0.f,
        previous.filtered_rate,gap,r.current_correction,previous.rate_quality);
    const auto eligible=eligible_input_window(true,p.current_history_index,p.age,
        p.elapsed_minutes==0xffffffffu?p.age:p.elapsed_minutes,p.wear_minutes);
    const auto quality=current_quality(r.trend_glucose.data(),tq.data(),r.filtered_history.data(),
        p.age,p.current_trend_index,p.current_history_index,eligible.last>eligible.first);
    r.quality=quality.glucose_status;
    if(!(r.current.filtered_glucose>0.f))r.quality|=0x8000;
    r.quality|=tq[p.current_trend_index];
    r.rate_quality=quality.rate_disagreement;
    // Exactly 8000 means this optional trend estimate is unavailable. Native
    // aggregation skips it; source faults carrying other bits still propagate.
    for(unsigned i=0;i<3;++i)if(quality.rate_status[i]!=0x8000)r.rate_quality|=quality.rate_status[i];
    if(eligible.last>eligible.first)r.rate_quality|=quality.rate_status[3];
    if(r.quality)r.rate_quality=0x8000;
    if(r.rate_quality){r.current.filtered_rate=0.f;r.current.reported_rate=0.f;}
    if(r.quality)r.current.filtered_glucose=0.f;
    r.reported_glucose=r.quality?0.f:scan_curve(r.current.filtered_glucose);
    r.trend_raw[5*p.current_trend_index+4]|=r.quality;
    r.glucose=r.quality?0:round_reported_glucose(r.reported_glucose,p.maximum,p.minimum);
    r.secondary_glucose=r.quality?0:round_unclipped_glucose(r.reported_glucose);
    r.reported_rate=r.current.reported_rate;
    r.display_range_valid=r.glucose>=p.minimum&&r.glucose<=p.maximum;
    if(!r.display_range_valid){r.reported_rate=0.f;r.current.reported_rate=0.f;r.current.filtered_rate=0.f;r.rate_quality|=0x8000;}
    r.arrow=r.display_range_valid?trend_arrow(r.reported_rate):0;
    r.range_category=r.quality?0:projected_range_category(r.reported_glucose,r.reported_rate,r.glucose,quality.rate_disagreement,p.low,p.high);
    const float temperature=r.trend_temperature[p.current_trend_index*5+1];
    if(!r.quality&&temperature>0.f&&temperature<50.f)r.temperature_tenths=std::uint16_t(std::lround(static_cast<double>(temperature*10.f)));
    r.rate_hundredths=static_cast<std::int16_t>(std::lround(static_cast<double>(r.reported_rate*100.f)));
    if((p.elapsed_minutes==0xffffffffu?p.age:p.elapsed_minutes)<p.warmup_minutes){
        r.quality|=0x8000;r.rate_quality=0x8000;r.glucose=r.secondary_glucose=0;r.rate_hundredths=0;
        r.arrow=r.range_category=0;r.temperature_tenths=999;r.display_range_valid=false;
        r.current.filtered_glucose=r.current.filtered_rate=0.f;
    }
    r.state.minute=p.age;r.state.filtered_glucose=r.current.filtered_glucose;r.state.filtered_rate=r.current.filtered_rate;
    r.state.rate_quality=r.rate_quality;
    const unsigned reported_index=p.history_output_delay_minutes==2?p.history_last_valid_index:
        (p.age<p.history_output_delay_minutes?0u:(p.age-p.history_output_delay_minutes)/15u);
    const unsigned first=reported_index>31?reported_index-31:1;
    const auto clock=p.elapsed_minutes==0xffffffffu?p.age:p.elapsed_minutes;
    const HistoryReportLimits limits{true,std::int64_t(clock)-p.age+15*first,p.warmup_minutes,p.wear_minutes};
    if(r.history_count)r.state.history=merge_reported_history(previous.history,r.filtered_history.data(),r.history_correction.data(),p.history_first_index,first,p.maximum,p.minimum,p.is_scan,r.history_count,limits);
    return r;
}
} // namespace libre2_processing
