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
                       DistanceMiles, SpeedMiles, MetricCount };
enum Field : size_t { Seconds, Metres, RestHR, AverageHR, DeviationHR, HRCoverage,
                     Kcal, MetresPerSecond, StepCount, ElevationMetres, Watts, RPM, FieldCount };
enum : uint32_t { Deleted=1, Linked=2, Generated=4, LabelOverride=8, ValueOverride=16,
                  NeedsDetails=32, Ambiguous=64, TimeOverride=128, NotReturned=256 };
struct Slice { uint32_t offset=0, count=0; };
struct alignas(8) Sample { int64_t time=0; float value=0; uint32_t source=0; };
struct Source { Slice origin, device; };
struct Rule { int32_t type=0, label=-1, metric=Duration; uint32_t reserved=0; };
struct alignas(8) Record {
    uint32_t group=0, flags=0, type=0, start=0, end=0, revision=0;
    int64_t modified=0;
    Slice id, origin, title, notes, heart, speed, power, cadence, rest;
    std::array<float,FieldCount> values;
    int32_t generatedLabel=-1, generatedMetric=Duration;
    float generatedValue=missing;
    uint32_t generatedTime=0;
    uint32_t source=0, reserved[3]{};
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
};
static_assert(std::endian::native==std::endian::little);
static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559);
static_assert(sizeof(Slice)==8 && sizeof(Sample)==16 && sizeof(Rule)==16);
static_assert(sizeof(Record)==184 && offsetof(Record,values)==104);
static_assert(sizeof(Header)<recordStart);
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
    }
    Header &header() { return *reinterpret_cast<Header*>(records.data()); }
    Record *get(uint32_t index) {
        if(index==0||index>header().count) return nullptr;
        return reinterpret_cast<Record*>(records.data()+recordStart)+(index-1);
    }
    bool valid(const Record &r) {
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
        return {static_cast<uint32_t>(offset),static_cast<uint32_t>(input.size())};
    }
    Slice append(std::string_view s) { return append<char>({s.data(),s.size()}); }
    void token(size_t type,std::string_view token) {
        if(type>=std::size(header().tokens)) throw std::out_of_range("Activity token type");
        if(text(header().tokens[type])!=token) header().tokens[type]=append(token);
        header().pendingTokens[type]={};
    }
    std::string_view token(size_t type) {
        if(type>=std::size(header().tokens))throw std::out_of_range("Activity token type");
        auto pending=text(header().pendingTokens[type]);
        return pending.empty()?text(header().tokens[type]):pending;
    }
    void checkpoint(size_t type,std::string_view token,bool dirty) {
        if(type>=std::size(header().tokens))throw std::out_of_range("Activity token type");
        if(dirty)for(uint32_t i=1;i<=header().count;++i)if(!(get(i)->flags&(Deleted|NotReturned)))get(i)->flags|=NeedsDetails;
        // Record the remaining work before remembering how far the change list
        // was read. The completed cursor is published after details and linking.
        std::atomic_signal_fence(std::memory_order_release);
        if(text(header().pendingTokens[type])!=token)header().pendingTokens[type]=append(token);
    }
    void resetTokens() { for(auto &t:header().tokens)t={};for(auto &t:header().pendingTokens)t={}; }
    void beginSessionSnapshot() {
        // A full query may omit old records after history access is revoked.
        // Retain their saved details instead of overwriting them with empty reads.
        for(uint32_t i=1;i<=header().count;++i)get(i)->flags|=NotReturned;
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
    static bool sameSession(const Record &a,const Record &b) {
        if(a.type!=b.type||a.end<=a.start||b.end<=b.start) return false;
        const int64_t overlap=int64_t(std::min(a.end,b.end))-std::max(a.start,b.start);
        return std::abs(int64_t(a.start)-b.start)<=120&&std::abs(int64_t(a.end)-b.end)<=120&&
            overlap>0&&overlap*10>=int64_t(std::max(a.end-a.start,b.end-b.start))*9;
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
            incoming.heart=old.heart; incoming.speed=old.speed; incoming.power=old.power; incoming.cadence=old.cadence; incoming.rest=old.rest;
            incoming.values=old.values;
            incoming.revision=old.revision;
            incoming.id=old.id; incoming.origin=old.origin;
            incoming.title=text(old.title)==title?old.title:append(title);
            incoming.notes=text(old.notes)==notes?old.notes:append(notes);
        } else {
            uint32_t group=0;
            for(uint32_t i=1;i<=header().count;++i) {
                auto &other=*get(i);
                if(!(other.flags&Deleted)&&valid(other)&&text(other.origin)!=origin&&sameSession(incoming,other)) {
                    bool represented=false;
                    for(uint32_t j=1;j<=header().count;++j) {
                        auto &member=*get(j);
                        if(member.group==other.group&&!(member.flags&Deleted)&&text(member.origin)==origin) {represented=true;break;}
                    }
                    if(represented)continue;
                    if(group&&group!=other.group) { group=0; break; }
                    group=other.group;
                }
            }
            index=header().count+1; incoming.group=group?group:index;
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
        return index;
    }
    Rule rule(int type) {
        for(uint32_t i=0;i<header().ruleCount;++i) if(header().rules[i].type==type) return header().rules[i];
        return {type,header().defaultLabel,Duration,0};
    }
    void setRule(Rule r) {
        if(r.metric<0||r.metric>=MetricCount) throw std::invalid_argument("Invalid activity measure");
        for(uint32_t i=0;i<header().ruleCount;++i) if(header().rules[i].type==r.type) {header().rules[i]=r;return;}
        if(header().ruleCount>=std::size(header().rules)) throw std::length_error("Too many activity mappings");
        header().rules[header().ruleCount]=r; ++header().ruleCount;
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
        default:return missing;
        }
    }
    // Samples retain their original times. Cover at most two minutes after a
    // sample; a ten-minute hole is not ten minutes of known heart rate.
    static std::array<float,3> statistics(std::span<const Sample> samples,int64_t start,int64_t end) {
        double weight=0,sum=0,squares=0;
        for(size_t i=0;i<samples.size();++i) {
            auto &s=samples[i]; if(!std::isfinite(s.value)||s.value<0) continue;
            int64_t to=std::min(end,s.time+120000);
            if(i+1<samples.size()) to=std::min(to,samples[i+1].time);
            const double w=double(std::max<int64_t>(0,to-std::max(start,s.time)))/1000;
            weight+=w; sum+=w*s.value; squares+=w*s.value*s.value;
        }
        if(weight<=0) return {missing,missing,0};
        const double mean=sum/weight;
        return {float(mean),float(std::sqrt(std::max(0.0,squares/weight-mean*mean))),float(weight)};
    }
    static std::span<const Sample> forSource(std::span<const Sample> samples,uint32_t source) {
        auto first=std::lower_bound(samples.begin(),samples.end(),source,[](auto &s,uint32_t id){return s.source<id;});
        auto last=std::upper_bound(first,samples.end(),source,[](uint32_t id,auto &s){return id<s.source;});
        return {first,last};
    }
    void samples(uint32_t index,int kind,std::span<const Sample> samples) {
        if(kind<0||kind>4)throw std::invalid_argument("Activity sample kind");
        auto *record=get(index); if(!record) throw std::out_of_range("Activity index");
        Record r=*record;
        Slice *slice=kind==0?&r.heart:kind==1?&r.speed:kind==2?&r.power:kind==3?&r.cadence:&r.rest;
        auto old=view<Sample>(*slice);
        if(old.size()!=samples.size()||(!old.empty()&&memcmp(old.data(),samples.data(),old.size_bytes())))
            *slice=append<Sample>(samples);
        // Samples are ordered by (source, time). Preserve every source but use
        // one source for the Amount; readings from two watches are not averaged.
        auto selected=forSource(samples,r.source);
        if(selected.empty()&&!samples.empty())selected=forSource(samples,samples.front().source);
        if(kind==4) {
            std::vector<float> rates;
            for(auto &sample:selected)if(sample.time<=int64_t(r.start)*1000&&sample.value>0)rates.push_back(sample.value);
            std::sort(rates.begin(),rates.end());
            r.values[RestHR]=rates.empty()?missing:rates.size()%2?rates[rates.size()/2]:
                (rates[rates.size()/2-1]+rates[rates.size()/2])/2;
            *get(index)=r;return;
        }
        auto stats=statistics(selected,int64_t(r.start)*1000,int64_t(r.end)*1000);
        if(kind==0) {r.values[AverageHR]=stats[0];r.values[DeviationHR]=stats[1];r.values[HRCoverage]=stats[2];}
        else r.values[kind==1?MetresPerSecond:kind==2?Watts:RPM]=stats[0];
        *get(index)=r;
    }
};
}
