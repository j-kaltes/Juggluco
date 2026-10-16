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
// Native, little-endian mmap storage. Indices are permanent; zero means absent.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <time.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace juggluco::activity {
constexpr uint32_t referenceBit=0x80000000u;
constexpr bool isReference(uint32_t value) { return (value&referenceBit)!=0; }
constexpr uint32_t indexOf(uint32_t value) { return value&~referenceBit; }
constexpr size_t recordStart=8192;
constexpr float missing=std::numeric_limits<float>::quiet_NaN();
enum Metric : int32_t { Duration, Distance, ExcessHR, MeanHR, HRSD, Calories,
                       Speed, Steps, Elevation, Power, Cadence,
                       DistanceMiles, SpeedMiles, MaxHR, MetricCount };
enum Field : size_t { Seconds, Metres, RestHR, AverageHR, DeviationHR, HRCoverage,
                     Kcal, MetresPerSecond, StepCount, ElevationMetres, Watts, RPM, FieldCount };
enum : uint32_t { Deleted=1, Linked=2, Generated=4, LabelOverride=8, ValueOverride=16,
                  NeedsDetails=32, Ambiguous=64, TimeOverride=128, NotReturned=256,
                  LocallyDeleted=512, Filtered=1024 };
enum Option : uint64_t { CreateAmounts=1, UseMiles=2 };
struct Slice { uint32_t offset=0, count=0; };
struct alignas(8) Sample { int64_t time=0; float value=0; uint32_t source=0; };
struct Source { Slice origin, device; };
struct RemovedSource {uint32_t group=0,source=0;};
// Zero retains the original precision (whole steps, tenths for other measures).
struct Rule { int32_t type=0, label=-1, metric=Duration; float roundTo=0; };
struct alignas(8) Record {
    uint32_t group=0, flags=0, type=0, start=0, end=0, revision=0;
    int64_t modified=0;
    Slice id, origin, title, notes, heart, speed, power, cadence, rest;
    std::array<float,FieldCount> values;
    int32_t generatedLabel=-1, generatedMetric=Duration;
    float generatedValue=missing;
    uint32_t generatedTime=0;
    uint32_t source=0;
    Slice amountBinding;
    float maximumHR=missing; // Uses the last previously reserved word.
};
struct alignas(8) Header {
    uint64_t magic=0;
    uint32_t version=1, count=0, ruleCount=0, interval=0;
    uint32_t from=0, lastSuccess=0;
    int32_t defaultLabel=-1;
    uint32_t generation=0;
    Rule rules[256];
    Slice tokens[16];
    Slice sources;
    Slice pendingTokens[16];
    uint32_t readPermissions=0;
    // Past the old header, in previously unused zero-filled space. Upgrades
    // require new, explicit consent too; the old tail padding is not a flag.
    alignas(8) uint64_t options=0;
    uint32_t detailsVersion=0;
    // Beyond the old header's tail padding, in unused mapped space.
    alignas(8) uint64_t removalVersion=0;
    uint64_t amountBindingVersion=0;
    Slice removedSources;
    uint64_t mirrorEpoch=0, mirrorRevision=0, receivedMirror=0, maximumVersion=0;
    uint32_t receivedDataSize=0, mirrorReserved=0;
};
static_assert(std::endian::native==std::endian::little);
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
static_assert(sizeof(Slice)==8 && sizeof(Sample)==16 && sizeof(Rule)==16);
static_assert(offsetof(Rule,roundTo)==12);
static_assert(sizeof(Record)==184 && offsetof(Record,values)==104);
static_assert(offsetof(Record,source)==168&&offsetof(Record,amountBinding)==172);
static_assert(offsetof(Record,maximumHR)==180);
static_assert(sizeof(Header)<recordStart);
static_assert(offsetof(Header,options)==4408);
static_assert(offsetof(Header,detailsVersion)==4416);
static_assert(offsetof(Header,removalVersion)==4424);
static_assert(offsetof(Header,amountBindingVersion)==4432);
static_assert(offsetof(Header,removedSources)==4440&&offsetof(Header,mirrorEpoch)==4448);
static_assert(std::is_trivially_copyable_v<Record> && std::is_standard_layout_v<Record>);

class Mapping {
    int fd=-1;
    void *address=MAP_FAILED;
    size_t length=0;
public:
    Mapping(const std::string &path,size_t minimum) {
        fd=open(path.c_str(),O_RDWR|O_CREAT|O_CLOEXEC,0600);
        if(fd<0) throw std::system_error(errno,std::generic_category(),path);
        try { struct stat st{};
            if(fstat(fd,&st)) throw std::system_error(errno,std::generic_category(),"activity stat");
            grow(std::max(minimum,static_cast<size_t>(st.st_size)));
        } catch(...) { close(fd); throw; }
    }
    Mapping(const Mapping&)=delete;
    ~Mapping() { if(address!=MAP_FAILED) munmap(address,length); if(fd>=0) close(fd); }
    void grow(size_t wanted) {
        if(wanted<=length) return;
        const size_t size=(wanted+65535u)&~size_t(65535u);
        if(size>UINT32_MAX) throw std::length_error("Activity file is full");
        if(ftruncate(fd,static_cast<off_t>(size))) throw std::system_error(errno,std::generic_category(),"activity grow");
        void *next=mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
        if(next==MAP_FAILED) throw std::system_error(errno,std::generic_category(),"activity mmap");
        if(address!=MAP_FAILED) munmap(address,length);
        address=next; length=size;
    }
    std::byte *data() const { return static_cast<std::byte*>(address); }
    size_t size() const { return length; }
};

