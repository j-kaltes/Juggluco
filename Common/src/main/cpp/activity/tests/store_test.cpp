// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone Linux test: clang++ -std=c++26 -pthread store_test.cpp -o store_test
#include "../store.hpp"
#include "../../nums/num.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

namespace a=juggluco::activity;
static bool near(float lhs,float rhs) { return std::abs(lhs-rhs)<0.001f; }
static a::Record session(uint32_t start,uint32_t end,int type=8) {
    a::Record r{};r.start=start;r.end=end;r.type=type;r.modified=int64_t(start)*1000;return r;
}
int main() {
    static_assert(sizeof(Num)==16 && offsetof(Num,mealptr)==4 && offsetof(Num,type)==12);
    char directory[]="/tmp/juggluco-activities-XXXXXX";
    assert(mkdtemp(directory));
    const uint32_t start=1700000000;
    uint32_t first;
    {
        a::Store s(directory);
        assert(s.header().options==0); // No Amount creation and metric units until explicitly changed.
        first=s.upsert("session-1","watch.one","Biking","Original",session(start,start+1200));
        assert(first==1 && s.header().count==1 && s.valid(*s.get(first)));
        assert(near(a::Store::amount(*s.get(first),a::Duration),20));
        assert(std::isnan(a::Store::amount(*s.get(first),a::Distance)));
        s.get(first)->flags|=a::Linked|a::Generated|a::LabelOverride;
        s.get(first)->generatedLabel=3;s.get(first)->generatedValue=20;

        // Exact identity is stable through replay and source corrections.
        auto update=session(start,start+1260);update.modified+=2000;
        assert(s.upsert("session-1","watch.one","Corrected","Notes",update)==first);
        assert(s.header().count==1 && s.get(first)->revision==2);
        assert((s.get(first)->flags&(a::Linked|a::Generated|a::LabelOverride))==(a::Linked|a::Generated|a::LabelOverride));
        assert(s.get(first)->generatedLabel==3 && near(s.get(first)->generatedValue,20));
        s.upsert("session-1","watch.one","Stale","",session(start,start+1200));
        assert(s.text(s.get(first)->title)=="Corrected" && s.get(first)->revision==2);

        // Two devices describing the same session share one Amount, but retain
        // separate raw HR and metrics. Same-source duplicates merge too.
        auto second=s.upsert("session-2","watch.two","","",session(start+30,start+1230));
        assert(second==2 && s.get(second)->group==first);
        auto third=s.upsert("session-3","watch.one","","",session(start,start+1260));
        assert(third==3 && s.get(third)->group==first);
        auto ambiguous=s.upsert("ambiguous","watch.three","","",session(start,start+1260));
        assert(s.get(ambiguous)->group==first);
        auto unrelated=s.upsert("other","watch.three","","",session(start+7200,start+8400));
        assert(s.get(unrelated)->group==unrelated);

        const int64_t ms=int64_t(start)*1000;
        a::Sample heart[]={{ms,100,0},{ms+60000,160,0},{ms+660000,120,0}};
        s.samples(first,0,heart);
        // 60 s at 100 + 120 s at 160 + 120 s at 120, not a filled ten-minute gap.
        assert(near(s.get(first)->values[a::AverageHR],132));
        assert(near(s.get(first)->values[a::DeviationHR],24));
        assert(near(s.get(first)->values[a::HRCoverage],300));
        a::Sample otherHeart[]={{ms,80,0},{ms+60000,90,0}};
        s.samples(second,0,otherHeart);
        assert(near(s.get(first)->values[a::AverageHR],132));
        assert(near(s.get(second)->values[a::AverageHR],88)); // clipped at this source's start
        const auto oldSlice=s.get(first)->heart;
        s.samples(first,0,heart); // Identical retries must not append another copy.
        assert(s.get(first)->heart.offset==oldSlice.offset);
        a::Sample speed[]={{ms,0,0},{ms+60000,10,0}};
        s.samples(first,1,speed);
        assert(near(a::Store::amount(*s.get(first),a::Speed),24));
        a::Sample rest[]={{ms-3000,60,0},{ms-2000,80,0},{ms-1000,65,0},{ms+1000,200,0}};
        s.samples(first,4,rest);
        assert(near(s.get(first)->values[a::RestHR],65));
        assert(near(a::Store::amount(*s.get(first),a::ExcessHR),67));
        assert(s.view<a::Sample>(s.get(first)->rest).size()==4);
        assert(std::isnan(a::Store::statistics({},ms,ms+1000)[0]));
        assert(s.view<a::Sample>({UINT32_MAX-7,UINT32_MAX}).empty());
        auto watchA=s.source("one.app","Watch A"),watchB=s.source("one.app","Watch B");
        assert(watchA!=watchB && s.source("one.app","Watch A")==watchA);
        s.get(unrelated)->source=watchB;
        const int64_t later=int64_t(s.get(unrelated)->start)*1000;
        a::Sample both[]={{later,80,watchA},{later+60000,90,watchA},{later,120,watchB},{later+60000,150,watchB}};
        s.samples(unrelated,0,both);
        assert(s.view<a::Sample>(s.get(unrelated)->heart).size()==4);
        assert(near(s.get(unrelated)->values[a::AverageHR],140));

        s.header().defaultLabel=3;s.header().from=start;s.header().interval=900;
        s.header().options=a::CreateAmounts|a::UseMiles;
        s.setRule({8,4,a::Distance,0});s.token(0,"cursor-after-completed-records");
        s.token(1,"completed-before-work");s.checkpoint(1,"pending-after-changes",true);
        assert(s.token(1)=="pending-after-changes" && s.text(s.header().tokens[1])=="completed-before-work");
        assert(s.pendingToken(1)=="pending-after-changes" && s.pendingToken(0).empty());
        s.get(first)->flags&=~a::NeedsDetails; // This activity finished before an interruption.
        s.upsert("session-1","watch.one","Corrected","Notes",update);
        assert(!(s.get(first)->flags&a::NeedsDetails)); // Replaying metadata keeps that progress.
        // Exercise both mappings growing beyond their original 64 KiB region.
        std::string longNotes(90000,'x');
        for(int i=0;i<400;++i)s.upsert("grown-"+std::to_string(i),"watch.one","",i==0?longNotes:"",session(start+10000+i*2000,start+11000+i*2000));
        assert(s.text(s.get(first)->id)=="session-1");

        // The Num wire layout is unchanged. The high-bit reference is opaque
        // when carried as a signed Java/Garmin integer and returned to C++.
        Num original{.time=start,.mealptr=Num::sportbit|first,.value=20,.type=3};
        std::array<std::byte,sizeof(Num)> bytes;
        std::memcpy(bytes.data(),&original,sizeof original);
        Num returned{};std::memcpy(&returned,bytes.data(),sizeof returned);
        const auto javaInt=std::bit_cast<int32_t>(returned.mealptr);
        assert(javaInt<0 && static_cast<uint32_t>(javaInt)==original.mealptr);
        assert(a::indexOf(returned.mealptr)==first && returned.type==3);
    }
    {
        // Direct reuse after remapping: no save/load serialization step.
        a::Store s(directory);
        assert(s.header().options==(a::CreateAmounts|a::UseMiles));
        assert(s.header().count==405 && s.header().from==start && s.header().interval==900);
        assert(s.rule(8).label==4 && s.rule(8).metric==a::Distance);
        assert(s.rule(999).label==3 && s.rule(999).metric==a::Duration);
        assert(s.text(s.header().tokens[0])=="cursor-after-completed-records");
        assert(s.token(1)=="pending-after-changes" && !(s.get(first)->flags&a::NeedsDetails));
        assert(s.pendingToken(1)=="pending-after-changes"); // Resume this batch after restarting.
        assert(s.get(2)->flags&a::NeedsDetails);
        s.token(1,"fully-completed");assert(s.pendingToken(1).empty());
        s.beginSessionSnapshot();
        assert(s.get(first)->flags&a::NotReturned);
        s.upsert("session-1","watch.one","Corrected","Notes",*s.get(first));
        assert(!(s.get(first)->flags&a::NotReturned) && (s.get(2)->flags&a::NotReturned));
        assert(near(s.get(2)->values[a::AverageHR],88)); // Access changes keep previously saved data.
        assert(s.text(s.get(first)->title)=="Corrected");
        assert(s.view<a::Sample>(s.get(first)->heart)[1].time==int64_t(start)*1000+60000);
        assert(near(s.get(first)->values[a::AverageHR],132));
        assert(s.text(s.get(6)->notes).size()==90000);
        s.resetTokens();assert(s.text(s.header().tokens[0]).empty());
        // A published record with an unavailable payload cannot be followed.
        auto damaged=*s.get(first);damaged.id={UINT32_MAX-1,64};
        assert(!s.valid(damaged));
        // An older file has zeroes in the unused options slot, but its former
        // tail padding need not be zero. Upgrading it must not imply consent.
        s.header().options=0;
        std::memset(reinterpret_cast<char*>(&s.header())+4404,0xff,4);
    }
    {
        a::Store s(directory);
        assert(s.header().options==0&&s.header().count==405);
        assert(s.rule(8).label==4&&s.rule(8).metric==a::Distance);
    }
    std::filesystem::remove_all(directory);
    std::puts("Activity storage, replay, sources, statistics, remapping and Num layout: passed");
}
