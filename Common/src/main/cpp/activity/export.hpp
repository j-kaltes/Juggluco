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
#include "types.hpp"
#include "speed_peak.hpp"
#include <charconv>
#include <climits>
#include <cstdio>
#include <ctime>
#include <cwchar>
#include <locale.h>

namespace juggluco::activity::tsv {
enum Column { Group, RecordID, Start, End, Activity, Type, ActivitySource, ActivityDevice,
    Elapsed, DistanceValue, Energy, SpeedValue, MaximumSpeed, StepsValue, ElevationValue, PowerValue,
    CadenceValue, HeartSource, HeartDevice, SummaryHeart, RestingHeart, MeanHeart,
    MaximumHeart, HeartSD, HeartCoverage, ExcessHeart, Status, HCID, Modified, Title, Notes, Count };
using Row=std::array<std::string,Count>;
inline constexpr Column standardColumns[]={RecordID,Start,Elapsed,Activity,DistanceValue,
    SpeedValue,MaximumSpeed,StepsValue,RestingHeart,MeanHeart,MaximumHeart,HeartSD,ExcessHeart,Energy};
inline constexpr auto extendedColumns=[] {
    std::array<Column,Count> columns{};
    for(size_t i=0;i<Count;++i)columns[i]=static_cast<Column>(i);
    return columns;
}();
inline std::span<const Column> columns(bool extended) {
    if(extended)return extendedColumns;
    return standardColumns;
}

// Use terminal character widths without changing the process-wide locale.
// Android/Linux provide C.UTF-8. If unavailable, non-ASCII bytes are escaped.
class TextLocale {
    locale_t locale=newlocale(LC_CTYPE_MASK,"C.UTF-8",nullptr),previous{};
public:
    TextLocale() {
        if(!locale)locale=newlocale(LC_CTYPE_MASK,"C",nullptr);
        if(!locale)throw std::runtime_error("Activity export locale unavailable");
        previous=uselocale(locale);
        if(!previous) {freelocale(locale);throw std::runtime_error("Activity export locale failed");}
    }
    ~TextLocale() {uselocale(previous);freelocale(locale);}
};

// Exactly one physical line per row and one tab per column boundary. Keep
// readable Unicode; escape control characters, backslashes and invalid bytes.
inline std::string textCell(std::string_view input) {
    constexpr char hex[]="0123456789ABCDEF";
    std::string out;
    for(size_t pos=0;pos<input.size();) {
        const unsigned char byte=input[pos];
        if(byte=='\\'||byte=='\t'||byte=='\n'||byte=='\r') {
            out+='\\';out+=byte=='\\'?'\\':byte=='\t'?'t':byte=='\n'?'n':'r';++pos;continue;
        }
        std::mbstate_t state{};wchar_t ch{};
        const size_t size=std::mbrtowc(&ch,input.data()+pos,input.size()-pos,&state);
        if(size>0&&size<=input.size()-pos&&wcwidth(ch)>=0) {
            out.append(input.substr(pos,size));pos+=size;
        } else {
            out+="\\x";out+=hex[byte>>4];out+=hex[byte&15];++pos;
        }
    }
    return out;
}
inline size_t width(std::string_view text) {
    size_t columns=0;
    while(!text.empty()) {
        std::mbstate_t state{};wchar_t ch{};
        const auto bytes=std::mbrtowc(&ch,text.data(),text.size(),&state);
        if(bytes==0||bytes>text.size()) {++columns;text.remove_prefix(1);}
        else {columns+=std::max(0,wcwidth(ch));text.remove_prefix(bytes);}
    }
    return columns;
}
inline std::string number(double value,int decimals=1) {
    if(!std::isfinite(value))return {};
    char buffer[128];
    const auto [end,error]=std::to_chars(buffer,buffer+sizeof(buffer),value,std::chars_format::fixed,decimals);
    if(error!=std::errc{})throw std::runtime_error("Activity number formatting failed");
    return {buffer,end};
}
inline std::string duration(double seconds) {
    if(!std::isfinite(seconds)||seconds<0||seconds>UINT32_MAX)return {};
    const auto total=static_cast<uint64_t>(std::round(seconds));
    char buffer[48];
    if(total>=86400)snprintf(buffer,sizeof(buffer),"%llu:%02u:%02u:%02u",
        static_cast<unsigned long long>(total/86400),unsigned(total/3600%24),unsigned(total/60%60),unsigned(total%60));
    else snprintf(buffer,sizeof(buffer),"%u:%02u:%02u",unsigned(total/3600),unsigned(total/60%60),unsigned(total%60));
    return buffer;
}
inline std::string timestamp(uint32_t seconds) {
    const time_t time=seconds;tm local{};char buffer[48];
    if(!localtime_r(&time,&local)||!strftime(buffer,sizeof(buffer),"%Y-%m-%dT%H:%M:%S%z",&local))
        throw std::runtime_error("Activity date formatting failed");
    return buffer;
}
inline Row headings(bool miles) {
    return {"Group","Record","Start","End","Activity","Type","Activity source","Activity device",
        "Duration (d:h:m:s)",miles?"Distance (mi)":"Distance (km)","Active energy (kcal)",
        miles?"Mean speed (mi/h)":"Mean speed (km/h)",miles?"Max speed (mi/h)":"Max speed (km/h)",
        "Steps","Elevation (m)","Mean power (W)",
        "Mean cadence (rpm)","HR source","HR device","Summary HR","Resting HR (bpm)",
        "Mean HR (bpm)","Max HR (bpm)","HR SD (bpm)","HR coverage (d:h:m:s)","Mean-resting HR (bpm)",
        "Status","Health Connect ID","Modified (Unix ms)","Title","Notes"};
}
inline Row activityRow(Store &store,uint32_t index,bool miles,bool extended) {
    const auto &record=*store.get(index);const auto &v=record.values;
    Row row;
    row[RecordID]=std::to_string(index);row[Start]=timestamp(record.start);
    row[Activity]=typeName(record.type);
    row[Elapsed]=duration(v[Seconds]);
    row[DistanceValue]=number(double(v[Metres])/(miles?1609.344:1000.0),3);
    row[Energy]=number(v[Kcal]);row[SpeedValue]=number(double(v[MetresPerSecond])*(miles?3600.0/1609.344:3.6));
    const auto speed=filteredSpeedPeak(store.measureSamples(record,store.view<Sample>(record.speed)),
        record.type,int64_t(record.start)*1000,int64_t(record.end)*1000);
    row[MaximumSpeed]=number(double(speed.value)*(miles?3600.0/1609.344:3.6));
    row[StepsValue]=number(v[StepCount],0);
    if(!extended)return row;
    row[Group]=std::to_string(store.groupOf(index));row[End]=timestamp(record.end);
    row[Type]=std::to_string(record.type);
    row[ActivitySource]=textCell(store.text(record.origin));
    row[ActivityDevice]=textCell(store.text(store.source(record.source).device));
    row[ElevationValue]=number(v[ElevationMetres]);
    row[PowerValue]=number(v[Watts]);row[CadenceValue]=number(v[RPM]);
    if(record.flags&Deleted)row[Status]="deleted";
    else if(record.flags&NotReturned)row[Status]="not returned";
    else row[Status]="current";
    if(record.flags&NeedsDetails)row[Status]+="; details pending";
    row[HCID]=textCell(store.text(record.id));
    if(record.modified)row[Modified]=std::to_string(record.modified);
    row[Title]=textCell(store.text(record.title));row[Notes]=textCell(store.text(record.notes));
    return row;
}
inline void heartRow(Row &row,const Record &record) {
    const auto &v=record.values;
    row[RestingHeart]=number(v[RestHR]);row[MeanHeart]=number(v[AverageHR]);
    row[MaximumHeart]=record.maximumHR>0?number(record.maximumHR):std::string{};
    row[HeartSD]=number(v[DeviationHR]);
    row[HeartCoverage]=std::isfinite(v[AverageHR])?duration(v[HRCoverage]):std::string{};
    row[ExcessHeart]=number(v[AverageHR]-v[RestHR]);
}

// Standard: one row per session using its saved HR summary. Extended: one row
// per HR app/device, or one without HR. Count limits always apply to sessions.
template<class Consume> bool rows(Store &store,std::span<const uint32_t> order,bool miles,bool extended,Consume consume) {
    for(auto index:order) {
        const auto &record=*store.get(index);
        auto row=activityRow(store,index,miles,extended);
        if(!extended) {
            heartRow(row,record);
            if(!consume(row))return false;
            continue;
        }
        const auto selected=store.heartSamples(record);
        const auto summaries=store.heartSummaries(record);
        if(summaries.empty()) {
            heartRow(row,record); // Preserve legacy cached statistics, if present.
            if(!consume(row))return false;
        }
        for(const auto &heart:summaries) {
            const auto sourceID=heart.source;
            const auto part=Store::forSource(store.view<Sample>(record.heart),sourceID);
            const auto source=store.source(sourceID);
            row[HeartSource]=textCell(store.text(source.origin));
            row[HeartDevice]=textCell(store.text(source.device));
            row[SummaryHeart]=!selected.empty()&&selected.front().source==sourceID?"1":"0";
            const auto stats=heart.statistics;
            Record copy=record; // Computing another source's baseline never changes the mmap.
            copy.values[AverageHR]=stats[0];copy.values[DeviationHR]=stats[1];copy.values[HRCoverage]=stats[2];
            copy.maximumHR=stats[3];
            store.restingHeartRate(copy,part);heartRow(row,copy);
            if(!consume(row))return false;
        }
    }
    return true;
}

// Only indices and one formatted row are copied. Two passes determine widths
// then stream the rows, while holding the same mutex used by the importer.
template<class Write> bool write(Store &store,uint32_t start,uint32_t end,bool header,int maxcount,Write output,bool extended=false) {
    std::lock_guard guard(store.mutex);
    TextLocale locale;
    const bool miles=store.header().options&UseMiles;
    const auto titles=headings(miles);
    const auto visible=columns(extended);
    std::vector<uint32_t> order;
    if(start<end&&maxcount>0)for(uint32_t i=1;i<=store.header().count;++i) {
        const auto &record=*store.get(i);
        if(record.start<end&&record.end>start&&store.sourceVisible(i)&&store.groupOf(i))order.push_back(i);
    }
    std::sort(order.begin(),order.end(),[&](uint32_t a,uint32_t b) {
        return store.get(a)->start!=store.get(b)->start?store.get(a)->start<store.get(b)->start:a<b;
    });
    if(maxcount>=0&&order.size()>size_t(maxcount))order.resize(maxcount);
    std::array<size_t,Count> widths{};
    auto measure=[&](const Row &row) {
        for(auto i:visible)widths[i]=std::max(widths[i],width(row[i]));
        return true;
    };
    measure(titles); // Headerless data has the same alignment as headed data.
    rows(store,order,miles,extended,measure);
    std::string line;
    auto emit=[&](const Row &row) {
        line.clear();
        for(size_t position=0;position<visible.size();++position) {
            const auto i=visible[position];
            line+=row[i];
            if(position+1<visible.size()) {line.append(widths[i]-width(row[i]),' ');line+='\t';}
        }
        line+='\n';return output(std::string_view(line));
    };
    if(header&&!emit(titles))return false;
    return rows(store,order,miles,extended,emit);
}
}
