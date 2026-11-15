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
#include "recovered_components.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace libre2_processing {
// Native 0x21c1f4, including its diagnostic fields and native float ordering.
struct TemporalFit {
    float slope{},intercept{};
    std::int16_t count{};std::uint16_t status{};
    float sum_x{},sum_xx{},inverse_determinant{},sum_y{},sum_xy{},weight{};
};
static_assert(sizeof(TemporalFit)==36);
inline TemporalFit fit_history_window(const float *v,const std::uint16_t *q,int index,int mode) {
    TemporalFit r{};if(mode<0||mode>2)return r;
    const int first=std::max(0,index+mode-2);int last=index+mode;
    if(last>36)--last;
    int coordinate=mode==0?-30:(mode==1?-15:0);
    for(int j=first;j<=last;++j,coordinate+=15){
        if(q[j])continue;
        const float x=static_cast<float>(coordinate);
        r.sum_x=r.sum_x+x;r.sum_xx=static_cast<float>(coordinate*coordinate)+r.sum_xx;
        r.sum_y=v[j]+r.sum_y;r.sum_xy=x*v[j]+r.sum_xy;++r.count;r.status|=q[j];
    }
    r.inverse_determinant=static_cast<float>(r.count)*r.sum_xx-r.sum_x*r.sum_x;
    if(r.inverse_determinant!=0.f)r.inverse_determinant=1.f/r.inverse_determinant;
    r.slope=(static_cast<float>(r.count)*r.sum_xy-r.sum_x*r.sum_y)*r.inverse_determinant;
    r.intercept=(r.sum_y*r.sum_xx-r.sum_xy*r.sum_x)*r.inverse_determinant;
    if(r.count<2)r.status|=0x8000;
    else r.weight=mode==1?(r.count==2?3.f:4.f):(r.count==2?.5f:1.f);
    if(r.slope>5.5f||r.slope< -5.f)r.weight=0.f;
    return r;
}
struct FilterWeights {float current,previous;};
inline float reference_glucose(const std::array<float,3>&saved){
    constexpr int weights[3]{11,6,1};float sum=0.f;int denominator=0;
    for(int i=0;i<3;++i)if(saved[i]!=4294967296.f){sum=sum+static_cast<float>(weights[i])*saved[i];denominator+=weights[i];}
    if(!denominator)return 0.f;
    float value=sum/static_cast<float>(denominator);
    if(value<97.f)value=97.f;
    if(value>125.f)value=125.f;
    return value;
}
inline FilterWeights temporal_filter_weights(std::uint32_t gap){
    constexpr std::array<std::int16_t,16> p{120,105,91,78,66,55,45,36,28,21,15,10,6,3,1,0};
    if(!gap)return{0.f,1.f};
    if(gap>=16)return{1.f,0.f};
    const float n=static_cast<float>(p[gap]),d=n+120.f;return{120.f/d,n/d};
}
// Status at byte 0; glucose at byte 8; the other fields are preserved by 0x257140.
struct ProjectionResult {
    std::uint16_t status{},padding{};std::uint32_t reserved{};float glucose{};
    std::uint32_t reserved2{},reserved3{};
};
static_assert(sizeof(ProjectionResult)==20);
inline void project_three_samples(ProjectionResult&r,const std::uint16_t*q,const float*v,
        int i,std::int8_t lag1,std::int8_t lag2,std::int8_t spacing,float c1,float c2){
    const int a=i-static_cast<std::int8_t>(lag1/spacing),b=i-static_cast<std::int8_t>(lag2/spacing);
    r.status=q[i]|q[a]|q[b];if(r.status){r.glucose=0.f;return;}
    const float current=v[i];float correction=0.f;
    if(current>=18.f&&current<500.f){
        const float delta=(current-v[b])*(c2/static_cast<float>(lag2))+
                          (current-v[a])*(c1/static_cast<float>(lag1));
        const float limit=current<55.f?((current-18.f)*40.f)/37.f:
                          (current<160.f?40.f:((500.f-current)*40.f)/340.f);
        const float magnitude=std::bit_cast<float>(std::bit_cast<std::uint32_t>(delta)&0x7fffffffU);
        correction=magnitude<limit?magnitude:limit;
        if(delta<0.f)correction=std::bit_cast<float>(std::bit_cast<std::uint32_t>(correction)^0x80000000U);
    }
    r.glucose=correction+current;
}
inline ProjectionResult project_trend(const float*v,const std::uint16_t*q,int i,int mode){
    ProjectionResult r{};
    if(mode==0)project_three_samples(r,q,v,i,15,6,1,14.f,6.f);
    else if(mode==1)project_three_samples(r,q,v,i,7,2,1,12.f,4.f);
    else if(mode==2)project_three_samples(r,q,v,i,12,4,1,13.f,7.f);
    return r;
}
inline ProjectionResult project_history(const float*v,const std::uint16_t*q,int i,int mode){
    ProjectionResult r{};
    if(mode==0)project_three_samples(r,q,v,i,-15,15,15,.53f,14.33f);
    else if(mode==1)project_three_samples(r,q,v,i,-15,-30,15,15.89f,-1.21f);
    else if(mode==2)project_three_samples(r,q,v,i,15,30,15,14.77f,-3.24f);
    return r;
}
struct TemporalSample {float glucose{};std::uint16_t status{},reserved{};};
inline std::array<TemporalSample,32> filter_calibrated_history(const float*v,const std::uint16_t*q){
    std::array<float,40> padded{};std::array<std::uint16_t,40> quality{};quality.fill(0x8000);
    for(int i=0;i<32;++i){padded[i+2]=v[i];quality[i+2]=q[i]|(v[i]>0.f?0:0x8000);}
    std::array<TemporalSample,32> result{};
    for(int i=0;i<32;++i){
        const auto a=fit_history_window(padded.data(),quality.data(),i+2,0);
        const auto b=fit_history_window(padded.data(),quality.data(),i+2,1);
        const auto c=fit_history_window(padded.data(),quality.data(),i+2,2);
        const float bridge_weight=(!a.status&&!c.status&&(a.weight>0.f||c.weight>0.f))?4.f:0.f;
        const float bridge=(a.intercept+c.intercept)*.5f;
        const float denominator=((a.weight+b.weight)+c.weight)+bridge_weight;
        const float numerator=((a.intercept*a.weight+b.intercept*b.weight)+c.intercept*c.weight)+bridge*bridge_weight;
        const float slopes[4]{a.slope,b.slope,c.slope,(a.slope+c.slope)*.5f};
        const float weights[4]{a.weight,b.weight,c.weight,bridge_weight};
        bool any=false;float lo=0.f,hi=0.f;
        for(int j=0;j<4;++j)if(weights[j]>0.f){
            if(!any){lo=hi=slopes[j];any=true;}
            else{if(slopes[j]<lo)lo=slopes[j];if(slopes[j]>hi)hi=slopes[j];}
        }
        const float glucose=denominator>0.f?numerator/denominator:0.f;
        const bool valid=glucose>0.f&&!(any&&hi-lo>5.f);
        result[i].status=valid?0:std::uint16_t(q[i]|0x8000);result[i].glucose=valid?glucose:0.f;
    }
    return result;
}
// Early historical signal-change check in 0x23428c. Consecutive valid samples
// at most four slots apart feed a four-entry ring. Testing begins seven slots
// after the first valid sample; a mean absolute change above 10 rejects input.
inline bool reject_history_changes(const float*v,const std::uint16_t*q,unsigned last){
    std::array<float,4>ring{4294967296.f,4294967296.f,4294967296.f,4294967296.f};
    int first=-1,previous=-1;unsigned n=0;
    for(unsigned i=0;i<32&&i<=last;++i){
        if(q[i])continue;
        if(first<0)first=int(i);
        if(previous>=0&&int(i)-previous<=4){
            const float delta=v[i]-v[previous];
            const float magnitude=std::bit_cast<float>(std::bit_cast<std::uint32_t>(delta)&0x7fffffffU);
            ring[n%4]=magnitude/static_cast<float>(15*(int(i)-previous));++n;
        }
        previous=int(i);
        if(int(i)<first+7)continue;
        float sum=0.f;int count=0;
        for(float r:ring)if(r!=4294967296.f){sum=sum+r;++count;}
        if(count&&sum/static_cast<float>(count)>10.f)return true;
    }
    return false;
}
// Native 0x255e04 returns uint16_t. Values are finite and fit in int32_t here.
inline std::uint16_t round_unclipped_glucose(float v){
    const long rounded=std::lround(static_cast<double>(v));
    return rounded<0?std::uint16_t(0):static_cast<std::uint16_t>(static_cast<std::uint32_t>(static_cast<float>(rounded)));
}
inline std::uint16_t round_reported_glucose(float v,std::uint16_t maximum,std::uint16_t minimum){
    const auto n=round_unclipped_glucose(v);
    if(n<minimum)return minimum-1;
    return n>maximum?maximum+1:n;
}
} // namespace libre2_processing
