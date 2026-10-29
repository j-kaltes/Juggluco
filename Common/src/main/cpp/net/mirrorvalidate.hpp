// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "comtypes.hpp"
#include <algorithm>
#include <climits>
#include <cstring>
#include <cstddef>

namespace mirrorvalidate {
// Match the existing encrypted-command limit, also for plaintext streams.
inline constexpr uint32_t maxCommand=256U*1024*1024;
inline bool length(uint32_t size,size_t header,int offset) {
    return size>=header&&size<=maxCommand&&size<=uint32_t(INT_MAX-offset-3);
}
// Validate every span before applying any of them. No overflow-prone pointer
// arithmetic and no dependence on the (unused, uninitialized) first field.
inline bool numbers(const numsend &packet,size_t available,size_t capacity,
                    uint32_t first,uint32_t previous) {
    if(available<sizeof(numsend)||packet.dbase>1||!packet.nr||packet.totlen!=available||
       packet.last>capacity||packet.last<first)return false;
    size_t offset=sizeof(numsend);
    uint32_t covered=std::max(first,previous);
    uint32_t starts[255],ends[255];
    const auto *bytes=reinterpret_cast<const uint8_t*>(&packet);
    for(unsigned i=0;i<packet.nr;++i) {
        if(available-offset<8)return false;
        uint32_t start,count;memcpy(&start,bytes+offset,4);memcpy(&count,bytes+offset+4,4);
        offset+=8;
        if(!count||start<first||start>packet.last||count>packet.last-start||
           count>(available-offset)/16)return false;
        starts[i]=start;ends[i]=start+count;offset+=size_t(count)*16;
    }
    if(offset!=available)return false;
    // Extension must contain all new records, even if spans arrive in reverse order.
    for(unsigned pass=0;pass<packet.nr&&covered<packet.last;++pass)
        for(unsigned i=0;i<packet.nr;++i)
            if(starts[i]<=covered)covered=std::max(covered,ends[i]);
    return covered>=packet.last;
}
inline bool filename(const char *name,size_t length) {
    if(length<2||name[length-1]||memchr(name,0,length-1)||name[0]=='/')return false;
    for(size_t begin=0;begin<length-1;) {
        size_t end=begin;while(end<length-1&&name[end]!='/')++end;
        if(end==begin||(end-begin==2&&name[begin]=='.'&&name[begin+1]=='.'))return false;
        begin=end+1;
    }
    return true;
}
inline bool file(const fileonce_t &packet,size_t available) {
    if(available<sizeof(fileonce_t)||packet.totlen!=available||!packet.nr)return false;
    size_t offset=sizeof(fileonce_t);
    if(packet.nr>(available-offset)/sizeof(datel))return false;
    offset+=size_t(packet.nr)*sizeof(datel);
    if(packet.namelen>available-offset||
       !filename(reinterpret_cast<const char*>(&packet)+offset,packet.namelen))return false;
    offset+=packet.namelen;
    for(unsigned i=0;i<packet.nr;++i) {
        const auto &span=packet.gegs[i];
        if(span.off<0||span.len<0||span.len>INT_MAX-span.off||size_t(span.len)>available-offset)return false;
        offset+=span.len;
    }
    const size_t aligned=(offset+3)&~size_t(3);
    if(aligned>available)return false;
    // Only the documented trailing calibration/stream update arguments remain.
    const size_t extra=available-aligned;
    return extra==0||(packet.dowith&&(extra==4||extra==5));
}
}