class Store {
    Mapping records, data;
    static constexpr uint64_t magic=0x3159544956495441ULL; // ACTIVIT Y, format 1
    uint32_t &used() { return *reinterpret_cast<uint32_t*>(data.data()+4); }
public:
    std::recursive_mutex mutex;
    explicit Store(const std::string &directory):records(directory+"/activities.dat",recordStart),
            data(directory+"/activity-data.dat",65536) {
        if(header().magic==0) { header()=Header{}; header().magic=magic; }
        if(header().magic!=magic||header().version!=1) throw std::runtime_error("Unsupported activity file");
        if(*reinterpret_cast<uint32_t*>(data.data())==0) {
            *reinterpret_cast<uint32_t*>(data.data())=0x31445641; used()=8;
        }
        if(*reinterpret_cast<uint32_t*>(data.data())!=0x31445641||used()<8||used()>data.size())
            throw std::runtime_error("Invalid activity data file");
        if(header().count>(records.size()-recordStart)/sizeof(Record))
            throw std::runtime_error("Invalid activity record count");
        if(header().ruleCount>std::size(header().rules)) throw std::runtime_error("Invalid activity mappings");
        if(header().detailsVersion<1) {
            // Old imports only queried HR from the session's app. Re-read
            // cached activities once, even when no Health Connect record changed.
            // Publish the pending work before the upgrade marker. Keep cursors,
            // cached samples, Amount references and user edits intact.
            for(uint32_t i=1;i<=header().count;++i)get(i)->flags|=NeedsDetails;
            std::atomic_signal_fence(std::memory_order_release);
            header().detailsVersion=1;
        }
        if(!header().mirrorEpoch) {
            uint64_t epoch=0;
            const int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC);
            if(random>=0){const auto result=read(random,&epoch,sizeof(epoch));close(random);if(result!=sizeof(epoch))epoch=0;}
            if(!epoch){timespec now{};clock_gettime(CLOCK_REALTIME,&now);epoch=uint64_t(now.tv_sec)*1000000000+now.tv_nsec+getpid();}
            header().mirrorEpoch=epoch?epoch:1;header().mirrorRevision=1;
        }
        if(!header().maximumVersion) {
            for(uint32_t i=1;i<=header().count;++i) {
                auto &r=*get(i);
                r.maximumHR=statistics(heartSamples(r),int64_t(r.start)*1000,int64_t(r.end)*1000)[3];
            }
            header().maximumVersion=1;changed();
        }
    }
    Header &header() { return *reinterpret_cast<Header*>(records.data()); }
    const Header &header() const {return *reinterpret_cast<const Header*>(records.data());}
    void changed(){if(++header().mirrorRevision==0)++header().mirrorRevision;}
    std::span<const std::byte> recordBytes() const {return {records.data(),recordStart+size_t(header().count)*sizeof(Record)};}
    std::span<const std::byte> dataBytes() const {return {data.data(),*reinterpret_cast<const uint32_t*>(data.data()+4)};}
    // A complete, validated mirror image is staged before either mapping changes.
    // Keep mappings alive for JNI/exporters; growing them refreshes their addresses.
    void receiveImage(std::span<const std::byte> incomingRecords,std::span<const std::byte> incomingData) {
        const uint32_t generation=header().generation;
        records.grow(incomingRecords.size());data.grow(incomingData.size());
        memcpy(data.data(),incomingData.data(),incomingData.size());
        memcpy(records.data(),incomingRecords.data(),incomingRecords.size());
        header().receivedMirror=1;header().receivedDataSize=incomingData.size();
        header().generation=std::max(generation,header().generation)+1;
        for(auto &token:header().tokens)token={};for(auto &token:header().pendingTokens)token={};
        header().readPermissions=0; // Health Connect cursors/permissions belong to this installation.
    }
    Record *get(uint32_t index) {
        if(index==0||index>header().count) return nullptr;
        return reinterpret_cast<Record*>(records.data()+recordStart)+(index-1);
    }
    const Record *get(uint32_t index) const {return const_cast<Store*>(this)->get(index);}
    bool valid(const Record &r) const {
        return r.group>0&&r.group<=header().count&&r.start>0&&r.end>r.start&&
               !text(r.id).empty()&&!text(r.origin).empty();
    }
    template<class T> std::span<const T> view(Slice s) const {
        const auto end=uint64_t(s.offset)+uint64_t(s.count)*sizeof(T);
        auto limit=*reinterpret_cast<const uint32_t*>(data.data()+4);
        if(s.offset<8||s.offset%alignof(T)||end>limit||end>data.size()) return {};
        return {reinterpret_cast<const T*>(data.data()+s.offset),s.count};
    }
    std::string_view text(Slice s) const {
        auto bytes=view<char>(s); return {bytes.data(),bytes.size()};
    }
    template<class T> Slice append(std::span<const T> input) {
        if(input.empty()) return {};
        const uint64_t offset=(uint64_t(used())+alignof(T)-1)&~uint64_t(alignof(T)-1);
        const uint64_t end=offset+input.size_bytes();
        if(end>UINT32_MAX||input.size()>UINT32_MAX) throw std::length_error("Activity data is full");
        data.grow(static_cast<size_t>(end));
        memcpy(data.data()+offset,input.data(),input.size_bytes());
        std::atomic_signal_fence(std::memory_order_release);
        used()=static_cast<uint32_t>(end);
        changed();
        return {static_cast<uint32_t>(offset),static_cast<uint32_t>(input.size())};
    }
    Slice append(std::string_view s) { return append<char>({s.data(),s.size()}); }
    void token(size_t type,std::string_view token) {
        if(type>=std::size(header().tokens)) throw std::out_of_range("Activity token type");
        if(text(header().tokens[type])!=token) header().tokens[type]=append(token);
        header().pendingTokens[type]={};
        changed();
    }
    std::string_view pendingToken(size_t type) {
        if(type>=std::size(header().tokens))throw std::out_of_range("Activity token type");
        return text(header().pendingTokens[type]);
    }
    std::string_view token(size_t type) {
        auto pending=pendingToken(type);
        return pending.empty()?text(header().tokens[type]):pending;
    }
    void checkpoint(size_t type,std::string_view token,bool dirty) {
        if(type>=std::size(header().tokens))throw std::out_of_range("Activity token type");
        if(dirty)for(uint32_t i=1;i<=header().count;++i)if(!(get(i)->flags&(Deleted|NotReturned)))get(i)->flags|=NeedsDetails;
        // Record the remaining work before remembering how far the change list
        // was read. The completed cursor is published after details and linking.
        std::atomic_signal_fence(std::memory_order_release);
        if(text(header().pendingTokens[type])!=token)header().pendingTokens[type]=append(token);
        changed();
    }
    uint32_t markDetails(uint32_t kind,int64_t from,int64_t to,std::string_view origin) {
        if(kind<1||kind>9||to<=from)throw std::invalid_argument("Invalid activity metric interval");
        uint32_t count=0;
        for(uint32_t i=1;i<=header().count;++i) {
            auto &r=*get(i);const auto group=groupOf(i);
            if((r.flags&(Deleted|NotReturned))||r.start<header().from||!group||
                    (get(group)->flags&LocallyDeleted)||sourceRemoved(group,r.source))continue;
            // HR and resting HR may come from another app/watch. Other values
            // are aggregated/read only from the exercise session's own app.
            if(kind!=1&&kind!=2&&text(r.origin)!=origin)continue;
            const int64_t start=int64_t(r.start)*1000;
            const int64_t begin=kind==2?start-7LL*86400*1000:start;
            const int64_t end=kind==2?start+1:int64_t(r.end)*1000;
            if(from<end&&to>begin&&!(r.flags&NeedsDetails)) {r.flags|=NeedsDetails;++count;}
        }
        if(count)changed();
        return count;
    }
    uint32_t markAllDetails() {return markDetails(1,0,std::numeric_limits<int64_t>::max(),{});}
    void resetTokens() { for(auto &t:header().tokens)t={};for(auto &t:header().pendingTokens)t={};changed(); }
    void beginSessionSnapshot() {
        // A full query may omit old records after history access is revoked.
        // Retain their saved details instead of overwriting them with empty reads.
        for(uint32_t i=1;i<=header().count;++i)get(i)->flags|=NotReturned;
        changed();
    }
    uint32_t source(std::string_view origin,std::string_view device) {
        const auto previous=view<Source>(header().sources);
        for(size_t i=0;i<previous.size();++i)
            if(text(previous[i].origin)==origin&&text(previous[i].device)==device)return i+1;
        std::vector<Source> all(previous.begin(),previous.end());
        const Slice app=append(origin),model=append(device);
        all.push_back({app,model});
        header().sources=append<Source>(all);
        return all.size();
    }
    Source source(uint32_t id) const {
        auto all=view<Source>(reinterpret_cast<const Header*>(records.data())->sources);
        return id>0&&id<=all.size()?all[id-1]:Source{};
    }
    uint32_t find(std::string_view id,std::string_view origin) {
        for(uint32_t i=1;i<=header().count;++i) {
            auto &r=*get(i); if(text(r.id)==id&&text(r.origin)==origin) return i;
        }
        return 0;
    }
    uint32_t groupOf(uint32_t index) const {
        // Old Amount/Kerfstok references remain aliases of the merged group.
        // Follow parents as well as flattened groups, including after a restart
        // between publishing a merge and flattening its other members.
        for(uint32_t steps=0;steps<header().count;++steps) {
            auto *r=get(index);if(!r||!valid(*r))return 0;
            if(r->group==index)return index;
            index=r->group;
        }
        return 0;
    }
    bool sourceRemoved(uint32_t group,uint32_t source) const {
        group=groupOf(group);
        for(const auto &removed:view<RemovedSource>(header().removedSources))
            if(removed.source==source&&groupOf(removed.group)==group)return true;
        return false;
    }
    bool sourceVisible(uint32_t index) {
        const auto *r=get(index);if(!r||!valid(*r))return false;
        if((r->flags&Deleted)||((r->flags&Filtered)&&!(r->flags&NeedsDetails)))return false;
        const auto group=groupOf(index);
        return group&&!(get(group)->flags&LocallyDeleted)&&!sourceRemoved(group,r->source);
    }
    bool groupVisible(uint32_t index) {
        const auto group=groupOf(index);
        if(!group||(get(group)->flags&LocallyDeleted))return false;
        for(uint32_t i=1;i<=header().count;++i) {
            if(groupOf(i)==group&&sourceVisible(i))return true;
        }
        return false;
    }
    uint32_t removeGroup(uint32_t index) {
        const auto group=groupOf(index);
        // Keep permanent IDs as tombstones for replay and old Kerfstok aliases.
        if(group&&!(get(group)->flags&LocallyDeleted)){get(group)->flags|=LocallyDeleted;changed();}
        return group;
    }
    bool removeSource(uint32_t group,uint32_t sourceID) {
        group=groupOf(group);
        if(!group||!sourceID||sourceID>view<Source>(header().sources).size())return false;
        if(sourceRemoved(group,sourceID))return true;
        const auto old=view<RemovedSource>(header().removedSources);
        std::vector<RemovedSource> removed(old.begin(),old.end());removed.push_back({group,sourceID});
        header().removedSources=append<RemovedSource>(removed);
        for(uint32_t i=1;i<=header().count;++i)if(groupOf(i)==group) {
            auto &r=*get(i);const auto selected=heartSamples(r);
            const auto stats=statistics(selected,int64_t(r.start)*1000,int64_t(r.end)*1000);
            r.values[AverageHR]=stats[0];r.values[DeviationHR]=stats[1];r.values[HRCoverage]=stats[2];r.maximumHR=stats[3];
            restingHeartRate(r,selected);
        }
        filterRecords();changed();return true;
    }
    bool hasInformation(const Record &r) const {
        if(!text(r.title).empty()||!text(r.notes).empty())return true;
        for(size_t field=1;field<FieldCount;++field)
            if(field!=HRCoverage&&std::isfinite(r.values[field])&&r.values[field]!=0)return true;
        for(const auto slice:{r.heart,r.speed,r.power,r.cadence})for(const auto &sample:view<Sample>(slice))
            if(!sourceRemoved(r.group,sample.source)&&sample.value!=0&&
                sample.time>=int64_t(r.start)*1000-120000&&sample.time<int64_t(r.end)*1000)return true;
        return false;
    }
    int sourceRank(const Record &r) const {
        const auto origin=text(r.origin);
        // Prefer workout-app recordings, including Garmin, to a parallel
        // automatic Fit recording. Compare other apps by completeness.
        return origin=="com.google.android.apps.fitness"?0:1;
    }
    int informationScore(const Record &r) const {
        int score=int(!text(r.title).empty())+int(!text(r.notes).empty());
        for(size_t field=1;field<FieldCount;++field)
            score+=field!=HRCoverage&&std::isfinite(r.values[field])&&r.values[field]!=0;
        return score+bool(r.heart.count)+bool(r.speed.count)+bool(r.power.count)+bool(r.cadence.count);
    }
    bool preferSource(const Record &candidate,const Record &current) const {
        const int a=sourceRank(candidate),b=sourceRank(current);
        if(a!=b)return a>b;
        const int x=informationScore(candidate),y=informationScore(current);
        return x!=y?x>y:candidate.end-candidate.start>current.end-current.start;
    }
    bool coversInformation(const Record &r,const Record &other) const {
        if(!text(r.title).empty()&&text(r.title)!=text(other.title))return false;
        if(!text(r.notes).empty()&&text(r.notes)!=text(other.notes))return false;
        constexpr std::array<float,FieldCount> precision={0,50,.05f,.05f,.05f,0,.05f,.05f/3.6f,.5f,.05f,.05f,.05f};
        for(size_t field=1;field<FieldCount;++field) {
            const float value=r.values[field];
            if(field==HRCoverage||!std::isfinite(value)||value==0)continue;
            if((field==AverageHR||field==DeviationHR)&&r.heart.count)continue;
            if((field==MetresPerSecond&&r.speed.count)||(field==Watts&&r.power.count)||(field==RPM&&r.cadence.count))continue;
            if(!std::isfinite(other.values[field])||std::abs(other.values[field]-value)>precision[field])return false;
        }
        const std::array<Slice,4> slices={r.heart,r.speed,r.power,r.cadence},others={other.heart,other.speed,other.power,other.cadence};
        for(size_t kind=0;kind<slices.size();++kind) {
            const auto available=view<Sample>(others[kind]);
            for(const auto &sample:view<Sample>(slices[kind])) {
                if(sourceRemoved(r.group,sample.source)||sample.value==0||sample.time<int64_t(r.start)*1000-120000||sample.time>=int64_t(r.end)*1000)continue;
                const auto part=forSource(available,sample.source);
                auto first=std::lower_bound(part.begin(),part.end(),sample.time,[](const auto &s,int64_t time){return s.time<time;});
                bool found=false;
                for(;first!=part.end()&&first->time==sample.time;++first)if(first->value==sample.value){found=true;break;}
                if(!found)return false;
            }
        }
        return true;
    }
    void filterRecords(std::span<const uint32_t> durationAmounts={}) {
        std::vector<uint32_t> order;std::vector<bool> filtered(header().count+1);
        for(uint32_t i=1;i<=header().count;++i) {
            const auto &r=*get(i);
            if(!valid(r)||(r.flags&(Deleted|NeedsDetails))||sourceRemoved(r.group,r.source))continue;
            const bool duration=rule(r.type).metric==Duration||
                std::find(durationAmounts.begin(),durationAmounts.end(),groupOf(i))!=durationAmounts.end();
            if(!hasInformation(r)&&!duration)filtered[i]=true;
            else order.push_back(i);
        }
        std::sort(order.begin(),order.end(),[&](uint32_t a,uint32_t b){
            const auto &left=*get(a),&right=*get(b);const auto x=informationScore(left),y=informationScore(right);
            if(x!=y)return x>y;
            const int ar=sourceRank(left),br=sourceRank(right);
            if(ar!=br)return ar>br;
            return left.end-left.start!=right.end-right.start?left.end-left.start>right.end-right.start:a<b;
        });
        std::vector<uint32_t> kept;
        for(auto index:order) {
            const auto &r=*get(index);
            for(auto otherIndex:kept) {
                const auto &other=*get(otherIndex);
                if(groupOf(index)!=groupOf(otherIndex)||text(r.origin)==text(other.origin))continue;
                const bool contained=r.start>=other.start&&r.end<=other.end;
                const bool close=std::abs(int64_t(r.start)-other.start)<=60&&std::abs(int64_t(r.end)-other.end)<=60;
                if((contained||close)&&coversInformation(r,other)){filtered[index]=true;break;}
            }
            if(!filtered[index])kept.push_back(index);
        }
        bool updated=false;
        for(uint32_t i=1;i<=header().count;++i) {
            auto &r=*get(i);const auto flags=filtered[i]?r.flags|Filtered:r.flags&~Filtered;
            if(flags!=r.flags){r.flags=flags;updated=true;}
        }
        if(updated)changed();
    }
    void recoverRemovedAmounts(std::span<const uint32_t> liveReferences) {
        if(header().removalVersion>=1)return;
        std::vector<bool> present(header().count+1);
        for(auto index:liveReferences)if(auto group=groupOf(index))present[group]=true;
        for(uint32_t i=1;i<=header().count;++i) {
            auto *r=get(i);
            if(r->group==i&&(r->flags&Linked)&&!present[i]&&groupVisible(i))removeGroup(i);
        }
        // Only infer old deletions during upgrade. Later source deletions may
        // also remove generated Amounts; those are not a local deletion request.
        std::atomic_signal_fence(std::memory_order_release);
        header().removalVersion=1;
    }
    uint32_t mergeOverlaps() {
        std::vector<uint32_t> order;
        for(uint32_t i=1;i<=header().count;++i) {
            auto &r=*get(i);
            // Deleted/omitted source records can still own a saved Amount.
            // Include their intervals so old zero-valued duplicates are repaired
            // too; they remain excluded when choosing current measurements.
            if(valid(r)&&groupOf(i))order.push_back(i);
        }
        std::sort(order.begin(),order.end(),[&](uint32_t a,uint32_t b) {
            return get(a)->start!=get(b)->start?get(a)->start<get(b)->start:a<b;
        });
        uint32_t merged=0;
        auto join=[&](uint32_t a,uint32_t b) {
            a=groupOf(a);b=groupOf(b);
            if(a==b)return a;
            const auto keep=std::min(a,b),alias=std::max(a,b);
            get(keep)->flags|=get(alias)->flags&(Linked|LocallyDeleted);
            std::atomic_signal_fence(std::memory_order_release);
            get(alias)->group=keep; // Parent indices only decrease.
            ++merged;return keep;
        };
        uint32_t group=0,end=0;
        for(auto index:order) {
            const auto &r=*get(index);
            auto next=groupOf(index);
            // A containing recording can join several shorter app segments.
            if(group&&r.start<end) {
                group=join(group,next);
                end=std::max(end,r.end);
            } else {group=next;end=r.end;}
        }
        // Apps can split the same workout at a handover: Fit's short fragment
        // ends where Strava/Garmin starts, sometimes with a few seconds' gap.
        // Require the same type and different apps; adjacent recordings from
        // one app, or a change of activity type, remain separate.
        constexpr uint32_t handoverSeconds=60;
        auto endings=order;
        std::sort(endings.begin(),endings.end(),[&](uint32_t a,uint32_t b){return get(a)->end<get(b)->end;});
        size_t first=0;
        for(auto index:order) {
            const auto &r=*get(index);
            while(first<endings.size()&&uint64_t(get(endings[first])->end)+handoverSeconds<r.start)++first;
            for(size_t p=first;p<endings.size();++p) {
                const auto &previous=*get(endings[p]);
                if(previous.end>r.start)break;
                if(previous.type==r.type&&text(previous.origin)!=text(r.origin))join(index,endings[p]);
            }
        }
        for(uint32_t i=1;i<=header().count;++i)if(auto group=groupOf(i))get(i)->group=group;
        if(merged)changed();
        return merged;
    }
    uint32_t representative(uint32_t group) {
        group=groupOf(group);
        uint32_t best=0;
        for(uint32_t i=1;i<=header().count;++i) {
            const auto &r=*get(i);
            if(r.group!=group||!sourceVisible(i))continue;
            const bool current=!(r.flags&NotReturned);
            const bool bestCurrent=best&&!(get(best)->flags&NotReturned);
            if(!best||(current&&!bestCurrent)||(current==bestCurrent&&preferSource(r,*get(best))))best=i;
        }
        return best;
    }
    std::array<uint32_t,2> interval(uint32_t group) {
        group=groupOf(group);
        uint32_t start=UINT32_MAX,end=0;
        for(uint32_t i=1;i<=header().count;++i) {
            const auto &r=*get(i);
            if(r.group!=group||!sourceVisible(i)||(r.flags&NotReturned))continue;
            start=std::min(start,r.start);end=std::max(end,r.end);
        }
        if(!end) {auto *r=get(group);return r?std::array{r->start,r->end}:std::array<uint32_t,2>{};}
        return {start,end};
    }
    uint32_t elapsed(uint32_t group) {
        group=groupOf(group);
        std::vector<std::array<uint32_t,2>> intervals;
        for(uint32_t i=1;i<=header().count;++i) {
            const auto &r=*get(i);
            if(r.group==group&&sourceVisible(i)&&!(r.flags&NotReturned))intervals.push_back({r.start,r.end});
        }
        if(intervals.empty()) {auto *r=get(group);return r?r->end-r->start:0;}
        std::sort(intervals.begin(),intervals.end());
        uint32_t seconds=0,end=0;
        for(const auto &times:intervals)if(times[1]>end) {
            seconds+=times[1]-std::max(end,times[0]);end=times[1];
        }
        return seconds;
    }
    uint32_t upsert(std::string_view id,std::string_view origin,std::string_view title,
                    std::string_view notes,Record incoming) {
        if(id.empty()||origin.empty()||incoming.end<=incoming.start||incoming.start==0)
            throw std::invalid_argument("Invalid activity session");
        uint32_t index=find(id,origin);
        if(index) {
            get(index)->flags&=~NotReturned;
            const Record old=*get(index);
            if(incoming.modified>0&&old.modified>incoming.modified) return index;
            if(!(old.flags&Deleted)&&old.modified==incoming.modified&&old.start==incoming.start&&old.end==incoming.end&&
                old.type==incoming.type&&old.source==incoming.source&&text(old.title)==title&&text(old.notes)==notes)return index;
            incoming.group=old.group;
            incoming.flags=(old.flags&~Deleted)|NeedsDetails;
            incoming.generatedLabel=old.generatedLabel; incoming.generatedMetric=old.generatedMetric;
            incoming.generatedTime=old.generatedTime; incoming.generatedValue=old.generatedValue;
            incoming.amountBinding=old.amountBinding;
            incoming.heart=old.heart; incoming.speed=old.speed; incoming.power=old.power; incoming.cadence=old.cadence; incoming.rest=old.rest;
            incoming.values=old.values;
            incoming.maximumHR=old.maximumHR;
            incoming.revision=old.revision;
            incoming.id=old.id; incoming.origin=old.origin;
            incoming.title=text(old.title)==title?old.title:append(title);
            incoming.notes=text(old.notes)==notes?old.notes:append(notes);
        } else {
            index=header().count+1; incoming.group=index;
            incoming.flags=NeedsDetails; incoming.values.fill(missing);
            incoming.id=append(id); incoming.origin=append(origin);
            incoming.title=append(title); incoming.notes=append(notes);
            records.grow(recordStart+size_t(index)*sizeof(Record));
        }
        incoming.values[Seconds]=float(incoming.end-incoming.start);
        incoming.revision++;
        // Payload first, then publication. Deliberately ordinary mmap writeback.
        auto *slot=reinterpret_cast<Record*>(records.data()+recordStart)+(index-1);
        *slot=incoming;
        std::atomic_signal_fence(std::memory_order_release);
        if(index>header().count) header().count=index;
        changed();
        mergeOverlaps();
        return index;
    }
    Rule rule(int type) {
        for(uint32_t i=0;i<header().ruleCount;++i) if(header().rules[i].type==type) return header().rules[i];
        return {type,header().defaultLabel,Duration,0};
    }
    void setRule(Rule r) {
        if(r.metric<0||r.metric>=MetricCount) throw std::invalid_argument("Invalid activity measure");
        if(!std::isfinite(r.roundTo)||r.roundTo<0)throw std::invalid_argument("Invalid rounding increment");
        for(uint32_t i=0;i<header().ruleCount;++i) if(header().rules[i].type==r.type) {header().rules[i]=r;changed();return;}
        if(header().ruleCount>=std::size(header().rules)) throw std::length_error("Too many activity mappings");
        header().rules[header().ruleCount]=r; ++header().ruleCount;changed();
    }
    static float amount(const Record &r,int metric) {
        const auto &v=r.values;
        switch(metric) {
        case Duration:return v[Seconds]/60; case Distance:return v[Metres]/1000;
        case ExcessHR:return v[AverageHR]-v[RestHR]; case MeanHR:return v[AverageHR];
        case HRSD:return v[DeviationHR]; case Calories:return v[Kcal];
        case Speed:return v[MetresPerSecond]*3.6f; case Steps:return v[StepCount];
        case Elevation:return v[ElevationMetres]; case Power:return v[Watts]; case Cadence:return v[RPM];
        case DistanceMiles:return v[Metres]/1609.344f;
        case SpeedMiles:return v[MetresPerSecond]*(3600.0f/1609.344f);
        case MaxHR:return r.maximumHR>0?r.maximumHR:missing;
        default:return missing;
        }
    }
    // Samples retain their original times. Cover at most two minutes after a
    // sample; a ten-minute hole is not ten minutes of known heart rate.
    static std::array<float,4> statistics(std::span<const Sample> samples,int64_t start,int64_t end) {
        double weight=0,sum=0,squares=0;float maximum=missing;
        for(size_t i=0;i<samples.size();++i) {
            auto &s=samples[i]; if(!std::isfinite(s.value)||s.value<0) continue;
            if(s.value>0&&s.time>=start&&s.time<end)maximum=std::isfinite(maximum)?std::max(maximum,s.value):s.value;
            int64_t to=std::min(end,s.time+120000);
            if(i+1<samples.size()) to=std::min(to,samples[i+1].time);
            const double w=double(std::max<int64_t>(0,to-std::max(start,s.time)))/1000;
            weight+=w; sum+=w*s.value; squares+=w*s.value*s.value;
        }
        if(weight<=0) return {missing,missing,0,maximum};
        const double mean=sum/weight;
        return {float(mean),float(std::sqrt(std::max(0.0,squares/weight-mean*mean))),float(weight),maximum};
    }
    struct Peak {float value=missing;int64_t time=0;};
    static Peak peak(std::span<const Sample> samples,int64_t start,int64_t end,bool allowZero=false) {
        Peak result;
        for(const auto &sample:samples) {
            if(!std::isfinite(sample.value)||sample.value<0||(!allowZero&&sample.value==0)||sample.time<start||sample.time>=end)continue;
            if(!std::isfinite(result.value)||sample.value>result.value||(sample.value==result.value&&sample.time<result.time))
                result={sample.value,sample.time};
        }
        return result;
    }
    static std::span<const Sample> forSource(std::span<const Sample> samples,uint32_t source) {
        auto first=std::lower_bound(samples.begin(),samples.end(),source,[](auto &s,uint32_t id){return s.source<id;});
        auto last=std::upper_bound(first,samples.end(),source,[](uint32_t id,auto &s){return id<s.source;});
        return {first,last};
    }
    int heartSourceRank(const Record &r,uint32_t id) const {
        const auto device=text(source(r.source).device);
        const auto candidate=source(id);
        // A type-only description such as " [1]" does not identify a watch.
        const bool sameDevice=!device.empty()&&device.front()!=' '&&device.front()!='['&&
            device==text(candidate.device);
        return id==r.source?3:sameDevice?2:text(candidate.origin)==text(r.origin)?1:0;
    }
    std::span<const Sample> heartSamples(const Record &r) const {
        auto remaining=view<Sample>(r.heart);
        std::span<const Sample> best;
        int bestRank=-1;
        float bestCoverage=0;
        while(!remaining.empty()) {
            const uint32_t id=remaining.front().source;
            const auto part=forSource(remaining,id);
            if(part.empty())break;
            const float coverage=statistics(part,int64_t(r.start)*1000,int64_t(r.end)*1000)[2];
            const int rank=heartSourceRank(r,id);
            if(!sourceRemoved(r.group,id)&&coverage>0&&(rank>bestRank||(rank==bestRank&&coverage>bestCoverage))) {
                best=part;bestRank=rank;bestCoverage=coverage;
            }
            remaining=remaining.subspan(part.size());
        }
        return best;
    }
    std::span<const Sample> measureSamples(const Record &r,std::span<const Sample> samples) const {
        auto selected=forSource(samples,r.source);
        if(sourceRemoved(r.group,r.source))selected={};
        for(size_t i=0;selected.empty()&&i<samples.size();) {
            const auto part=forSource(samples,samples[i].source);
            if(!sourceRemoved(r.group,samples[i].source))selected=part;
            i+=part.size();
        }
        return selected;
    }
    Peak speedPeak(const Record &r) const {
        return peak(measureSamples(r,view<Sample>(r.speed)),int64_t(r.start)*1000,int64_t(r.end)*1000,true);
    }
    Peak heartPeak(const Record &r) const {
        return peak(heartSamples(r),int64_t(r.start)*1000,int64_t(r.end)*1000);
    }
    void restingHeartRate(Record &r,std::span<const Sample> heart) const {
        // The baseline must belong to the selected HR source, not a different
        // watch or the session app. A missing matching baseline stays missing.
        const auto selected=heart.empty()?std::span<const Sample>{}:forSource(view<Sample>(r.rest),heart.front().source);
        std::vector<float> rates;
        const int64_t end=int64_t(r.start)*1000,begin=end-7LL*86400*1000;
        for(auto &sample:selected)if(sample.time>=begin&&sample.time<=end&&sample.value>0)rates.push_back(sample.value);
        std::sort(rates.begin(),rates.end());
        r.values[RestHR]=rates.empty()?missing:rates.size()%2?rates[rates.size()/2]:
            (rates[rates.size()/2-1]+rates[rates.size()/2])/2;
    }
    struct HeartSummary { uint32_t source; std::array<float,4> statistics; float resting; int64_t maximumTime; };
    std::vector<HeartSummary> heartSummaries(const Record &r) const {
        std::vector<HeartSummary> summaries;
        auto remaining=view<Sample>(r.heart);
        while(!remaining.empty()) {
            const auto id=remaining.front().source;
            const auto part=forSource(remaining,id);
            if(part.empty())break;
            const auto stats=statistics(part,int64_t(r.start)*1000,int64_t(r.end)*1000);
            if(!sourceRemoved(r.group,id)&&stats[2]>0) {
                Record copy=r;restingHeartRate(copy,part);
                summaries.push_back({id,stats,copy.values[RestHR],peak(part,int64_t(r.start)*1000,int64_t(r.end)*1000).time});
            }
            remaining=remaining.subspan(part.size());
        }
        // Match the activity's selected HR source first, followed by matching
        // devices/apps, then coverage. Do not change or combine the raw samples.
        std::stable_sort(summaries.begin(),summaries.end(),[&](const auto &a,const auto &b) {
            const int aRank=heartSourceRank(r,a.source),bRank=heartSourceRank(r,b.source);
            return aRank!=bRank?aRank>bRank:a.statistics[2]>b.statistics[2];
        });
        return summaries;
    }
    void samples(uint32_t index,int kind,std::span<const Sample> samples) {
        if(kind<0||kind>4)throw std::invalid_argument("Activity sample kind");
        auto *record=get(index); if(!record) throw std::out_of_range("Activity index");
        Record r=*record;
        Slice *slice=kind==0?&r.heart:kind==1?&r.speed:kind==2?&r.power:kind==3?&r.cadence:&r.rest;
        auto old=view<Sample>(*slice);
        if(old.size()!=samples.size()||(!old.empty()&&memcmp(old.data(),samples.data(),old.size_bytes())))
            *slice=append<Sample>(samples);
        // Samples are ordered by (source, time). Never average different sources.
        if(kind==0||kind==4) {
            const auto selected=heartSamples(r);
            if(kind==0) {
                const auto stats=statistics(selected,int64_t(r.start)*1000,int64_t(r.end)*1000);
                r.values[AverageHR]=stats[0];r.values[DeviationHR]=stats[1];r.values[HRCoverage]=stats[2];r.maximumHR=stats[3];
            }
            restingHeartRate(r,selected);
            *get(index)=r;changed();return;
        }
        const auto selected=measureSamples(r,samples);
        auto stats=statistics(selected,int64_t(r.start)*1000,int64_t(r.end)*1000);
        r.values[kind==1?MetresPerSecond:kind==2?Watts:RPM]=stats[0];
        *get(index)=r;changed();
    }
};
}
