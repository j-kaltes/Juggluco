// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace juggluco::activity {
struct SpeedPeak {float value=std::numeric_limits<float>::quiet_NaN();int64_t time=0;};

inline float speedSpikeLimit(uint32_t type) {
    // AndroidX ExerciseSessionRecord types. These generous suspicion thresholds
    // are not hard caps: an incorrectly selected activity may really be faster.
    switch(type) {
        case 37: case 79: return 25.0f/3.6f; // Hiking, walking.
        case 56: case 57: return 45.0f/3.6f; // Running, treadmill running.
        case 8: case 9: return 90.0f/3.6f; // Biking, stationary biking.
        case 73: case 74: return 15.0f/3.6f; // Open-water and pool swimming.
        default: return std::numeric_limits<float>::infinity();
    }
}

// Samples must be from one source, in time order; values are m/s, times are ms.
// Only the derived maximum changes. Keep the raw data for means and mirroring.
template<class Sample>
SpeedPeak filteredSpeedPeak(std::span<const Sample> samples,uint32_t type,int64_t start,int64_t end) {
    constexpr int64_t minute=60000;
    // Walking/hiking artefacts can take several seconds to fall back to normal.
    const double acceleration=(type==37||type==79)?2.0:4.0; // m/s^2, using actual sample intervals.
    const auto limit=speedSpikeLimit(type);
    auto valid=[&](const Sample &p) {
        return std::isfinite(p.value)&&p.value>=0&&p.time>=start&&p.time<end;
    };
    auto include=[](SpeedPeak &peak,const Sample &p) {
        if(!std::isfinite(peak.value)||p.value>peak.value||(p.value==peak.value&&p.time<peak.time))
            peak={p.value,p.time};
    };
    SpeedPeak result;
    if(!std::isfinite(limit)) {
        for(const auto &p:samples)
            if(valid(p))
                include(result,p);
        return result;
       }
    // The increasing stack finds the nearest sample below half the candidate
    // speed. Do not require that neighbour to be below the activity limit:
    // even a mislabelled fast ride can contain a much faster GPS spike.
    struct Context {const Sample *sample;size_t abrupt;};
    std::vector<Context> context;
    struct Side {uint16_t elapsed=0;bool qualifies=false,neighbour=false;};
    std::vector<Side> left(samples.size());
    for(bool reverse:{false,true}) {
        context.clear();size_t abrupt=0;
        const Sample *previous=nullptr;
        for(size_t offset=0;offset<samples.size();++offset) {
            const size_t at=reverse?samples.size()-1-offset:offset;
            const auto &p=samples[at];if(!valid(p))continue;
            if(previous) {
                const auto dt=reverse?previous->time-p.time:p.time-previous->time;
                // In reverse order a rise measures forward deceleration.
                // Equal timestamps give no evidence of acceleration.
                if(dt>0&&(double(p.value)-previous->value)*1000>acceleration*dt)++abrupt;
            }
            if(p.value>limit) {
                const auto found=std::lower_bound(context.begin(),context.end(),p.value/2,
                    [](const Context &c,float half){return c.sample->value<half;});
                const bool neighbour=found!=context.begin();
                bool qualifies;
                int64_t elapsed;
                if(neighbour) {
                    const auto &near=*(found-1);
                    elapsed=reverse?near.sample->time-p.time:p.time-near.sample->time;
                    qualifies=elapsed>0&&elapsed<=minute&&abrupt>near.abrupt;
                } else {
                    // Only a real session boundary may replace missing context;
                    // a gap in the middle cannot stand in for the start/end.
                    elapsed=reverse?end-p.time:p.time-start;
                    qualifies=elapsed<=minute;
                }
                if(!reverse)left[at]={uint16_t(qualifies?elapsed:0),qualifies,neighbour};
                else {
                    // Keep sustained fast travel even when both ends of it
                    // happen to fall within a minute of this particular point.
                    const auto &before=left[at];
                    const bool spike=before.qualifies&&qualifies&&(before.neighbour||neighbour)&&
                        before.elapsed+elapsed<=minute;
                    if(!spike)include(result,p);
                }
            } else if(reverse)include(result,p);
            while(!context.empty()&&context.back().sample->value>=p.value)context.pop_back();
            context.push_back({&p,abrupt});previous=&p;
        }
    }
    return result;
}
}
