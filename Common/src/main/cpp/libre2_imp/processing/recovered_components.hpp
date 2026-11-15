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
// Partial independent reconstruction; see FINDINGS.md for the tested scope.
// Compile without fast-math and with -ffp-contract=off.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <array>
#include "factory_lookup.hpp"
#include "patch_profile.hpp"
namespace libre2_processing {
inline float age_correction(std::uint16_t age,bool enabled) {
    if(!enabled||age<60||age>5760)return 0.f;
    const double exponent=-int(age);
    const float value=float(80.0*(std::pow(0x1.005b06p+0,exponent)-std::pow(0x1.00a80ap+0,exponent)));
    return std::min(value,8.f);
}
// Entry 0x24fcd0. The caller supplies a history index in 15-minute units.
inline float early_signal_correction(std::uint16_t history_index,float current,
        float history,bool enabled) {
    if(!enabled||!(current>0.f)||!(current>history))return 0.f;
    float factor=0.f;
    if(history_index>=4&&history_index<=96){
        factor=float(history_index)/-96.f;
        factor=(factor+1.f)*0.3f;
    }
    const float correction=(current-history)*factor;
    return correction<36.f?correction:36.f;
}
// Entry 0x2511c0; controlled by configuration byte 0x30, bit 3.
inline float late_age_correction(std::uint16_t age,bool enabled) {
    if(!enabled||age<11520)return 0.f;
    const unsigned capped=std::min(unsigned(age),24480u);
    const float scaled=float(capped-11520u)*15.f;
    return 0.f+scaled/12960.f;
}
inline float compensate_temperature(float signal,float temperature,std::uint16_t coefficient_milli,bool valid) {
    if(!valid)return 0.f;
    return float(double(signal)*std::pow(double(coefficient_milli)/1000.0,32.5-double(temperature)));
}
inline float scan_curve(float input) {
    const float x=input/37.5f;
    const float sixth=float(std::pow(double(x),6.0));
    const float fraction=(1.f-sixth)/(sixth+1.f);
    const float shifted=fraction+1.f;
    const float scaled=shifted*0.49f;
    const float slope=scaled+1.02f;
    const float product=slope*input;
    return product+-4.f;
}
struct RawTemperature {float temperature;std::uint16_t status;};
struct CalibratedSignal {float signal;std::uint16_t status;};
struct FactoryCoefficients {float divisor,offset;};
inline constexpr std::array<std::int32_t,4> native_factory_parameters{
    -965071,10500544,-1469174,13193833};
struct FactoryFooter {std::int16_t upper,lower;std::uint16_t temperature_reference;};
inline FactoryFooter decode_factory_footer(std::span<const std::uint8_t,8> bytes) {
    int lower=bytes[0];if(bytes[4]&2)lower=-lower;
    return {std::int16_t(bytes[1]|((bytes[2]&0x3f)<<8)),std::int16_t(lower),
        std::uint16_t((bytes[6]>>4)|(bytes[7]<<4))};
}
struct FactoryMetadata {
    FactoryFooter references;
    std::uint16_t index,factory_day;
    std::uint8_t correction_code,average_delta,peak_delta;
};
inline FactoryMetadata decode_factory_metadata(std::span<const std::uint8_t,344> fram) {
    auto word=[&](unsigned offset){return std::uint16_t(fram[offset]|(unsigned(fram[offset+1])<<8));};
    return {decode_factory_footer(std::span<const std::uint8_t,8>(fram.data()+0x150,8)),
        std::uint16_t((word(2)>>3)&1023),word(0x144),std::uint8_t(fram[2]&7),
        std::uint8_t(fram[0x14e]&31),std::uint8_t(fram[0x14f]&63)};
}
// Entry 0x25e770. Field names retain their offsets where their physical meaning
// is not established. Parameters are four signed integers supplied by the caller.
// The correction code is the sensor's three-bit field (0..7).
inline FactoryCoefficients calculate_factory_coefficients(std::uint16_t index,
        std::uint16_t adjustment_word,std::uint8_t correction_code,
        const std::array<std::int32_t,4>& parameters) {
    if(index==0||index>1023)return {0.f,0.f};
    const auto table=factory_lookup[index-1];
    const float p3=float(parameters[3])/10000000.f;
    const float p2=float(parameters[2])/1000.f;
    const float p1=float(parameters[1])/10000000.f;
    const float p0=float(parameters[0])/1000000.f;
    float base=1000.f*table.sensitivity;
    base=base*p1+p0;
    float intercept=float(table.bias)*250.f;
    intercept=intercept*p3+p2;
    float adjustment=0.f;
    if(adjustment_word>180){
        adjustment=float(unsigned(correction_code)*2u*(unsigned(adjustment_word)-180u))/30000.f;
        adjustment=std::min(adjustment,0.1f);
    }
    return {(1.f-adjustment)*base,-intercept/base};
}
inline FactoryCoefficients calculate_factory_coefficients(std::uint16_t index,
        std::uint16_t adjustment_word,std::uint8_t correction_code) {
    return calculate_factory_coefficients(index,adjustment_word,correction_code,native_factory_parameters);
}
// Entry 0x24b88d. The two configuration words are signed ADC reference values.
inline CalibratedSignal normalize_raw_signal(float sample,std::int16_t upper,
        std::int16_t lower,std::uint16_t status) {
    if(status)return {0.f,status};
    if(upper==lower)return {0.f,0x8000};
    const float baseline=float(lower);
    if(!(sample>baseline))status=0x8000;
    float value=(sample-baseline)*65.f;
    value*=1000.f;
    if(!(sample>baseline))value=0.f;
    return {value/float(int(upper)-int(lower)),status};
}
// Entry 0x24f280. A zero divisor invalidates the record, including -0.f.
inline CalibratedSignal apply_factory_calibration(float signal,float offset,
        float divisor,std::uint16_t status) {
    if(divisor==0.f)status|=0x8000;
    if(status)return {0.f,status};
    const float scaled=signal/divisor;
    return {scaled+offset,status};
}
// Entry 0x24d73c: prepare the temperature fields consumed by compensation.
// Existing errors suppress the temperature; the accepted interval is [20,40].
inline RawTemperature classify_temperature(float temperature,std::uint16_t status){
    if(status)return {0.f,status};
    if(temperature>40.f)status|=0x2000;
    else if(temperature<20.f)status|=0x4000;
    return {temperature,status};
}
// Entry 0x24e465. Input/output records have five floats and five uint16 words.
// The native fixed buffers require count + 2*radius <= 34; real calls use
// (count=16,radius=3) and (count=32,radius=1).
inline void smooth_temperature_records(unsigned count,unsigned radius,float *output,std::uint16_t *input){
    std::array<float,34> temperatures{};std::array<std::uint16_t,34> flags;
    flags.fill(0x8000);
    for(unsigned i=0;i<count;++i){temperatures[radius+i]=output[i*5+3];flags[radius+i]=input[i*5+4];}
    for(unsigned i=0;i<count;++i){
        float sum=0.f;unsigned valid=0;
        for(unsigned j=i;j<=i+2*radius;++j)if(!flags[j]){sum+=temperatures[j];++valid;}
        output[i*5+3]=valid?sum/float(valid):0.f;
    }
    for(unsigned i=0;i<count;++i){
        auto status=input[i*5+4];
        input[i*5+4]=!(output[i*5+3]>0.f)?std::uint16_t((status&0x6fff)|0x8000):std::uint16_t(status&0x87ff);
    }
}
// Entry 0x24b934: record words 1 and 3 are signed ADC operands; word 4 is
// status. Only output float 1 and the input status word change in this stage.
inline RawTemperature convert_raw_temperature(std::uint16_t numerator_bits,
        std::uint16_t denominator_bits,std::uint16_t status) {
    if(status)return {0.f,status};
    const auto numerator=std::int16_t(numerator_bits);
    const auto denominator=std::int16_t(denominator_bits);
    if(numerator<=0)return {0.f,std::uint16_t(status|0x1000)};
    if(denominator==0)return {0.f,std::uint16_t(status|0x0800)};
    const float scaled=72500.f*float(numerator);
    const float ratio=scaled/float(denominator);
    const double resistance=double(ratio)-1000.0;
    if(!(resistance>0))return {0.f,std::uint16_t(status|0x0800)};
    const double x=std::log(resistance);
    const double linear=0.0001964561*x+0.0009180023;
    const double quadratic=7.061775e-7*x*x+linear;
    const double cubic=5.283566e-8*x*x*x+quadratic;
    const float temperature=float(1.0/cubic-273.15);
    if(temperature<0.f||temperature>50.f)status|=0x0800;
    return {temperature,status};
}
struct CalibrationParameters {
    std::int16_t adc_upper,adc_lower;
    std::uint16_t temperature_coefficient_milli;
    FactoryCoefficients glucose;
    std::uint8_t signal_average_delta=13,signal_peak_delta=38;
};
// The trend preprocessor at 0x23428c, ending before its normalization calls.
inline void prepare_trend_signals(float *output,std::uint16_t *raw,
        std::uint8_t average_delta,std::uint8_t peak_delta) {
    std::array<std::int16_t,20> samples{};
    std::array<std::uint16_t,20> flags;flags.fill(0x8000);
    for(unsigned i=0;i<16;++i){samples[i+2]=std::int16_t(raw[i*5]);flags[i+2]=raw[i*5+4];}
    for(unsigned i=0;i<16;++i){
        unsigned count=0,last=0,pairs=0;float sum=0.f,delta_sum=0.f;std::uint16_t status=0;
        for(unsigned j=i;j<i+5;++j){
            status|=flags[j];
            if(flags[j])continue;
            sum+=float(samples[j]);
            if(count&&last+1==j){
                const float delta=std::fabs(float(int(samples[j])-int(samples[last])));
                if(delta>float(unsigned(peak_delta)*16u))status|=8;
                delta_sum+=delta;
                ++pairs;
            }
            last=j;++count;
        }
        const float mean=count?sum/float(count):0.f;
        if(pairs&&delta_sum/float(pairs)>float(unsigned(average_delta)*16u))status|=16;
        if(!(status&24)&&mean!=0.f)status=0;
        output[i*5]=status?0.f:mean;raw[i*5+4]=status;
    }
}
inline void prepare_history_signals(unsigned count,float *output,const std::uint16_t *raw) {
    for(unsigned i=0;i<count;++i)output[i*5]=float(std::int16_t(raw[i*5]));
}
// Six identified native stages, before later scan/stream filtering and rounding.
// Records use the native layouts: five uint16 words and five output floats.
// count + 2*radius must not exceed 34; the observed shapes are 16/3 and 32/1.
inline void calibrate_prepared_records(unsigned count,unsigned radius,float *output,
        std::uint16_t *raw,float *glucose,const CalibrationParameters& parameters) {
    for(unsigned i=0;i<count;++i){
        auto *out=output+i*5;auto *input=raw+i*5;
        auto signal=normalize_raw_signal(out[0],
            parameters.adc_upper,parameters.adc_lower,input[4]);
        out[0]=signal.signal;input[4]=signal.status;
        auto temperature=convert_raw_temperature(input[1],input[3],input[4]);
        out[1]=temperature.temperature;input[4]=temperature.status;
        temperature=classify_temperature(out[1],input[4]);
        out[3]=out[4]=temperature.temperature;input[4]=temperature.status;
    }
    smooth_temperature_records(count,radius,output,raw);
    for(unsigned i=0;i<count;++i){
        auto *out=output+i*5;auto *input=raw+i*5;
        out[0]=compensate_temperature(out[0],out[3],parameters.temperature_coefficient_milli,input[4]==0);
        auto result=apply_factory_calibration(out[0],parameters.glucose.offset,
            parameters.glucose.divisor,input[4]);
        glucose[i]=result.signal;input[4]=result.status;
    }
}
inline void calibrate_raw_records(unsigned count,unsigned radius,float *output,
        std::uint16_t *raw,float *glucose,const CalibrationParameters& parameters) {
    if(count==16)prepare_trend_signals(output,raw,parameters.signal_average_delta,parameters.signal_peak_delta);
    else prepare_history_signals(count,output,raw);
    calibrate_prepared_records(count,radius,output,raw,glucose,parameters);
}
}
