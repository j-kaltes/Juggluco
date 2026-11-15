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
#include "persistent_state.hpp"
namespace libre2_processing {
// Native historical projection mixture: three projections and the filtered
// measurement, followed by the additive correction and scan curve.
inline std::array<TemporalSample,31> project_reported_history(const TemporalSample*history,const float*correction){
    std::array<float,40>values{};std::array<std::uint16_t,40>flags{};flags.fill(0x8000);
    for(int i=0;i<32;++i){values[i+2]=history[i].glucose;flags[i+2]=history[i].status;}
    constexpr float weights[3]{94.f,94.f,92.f};std::array<TemporalSample,31>out{};
    for(int i=0;i<31;++i){
        float sum=0.f,weight=0.f;
        for(int m=0;m<3;++m){const auto p=project_history(values.data(),flags.data(),i+2,m);if(!p.status){sum=sum+p.glucose*weights[m];weight=weight+weights[m];}}
        if(!history[i].status){sum=sum+history[i].glucose*75.f;weight=weight+75.f;}
        if(weight>0.f){out[i].glucose=scan_curve(sum/weight+correction[i]);if(out[i].glucose<0.f)out[i].glucose=0.f;out[i].status=0;}
        else out[i].status=history[i].status|0x8000;
    }
    return out;
}
// Cached entries are reused by timestamp, including nonzero quality codes.
// ffff is missing: scans may fill every slot, streams only the newest.
// The 32-point raw window can move before completed timestamps advance.
struct HistoryReportLimits {bool enabled{};std::int64_t first_elapsed{};std::uint16_t warmup{},wear=65535;};
inline std::array<PersistedHistory,31> merge_reported_history(
        const std::array<PersistedHistory,31>&previous,const TemporalSample*history,
        const float*correction,unsigned raw_first_index,unsigned output_first_index,
        std::uint16_t maximum=500,std::uint16_t minimum=40,bool is_scan=true,unsigned output_count=31,HistoryReportLimits limits={}){
    (void)raw_first_index;
    const auto projected=project_reported_history(history,correction);
    std::array<PersistedHistory,31>out{};
    for(unsigned i=0;i<31;++i){
        const unsigned index=output_first_index+i;const auto minute=static_cast<std::uint16_t>(15*index);
        out[i]={0,0xffff,minute,0.f};
        if(i>=output_count)continue;
        bool reused=false;
        for(const auto&p:previous)if(p.minute==minute&&(p.quality!=0xffff||(!is_scan&&i+1<output_count))){out[i]=p;reused=true;break;}
        if(reused)continue;
        out[i]={0,0xffff,minute,0.f};
        if(!is_scan&&i+1<output_count)continue;
        const auto elapsed=limits.first_elapsed+15*i;
        if(limits.enabled&&(elapsed<limits.warmup||elapsed>limits.wear)){out[i].quality=projected[i].status|0x8000;continue;}
        // Native output slots are positional, including the quarter-hour
        // boundary where the raw and completed-history timestamp bases differ.
        const auto&r=projected[i];out[i].quality=r.status;out[i].filtered_glucose=r.glucose;
        if(!r.status)out[i].glucose=round_reported_glucose(r.glucose,maximum,minimum);
    }
    return out;
}
} // namespace libre2_processing
