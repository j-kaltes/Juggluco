// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <algorithm>
#include <cstring>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "num.h"

namespace numrepair {
inline constexpr uint32_t pending=0x52504d4e;
inline constexpr size_t markerOffset=20;
struct Fd {
    int fd;
    explicit Fd(int fd):fd(fd){}
    ~Fd(){if(fd>=0)close(fd);}
    Fd(const Fd&)=delete;
};
inline void require(bool ok,[[maybe_unused]] const char *operation,[[maybe_unused]] const std::string &path) {
    if(ok)return;
#ifdef LOGGER
    const int error=errno;
    LOGGER("Numerical mirror repair %s failed for %s: %s (%d)\n",operation,path.c_str(),strerror(error),error);
#endif
    throw std::runtime_error("Numerical mirror repair failed; original files retained");
}
inline uint32_t get(const std::array<uint8_t,256>&h,size_t offset) {
    uint32_t value;memcpy(&value,h.data()+offset,4);return value;
}
inline void put(std::array<uint8_t,256>&h,size_t offset,uint32_t value) {memcpy(h.data()+offset,&value,4);}
inline void trim(const std::string &path,size_t length) {
    Fd fd(open(path.c_str(),O_RDWR|O_CREAT,0600));
    require(fd.fd>=0,"open for truncation",path);
    require(ftruncate(fd.fd,length)==0&&fsync(fd.fd)==0,"truncate mirror auxiliary file",path);
}
// Run before any mappings. Correct only the demonstrated received-mirror
// corruption, or finish its interrupted truncation. Keep all existing records.
inline void prepare(const std::string &base,const char *file,size_t minimum) {
    Fd original(open(file,O_RDWR));
    if(original.fd<0){require(errno==ENOENT,"open for truncation",file);return;}
    std::array<uint8_t,256> header{};struct stat st{};
    if(fstat(original.fd,&st)||pread(original.fd,header.data(),header.size(),0)!=ssize_t(header.size()))return;
    constexpr uint8_t magic[]={0xfe,0xd2,0xde,0xad};
    const uint32_t last=get(header,128),first=get(header,144);
    if(minimum<16||memcmp(header.data(),magic,4)||header[16]!=1||st.st_size<256||first>=minimum-16)return;
    const bool interrupted=get(header,markerOffset)==pending;
    if(!interrupted&&(last!=get(header,120)||last<946684800U||
       uint64_t(last)<=(uint64_t(st.st_size)-256)/sizeof(Num)))return;

    uint32_t end=first;
    std::array<Num,4096> block;
    bool foundEnd=false;
    for(uint64_t offset=256+uint64_t(first)*sizeof(Num);offset<uint64_t(st.st_size)&&!foundEnd;) {
        const auto got=pread(original.fd,block.data(),block.size()*sizeof(Num),offset);
        require(got>=0,"read existing records",file);
        const size_t records=size_t(got)/sizeof(Num);
        if(!records)break;
        for(size_t i=0;i<records;++i) {
            if(!block[i].time){foundEnd=true;break;}
            ++end;
        }
        offset+=records*sizeof(Num);
    }
    const size_t count=std::max(minimum,size_t(end)+16+4096);
    if(count>size_t(PTRDIFF_MAX)/sizeof(Num))errno=EOVERFLOW;
    require(count<=size_t(PTRDIFF_MAX)/sizeof(Num),"size replacement",file);
    // Mark the operation before truncating. Startup can repeat it if interrupted.
    require(pwrite(original.fd,&pending,sizeof(pending),markerOffset)==sizeof(pending)&&
        fsync(original.fd)==0,"mark mirror truncation",file);
    trim(base+"/libreids.dat",count*8);
    trim(base+"/librechanged.dat",count*4);
    trim(base+"/changetimes.dat",count*4);
    put(header,120,end);put(header,128,end);put(header,markerOffset,pending);
    // The two corrected positions must be durable before shrinking nums.
    require(pwrite(original.fd,header.data(),header.size(),0)==ssize_t(header.size())&&
        fsync(original.fd)==0,"write corrected header",file);
    require(ftruncate(original.fd,count*sizeof(Num))==0&&fsync(original.fd)==0,"truncate mirror records",file);
    // Records are already present: completion needs no forced full resend.
    const uint32_t cleared=0;
    require(pwrite(original.fd,&cleared,sizeof(cleared),markerOffset)==sizeof(cleared)&&
        fsync(original.fd)==0,"finish mirror truncation",file);
}
}
