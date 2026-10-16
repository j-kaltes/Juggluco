// SPDX-License-Identifier: GPL-3.0-or-later
#include "../amount.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

namespace a=juggluco::activity;
static bool near(float lhs,float rhs) { return std::abs(lhs-rhs)<0.001f; }
int main() {
    static_assert(a::Duration==0&&a::Distance==1&&a::Speed==6&&a::Cadence==10);
    char directory[]="/tmp/juggluco-activity-amounts-XXXXXX";
    assert(mkdtemp(directory));
    {
        a::Store store(directory);
        a::Record input{};input.type=8;input.start=1700000000;input.end=input.start+7134;
        const auto group=store.upsert("garmin","com.garmin.android.apps.connectmobile","","",input);
        input.start+=60;input.end+=80;
        const auto other=store.upsert("fit","com.google.android.apps.fitness","","",input);
        auto &root=*store.get(group),&fit=*store.get(other);
        assert(fit.group==group);
        root.flags=a::Linked|a::Generated;fit.flags=0;
        root.values[a::Metres]=29562.3f;fit.values[a::Metres]=29208.5f;
        root.values[a::MetresPerSecond]=4.3f;
        root.generatedLabel=3;root.generatedMetric=a::Duration;
        root.generatedValue=7134.0f/60;root.generatedTime=root.start;
        const Num original{.time=root.start,.mealptr=Num::sportbit|group,.value=root.generatedValue,.type=3};
        const a::Rule distance{8,4,a::Distance,0};
        auto measurement=a::measurementFor(store,group,a::amountMetric(root,distance,true));
        assert(measurement==&root); // Garmin takes precedence over a slightly longer Fit interval.
        auto updated=a::updateAmount(root,original,*measurement,distance,true);
        assert(updated.number.type==4&&near(updated.number.value,29.6f)&&updated.metric==a::Distance);
        assert(updated.number.time==original.time&&updated.number.mealptr==original.mealptr);
        assert(near(root.values[a::Metres],29562.3f)); // Raw data remains unrounded.

        // Future-only mapping changes keep the existing Amount's measure.
        auto futureOnly=a::updateAmount(root,original,root,distance,false);
        assert(futureOnly.number.type==3&&near(futureOnly.number.value,118.9f)&&futureOnly.metric==a::Duration);
        // Repair an Amount already relabeled by the previous implementation.
        auto relabeled=root;relabeled.generatedLabel=4;
        auto bike=original;bike.type=4;
        auto repaired=a::updateAmount(relabeled,bike,root,distance,true);
        assert(near(repaired.number.value,29.6f)&&repaired.metric==a::Distance);
        // Equal numeric values must still change the saved metric.
        auto equal=root;equal.values[a::Metres]=118900;
        assert(a::updateAmount(root,original,equal,distance,true).metric==a::Distance);

        // Select a source by the requested measure, not the old duration.
        root.values[a::Metres]=a::missing;
        assert(a::measurementFor(store,group,a::Distance)==&fit);
        assert(near(a::updateAmount(root,original,fit,distance,true).number.value,29.2f));
        fit.flags|=a::NeedsDetails;
        auto unavailable=a::updateAmount(root,original,*a::measurementFor(store,group,a::Distance),distance,true);
        assert(unavailable.number.type==original.type&&unavailable.number.value==original.value&&unavailable.metric==a::Duration);
        fit.flags=0;root.values[a::Metres]=29562.3f;

        // Individually edited fields and manually entered Amounts stay intact.
        auto edited=root;auto manual=original;manual.type=7;manual.value=42;manual.time+=120;
        auto preserved=a::updateAmount(edited,manual,root,distance,true);
        assert(preserved.number.type==7&&preserved.number.value==42&&preserved.number.time==manual.time);
        assert((edited.flags&(a::LabelOverride|a::ValueOverride|a::TimeOverride))==(a::LabelOverride|a::ValueOverride|a::TimeOverride));
        edited=root;manual=original;manual.type=7;
        preserved=a::updateAmount(edited,manual,root,distance,true);
        assert(preserved.number.type==7&&near(preserved.number.value,29.6f));
        edited=root;edited.flags=a::Linked;
        preserved=a::updateAmount(edited,original,root,distance,true);
        assert(preserved.number.type==3&&preserved.number.value==original.value);
        ++root.start;
        assert(a::updateAmount(root,original,root,distance,true).number.time==original.time);

        // Persist the applied metric; the next retrieval must not revert to minutes.
        root.generatedLabel=updated.number.type;root.generatedValue=updated.number.value;
        root.generatedMetric=updated.metric;root.generatedTime=updated.number.time;
        root.values[a::Metres]=30040;
        updated=a::updateAmount(root,updated.number,root,distance,false);
        assert(near(updated.number.value,30.0f)&&updated.metric==a::Distance&&updated.number.time==root.start);
        assert(near(a::amountValue(root,a::Speed),15.5f));
        assert(near(a::amountValue(root,a::SpeedMiles),9.6f));
        root.values[a::Metres]=29562.3f;
        assert(near(a::amountValue(root,a::DistanceMiles),18.4f));
        root.values[a::StepCount]=1234.4f;
        assert(a::amountValue(root,a::Steps)==1234);
    }
    {
        a::Store store(directory);
        assert(store.get(1)->generatedMetric==a::Distance);
        assert(near(store.get(1)->values[a::Metres],29562.3f));
    }
    std::filesystem::remove_all(directory);
    std::puts("Activity mapping, rounding, source selection, overrides and units: passed");
}
