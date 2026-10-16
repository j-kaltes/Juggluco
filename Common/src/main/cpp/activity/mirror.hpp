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
#include "amount_binding.hpp"
#include "cursor.hpp"

namespace juggluco::activity::mirror {
// Uses the existing acknowledged file-once protocol, like meals. Older peers
// can retain this file without understanding activity references.
inline constexpr char filename[]="activity-mirror.dat";
inline constexpr uint64_t magic=0x31524f5252494d41ULL;
inline constexpr size_t maxBytes=256*1024*1024-1024;
struct Envelope {uint64_t magic=mirror::magic;uint32_t version=1,records=0,data=0,start=8;};
static_assert(sizeof(Envelope)==24);
struct Transfer {std::vector<std::byte> bytes;Cursor cursor;};
inline bool unchanged(const Store &store,const Cursor &cursor) {
    return cursor.epoch==store.header().mirrorEpoch&&cursor.revision==store.header().mirrorRevision;
}
inline Transfer capture(Store &store,const Cursor &previous) {
    const auto records=store.recordBytes(),data=store.dataBytes();
    if(records.size()+data.size()+sizeof(Envelope)>maxBytes)throw std::length_error("Activity mirror data is too large");
    const uint32_t start=previous.epoch==store.header().mirrorEpoch&&previous.dataSize>=8&&previous.dataSize<=data.size()?previous.dataSize:8;
    Envelope envelope;envelope.records=records.size();envelope.data=data.size();envelope.start=start;
    Transfer result;result.cursor={store.header().mirrorEpoch,store.header().mirrorRevision,uint32_t(data.size())};
    result.bytes.resize(sizeof(envelope)+records.size()+8+data.size()-start);
    auto *out=result.bytes.data();memcpy(out,&envelope,sizeof(envelope));out+=sizeof(envelope);
    memcpy(out,records.data(),records.size());out+=records.size();
    memcpy(out,data.data(),8);out+=8;memcpy(out,data.data()+start,data.size()-start);
    return result;
}
inline void validate(std::span<const std::byte> records,std::span<const std::byte> data) {
    if(records.size()<recordStart||data.size()<8)throw std::runtime_error("Incomplete activity mirror image");
    Header h{};memcpy(&h,records.data(),sizeof(h));
    uint32_t dataMagic=0,used=0;memcpy(&dataMagic,data.data(),4);memcpy(&used,data.data()+4,4);
    if(h.magic!=0x3159544956495441ULL||h.version!=1||h.ruleCount>std::size(h.rules)||!h.mirrorEpoch||
        recordStart+uint64_t(h.count)*sizeof(Record)!=records.size()||dataMagic!=0x31445641||used!=data.size())
        throw std::runtime_error("Invalid activity mirror image");
    auto slice=[&](Slice value,size_t size,size_t alignment) {
        if(!value.count)return;
        if(value.offset<8||value.offset%alignment||uint64_t(value.offset)+uint64_t(value.count)*size>data.size())
            throw std::runtime_error("Invalid activity mirror slice");
    };
    slice(h.sources,sizeof(Source),alignof(Source));slice(h.removedSources,sizeof(RemovedSource),alignof(RemovedSource));
    for(uint32_t i=0;i<h.ruleCount;++i)if(h.rules[i].metric<0||h.rules[i].metric>=MetricCount||
        !std::isfinite(h.rules[i].roundTo)||h.rules[i].roundTo<0)throw std::runtime_error("Invalid mirrored activity mapping");
    for(auto token:h.tokens)slice(token,1,1);for(auto token:h.pendingTokens)slice(token,1,1);
    for(uint32_t i=0;i<h.sources.count;++i) {
        Source source{};memcpy(&source,data.data()+h.sources.offset+i*sizeof(Source),sizeof(source));
        slice(source.origin,1,1);slice(source.device,1,1);
    }
    for(uint32_t i=0;i<h.removedSources.count;++i) {
        RemovedSource removed{};memcpy(&removed,data.data()+h.removedSources.offset+i*sizeof(removed),sizeof(removed));
        if(!removed.group||removed.group>h.count||!removed.source||removed.source>h.sources.count)
            throw std::runtime_error("Invalid removed activity source");
    }
    for(uint32_t i=1;i<=h.count;++i) {
        Record r{};memcpy(&r,records.data()+recordStart+(i-1)*sizeof(Record),sizeof(r));
        if(!r.group||r.group>i||r.end<=r.start||!r.start||!r.id.count||!r.origin.count||r.source>h.sources.count)
            throw std::runtime_error("Invalid mirrored activity record");
        for(auto text:{r.id,r.origin,r.title,r.notes})slice(text,1,1);
        for(auto samples:{r.heart,r.speed,r.power,r.cadence,r.rest})slice(samples,sizeof(Sample),alignof(Sample));
        if(r.amountBinding.count>1)throw std::runtime_error("Invalid mirrored Amount binding");
        slice(r.amountBinding,sizeof(AmountBinding),alignof(AmountBinding));
        if(r.amountBinding.count) {
            AmountBinding binding{};memcpy(&binding,data.data()+r.amountBinding.offset,sizeof(binding));
            if(!std::isfinite(binding.roundTo)||binding.roundTo<0)throw std::runtime_error("Invalid mirrored Amount rounding increment");
        }
    }
}
inline void receive(Store &store,std::span<const std::byte> bytes) {
    Envelope envelope{};
    if(bytes.size()<sizeof(envelope)||bytes.size()>maxBytes)throw std::runtime_error("Incomplete activity mirror transfer");
    memcpy(&envelope,bytes.data(),sizeof(envelope));
    if(envelope.magic!=magic||envelope.version!=1||envelope.start<8||envelope.start>envelope.data||envelope.records<recordStart||
        uint64_t(envelope.records)+envelope.data+sizeof(envelope)>maxBytes||
        sizeof(envelope)+uint64_t(envelope.records)+8+envelope.data-envelope.start!=bytes.size())
        throw std::runtime_error("Invalid activity mirror transfer");
    const auto records=bytes.subspan(sizeof(envelope),envelope.records);
    Header incoming{};memcpy(&incoming,records.data(),sizeof(incoming));
    // Replayed/echoed state is already installed. In particular, do not reset
    // the producer's Health Connect cursors when a clone sends its image back.
    if(store.header().mirrorEpoch==incoming.mirrorEpoch&&store.header().mirrorRevision==incoming.mirrorRevision)return;
    std::vector<std::byte> data(envelope.data);
    if(envelope.start>8) {
        if(store.header().mirrorEpoch!=incoming.mirrorEpoch||!store.header().receivedMirror||
            store.header().receivedDataSize!=envelope.start||store.dataBytes().size()<envelope.start)
            throw std::runtime_error("Activity mirror needs a full transfer");
        memcpy(data.data(),store.dataBytes().data(),envelope.start);
    }
    const auto *payload=bytes.data()+sizeof(envelope)+envelope.records;
    memcpy(data.data(),payload,8);memcpy(data.data()+envelope.start,payload+8,envelope.data-envelope.start);
    validate(records,data);store.receiveImage(records,data);
}
}
