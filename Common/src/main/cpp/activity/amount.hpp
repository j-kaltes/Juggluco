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
/*      Thu Oct 15 12:52:57 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.hpp"
#include "../nums/num.h"

namespace juggluco::activity {
constexpr uint32_t amountMatchMargin=10*60;
struct AmountMatchTarget {uint32_t group;int32_t label;uint32_t start,end;};
inline std::array<uint32_t,2> amountMatchRange(uint32_t start,uint32_t end) {
    return {start>amountMatchMargin?start-amountMatchMargin:0,
        uint32_t(std::min<uint64_t>(UINT32_MAX,uint64_t(end)+amountMatchMargin))};
}
inline uint32_t preferredAmountGroup(std::span<const AmountMatchTarget> targets,
                                    int label,int defaultLabel,uint32_t time) {
    uint32_t best=amountMatchMargin+1,group=0;
    for(const auto &target:targets) {
        if((target.label<0?defaultLabel:target.label)!=label)continue;
        const auto gap=time<target.start?target.start-time:time>target.end?time-target.end:0;
        if(gap>amountMatchMargin)continue;
        if(gap<best){best=gap;group=target.group;}
        else if(gap==best)group=0; // Never let processing order resolve a tie.
    }
    return group;
}
inline double roundingIncrement(int metric,float roundTo) {
    if(roundTo<=0)return metric==Steps?1.0:0.1;
    // Recover a short decimal that round-trips to the saved float increment.
    // Multiplying by 0.1f directly can store 1.3000001 instead of 1.3f.
    for(double scale=1;scale<=1e46;scale*=10) {
        const double decimal=std::round(double(roundTo)*scale)/scale;
        if(float(decimal)==roundTo)return decimal;
    }
    return roundTo;
}
inline float amountValue(const Record &record,int metric,float roundTo=0) {
    const float value=Store::amount(record,metric);
    // Keep the original measurements in the activity record. Amounts are
    // summaries. Round after conversion to the selected measure's unit.
    if(!std::isfinite(value))return value;
    const double increment=roundingIncrement(metric,roundTo);
    double scaled=double(value)/increment;
    // SI values and increments are floats. Snap half steps within their input
    // precision so e.g. 2.35 rounded to 0.1 is 2.4 rather than 2.3.
    const double half=std::floor(std::abs(scaled))+0.5;
    double precision=std::numeric_limits<float>::epsilon()*std::max(1.0,std::abs(scaled));
    if(metric==ExcessHR)precision=std::max(precision,std::numeric_limits<float>::epsilon()*
        (std::abs(double(record.values[AverageHR]))+std::abs(double(record.values[RestHR])))/increment);
    // Large quantities or very fine increments must not turn whole/quarter
    // steps into half steps merely because the relative float error is large.
    precision=std::min(precision,0.0001);
    if(std::abs(std::abs(scaled)-half)<=precision)
        scaled=std::copysign(half,scaled);
    const float rounded=std::round(scaled)*increment;
    return std::isfinite(rounded)?rounded:value;
}
inline int amountMetric(const Record &root,const Rule &rule,bool applyMapping) {
    return (root.flags&Generated)&&!applyMapping?root.generatedMetric:rule.metric;
}
inline const Record *measurementFor(Store &store,uint32_t group,int metric) {
    group=store.groupOf(group);
    const Record *measurement=nullptr;
    int quality=-1;
    for(uint32_t i=1;i<=store.header().count;++i) {
        auto *record=store.get(i);
        if(record->group!=group||(record->flags&(NeedsDetails|NotReturned))||!store.sourceVisible(i))continue;
        const float value=Store::amount(*record,metric);
        // The requested measure must be available first. Then prefer workout
        // apps over Fit, and richer records before comparing duration.
        // A longer Fit interval alone does not make its tiny distance useful.
        const int rank=std::isfinite(value)?1+int((metric==Distance||metric==DistanceMiles)&&value>0):0;
        if(!measurement||rank>quality||(rank==quality&&store.preferSource(*record,*measurement))) {
            measurement=record;quality=rank;
        }
    }
    return measurement;
}
inline Record groupMeasurement(Store &store,uint32_t group,const Record &source) {
    Record result=source;
    result.values[Seconds]=float(store.elapsed(group));
    return result;
}
inline void rememberOverrides(Record &owner,const Num &number) {
    if(!(owner.flags&Generated))return;
    if(number.type!=uint32_t(owner.generatedLabel))owner.flags|=LabelOverride;
    if(number.value!=owner.generatedValue)owner.flags|=ValueOverride;
    if(number.time!=owner.generatedTime)owner.flags|=TimeOverride;
}
inline bool untouchedGenerated(Record &owner,const Num &number) {
    rememberOverrides(owner,number);
    return (owner.flags&Generated)&&!(owner.flags&(LabelOverride|ValueOverride|TimeOverride));
}
enum class RemovedAmount { Keep, Erase, Detach };
inline RemovedAmount removedAmount(Store &store,const Num &number) {
    if(!Num::issportref(number.mealptr))return RemovedAmount::Keep;
    const auto index=indexOf(number.mealptr),group=store.groupOf(index);
    if(!group||store.groupVisible(group))return RemovedAmount::Keep;
    auto &owner=*store.get(index);
    rememberOverrides(owner,number);
    // Source deletion removes imported summaries. Individually entered/edited
    // numbers remain ordinary Amounts without a link to a deleted activity.
    return (owner.flags&Generated)&&!(owner.flags&(LabelOverride|ValueOverride|TimeOverride))?
        RemovedAmount::Erase:RemovedAmount::Detach;
}
// Indices into a snapshot of local Amounts. Keep every user-owned entry and at
// most one untouched generated entry; a user-owned entry takes its place when
// present. The caller uses Numdata's normal deletion path for backup/Kerfstok.
inline std::vector<size_t> duplicateAmounts(Store &store,std::span<const Num> numbers) {
    const size_t absent=numbers.size();
    std::vector<size_t> primary(store.header().count+1,absent),duplicates;
    std::vector<uint32_t> groups(numbers.size());
    std::vector<bool> automatic(numbers.size());
    for(size_t i=0;i<numbers.size();++i) {
        const auto &n=numbers[i];if(!Num::issportref(n.mealptr))continue;
        const auto ownerId=indexOf(n.mealptr),group=store.groupOf(ownerId);
        if(!group)continue;
        auto &owner=*store.get(ownerId);
        rememberOverrides(owner,n);
        automatic[i]=(owner.flags&Generated)&&!(owner.flags&(LabelOverride|ValueOverride|TimeOverride));
        groups[i]=group;
        auto &best=primary[group];
        if(best==absent)best=i;
        else if(automatic[best]&&!automatic[i])best=i;
        else if(automatic[best]&&automatic[i]) {
            const auto &old=*store.get(indexOf(numbers[best].mealptr));
            if(owner.end-owner.start>old.end-old.start)best=i;
        }
    }
    for(size_t i=0;i<numbers.size();++i)
        if(groups[i]&&automatic[i]&&primary[groups[i]]!=i)duplicates.push_back(i);
    return duplicates;
}
struct AmountUpdate { Num number; int metric; float roundTo; };
inline AmountUpdate updateAmount(Record &root,const Num &number,const Record &measurement,
                                 const Rule &rule,bool applyMapping,uint32_t groupStart=0,float previousRoundTo=0) {
    AmountUpdate updated{number,root.generatedMetric,previousRoundTo};
    if(!(root.flags&Generated))return updated;
    rememberOverrides(root,number);
    const int metric=amountMetric(root,rule,applyMapping);
    const float roundTo=applyMapping?rule.roundTo:previousRoundTo;
    const float value=amountValue(measurement,metric,roundTo);
    // Do not put a new label/unit on an old value if the source cannot supply
    // the requested measure. Individually entered values remain user-owned.
    if(applyMapping&&metric!=root.generatedMetric&&!(root.flags&ValueOverride)&&!std::isfinite(value))return updated;
    if(applyMapping&&!(root.flags&LabelOverride)&&rule.label>=0)updated.number.type=rule.label;
    if(!(root.flags&ValueOverride)&&std::isfinite(value)) {
        updated.number.value=value;
        updated.metric=metric;
        updated.roundTo=roundTo;
    }
    // Mapping changes may run above an open Amount editor. Keep its Num pointer
    // valid by deferring moves in the number array until ordinary retrieval.
    if(!applyMapping&&!(root.flags&TimeOverride))updated.number.time=groupStart?groupStart:root.start;
    return updated;
}
}
