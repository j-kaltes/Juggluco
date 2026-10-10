// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "store.hpp"
#include "../nums/num.h"

namespace juggluco::activity {
inline float amountValue(const Record &record,int metric) {
    const float value=Store::amount(record,metric);
    // Keep the original measurements in the activity record. Amounts are
    // summaries, shown in the graph, editor and on Kerfstok with this precision.
    const double scale=metric==Steps?1.0:10.0;
    return std::isfinite(value)?std::round(double(value)*scale)/scale:value;
}
inline int amountMetric(const Record &root,const Rule &rule,bool applyMapping) {
    return (root.flags&Generated)&&!applyMapping?root.generatedMetric:rule.metric;
}
inline const Record *measurementFor(Store &store,uint32_t group,int metric) {
    const Record *measurement=nullptr;
    for(uint32_t i=1;i<=store.header().count;++i) {
        auto *record=store.get(i);
        if(record->group!=group||(record->flags&(Deleted|NeedsDetails|NotReturned))||!store.valid(*record))continue;
        if(!measurement)measurement=record;
        if(std::isfinite(Store::amount(*record,metric)))return record;
    }
    return measurement;
}
struct AmountUpdate { Num number; int metric; };
inline AmountUpdate updateAmount(Record &root,const Num &number,const Record &measurement,
                                 const Rule &rule,bool applyMapping) {
    AmountUpdate updated{number,root.generatedMetric};
    if(!(root.flags&Generated))return updated;
    if(number.type!=uint32_t(root.generatedLabel))root.flags|=LabelOverride;
    if(number.value!=root.generatedValue)root.flags|=ValueOverride;
    if(number.time!=root.generatedTime)root.flags|=TimeOverride;
    const int metric=amountMetric(root,rule,applyMapping);
    const float value=amountValue(measurement,metric);
    // Do not put a new label/unit on an old value if the source cannot supply
    // the requested measure. Individually entered values remain user-owned.
    if(applyMapping&&metric!=root.generatedMetric&&!(root.flags&ValueOverride)&&!std::isfinite(value))return updated;
    if(applyMapping&&!(root.flags&LabelOverride)&&rule.label>=0)updated.number.type=rule.label;
    if(!(root.flags&ValueOverride)&&std::isfinite(value)) {
        updated.number.value=value;
        updated.metric=metric;
    }
    // Mapping changes may run above an open Amount editor. Keep its Num pointer
    // valid by deferring moves in the number array until ordinary retrieval.
    if(!applyMapping&&!(root.flags&TimeOverride))updated.number.time=root.start;
    return updated;
}
}
