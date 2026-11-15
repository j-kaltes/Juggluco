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
#include "temporal_components.hpp"
namespace libre2_processing {
inline constexpr std::array<float,15> current_projection_far{-3.24f,-3.37f,-4.01f,-5.83f,-6.44f,-6.08f,-7.38f,-7.47f,-7.64f,-6.9f,-6.52f,-5.67f,-3.82f,-4.63f,-5.73f};
inline constexpr std::array<float,15> current_projection_near{16.77f,17.46f,18.67f,20.93f,22.33f,22.74f,24.39f,25.08f,25.57f,25.34f,25.99f,25.34f,24.11f,25.51f,27.06f};
inline constexpr std::array<float,15> history_rate_far{-.324f,-.337f,-.401f,-.583f,-.644f,-.608f,-.738f,-.747f,-.764f,-.69f,-.652f,-.567f,-.382f,-.463f,-.573f};
inline constexpr std::array<float,15> history_rate_near{1.477f,1.546f,1.667f,1.893f,2.033f,2.074f,2.239f,2.308f,2.357f,2.334f,2.399f,2.334f,2.211f,2.351f,2.506f};
inline constexpr std::array<std::int16_t,15> history_glucose_weight{92,91,91,90,89,88,88,87,86,85,85,84,83,82,82};
inline constexpr std::array<std::int16_t,15> history_rate_weight{92,91,91,90,89,88,88,87,86,86,85,83,83,82,82};

// Native final rate compression uses double arithmetic in the middle branch.
inline float compress_rate(float rate){
    const float magnitude=std::bit_cast<float>(std::bit_cast<std::uint32_t>(rate)&0x7fffffffU);
    if(magnitude<=1.25f)return .8f*rate;
    if(magnitude>=2.f)return rate;
    const float result=static_cast<float>((static_cast<double>(magnitude)-1.25)*1.3333333730697632+1.0);
    return rate<0.f?std::bit_cast<float>(std::bit_cast<std::uint32_t>(result)^0x80000000U):result;
}
inline unsigned trend_arrow(float rate){return rate< -2.f?1:(rate< -1.f?2:(rate<=1.f?3:(rate<=2.f?4:5)));}
inline unsigned glucose_range_category(std::uint16_t glucose,std::uint16_t quality,
        std::uint16_t low,std::uint16_t high,unsigned initial=3){
    if(quality)return 0;
    if(glucose<low)return 1;
    if(glucose>high)return 5;
    return initial;
}
inline unsigned projected_range_category(float glucose,float rate,std::uint16_t displayed,
        std::uint16_t quality,std::uint16_t low,std::uint16_t high){
    // The native prediction is rounded, narrowed to a signed word, then compared.
    const auto future=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(
        std::lround(static_cast<double>(rate*15.f+glucose))));
    const unsigned initial=future<low?2:(future>high?4:3);
    // A measured out-of-target result still overrides a suppressed prediction.
    return glucose_range_category(displayed,0,low,high,quality?0:initial);
}
struct CurrentQuality {
    std::array<float,4>rate{};
    std::array<std::uint16_t,4>rate_status{};
    std::uint16_t glucose_status{},rate_disagreement{};
};
inline CurrentQuality current_quality(const float*trend,const std::uint16_t*tflags,
        const TemporalSample*history,unsigned age,int ti,int hi,bool require_history_quality=true){
    CurrentQuality q{};
    constexpr int lag1[3]{15,7,12},lag2[3]{6,2,4};
    constexpr float c1[3]{.217f,1.054f,.634f},c2[3]{1.02f,.169f,.585f};
    for(int m=0;m<3;++m){
        if(ti<lag1[m]||ti<lag2[m])q.rate_status[m]=0x8000;
        else{
            q.rate_status[m]=tflags[ti]|tflags[ti-lag1[m]]|tflags[ti-lag2[m]];
            if(!q.rate_status[m])q.rate[m]=(trend[ti]-trend[ti-lag2[m]])*(c2[m]/float(lag2[m]))+
                (trend[ti]-trend[ti-lag1[m]])*(c1[m]/float(lag1[m]));
        }
        if(!q.rate_status[m]&&(q.rate[m]<-5.f||q.rate[m]>5.5f))q.rate_status[m]=0x8000;
    }
    if(hi<2)q.rate_status[3]=0x8000;
    else{
        q.rate_status[3]=history[hi].status|history[hi-1].status|history[hi-2].status;
        if(q.rate_status[3])q.rate_status[3]=0x8000;
        if(!q.rate_status[3])q.rate[3]=(history[hi].glucose-history[hi-1].glucose)*(history_rate_near[age%15]/15.f)+
            (history[hi].glucose-history[hi-2].glucose)*(history_rate_far[age%15]/30.f);
        if(!q.rate_status[3]&&(q.rate[3]<-5.f||q.rate[3]>5.5f))q.rate_status[3]=0x8000;
    }
    bool any=false;float lo=0.f,hi_rate=0.f;
    for(unsigned i=0;i<4;++i)if(!q.rate_status[i]){
        if(!any){lo=hi_rate=q.rate[i];any=true;}
        else{if(q.rate[i]<lo)lo=q.rate[i];if(q.rate[i]>hi_rate)hi_rate=q.rate[i];}
    }
    const float spread=hi_rate-lo;
    q.rate_disagreement=any&&spread>3.f?0x8000:0;
    q.glucose_status=(!any||spread>6.f?0x8000:0)|(require_history_quality?q.rate_status[3]:0);
    return q;
}
struct CurrentEstimates {
    float projected_glucose{},projected_rate{},filtered_glucose{},filtered_rate{},reported_rate{};
};
// Rate qualification also controls which trend glucose projections are mixed.
// Full output quality handling lives in evaluate_numeric(); the additive
// correction is supplied separately to preserve the native stage ordering.
inline CurrentEstimates combine_current_projections(const float*trend,const std::uint16_t*tflags,
        const TemporalSample*history,unsigned age,int trend_index,int history_index,
        float previous_glucose,float previous_rate,std::uint16_t gap,float correction,std::uint16_t previous_rate_quality=0){
    std::array<float,48>tv{},hv{};std::array<std::uint16_t,48>tq{},hq{};tq.fill(0x8000);hq.fill(0x8000);
    for(int i=0;i<16;++i){tv[i+15]=trend[i];tq[i+15]=tflags[i];}
    for(int i=0;i<32;++i){hv[i+2]=history[i].glucose;hq[i+2]=history[i].status;}
    const int ti=trend_index+15,hi=history_index+2;const unsigned phase=age%15;
    const auto quality=current_quality(trend,tflags,history,age,trend_index,history_index);
    ProjectionResult hp{};project_three_samples(hp,hq.data(),hv.data(),hi,30,15,15,current_projection_far[phase],current_projection_near[phase]);
    float sum=0.f,weight=0.f,rsum=0.f,rweight=0.f;
    const float hw=static_cast<float>(history_glucose_weight[phase]);
    if(!hp.status){sum=hp.glucose*hw;weight=hw;}
    if(!(hq[hi]|hq[hi-1]|hq[hi-2])){
        const float r=(hv[hi]-hv[hi-1])*(history_rate_near[phase]/15.f)+(hv[hi]-hv[hi-2])*(history_rate_far[phase]/30.f);
        if(r>=-5.f&&r<=5.5f){const float w=static_cast<float>(history_rate_weight[phase]);rsum=r*w;rweight=w;}
    }
    constexpr int lag1[3]{15,7,12},lag2[3]{6,2,4};
    constexpr float c1[3]{.217f,1.054f,.634f},c2[3]{1.02f,.169f,.585f},rw[3]{91.f,90.f,91.f};
    for(int m=0;m<3;++m){
        const auto projected=project_trend(tv.data(),tq.data(),ti,m);
        if(!projected.status&&!quality.rate_status[m]){sum=sum+projected.glucose*91.f;weight=weight+91.f;}
        if(!(tq[ti]|tq[ti-lag1[m]]|tq[ti-lag2[m]])){
            const float r=(tv[ti]-tv[ti-lag2[m]])*(c2[m]/static_cast<float>(lag2[m]))+(tv[ti]-tv[ti-lag1[m]])*(c1[m]/static_cast<float>(lag1[m]));
            if(r>=-5.f&&r<=5.5f){rsum=rsum+r*rw[m];rweight=rweight+rw[m];}
        }
    }
    if(!tq[ti]){sum=sum+tv[ti]*10.f;weight=weight+10.f;}
    CurrentEstimates r{};if(weight>0.f)r.projected_glucose=sum/weight+correction;
    if(rweight>0.f)r.projected_rate=rsum/rweight;
    const auto w=temporal_filter_weights(gap);
    r.filtered_glucose=previous_glucose>0.f?w.previous*previous_glucose+w.current*r.projected_glucose:r.projected_glucose;
    r.filtered_rate=previous_glucose>0.f&&!previous_rate_quality?w.previous*previous_rate+w.current*r.projected_rate:r.projected_rate;
    r.reported_rate=compress_rate(r.filtered_rate);return r;
}
} // namespace libre2_processing
