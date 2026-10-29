// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <exception>

namespace juggluco::activity::mirror {
inline constexpr size_t chunkBytes=256*1024;
inline constexpr size_t maxChunkTransfer=256*1024*1024-1024;
inline constexpr unsigned char capability[3]={'A','C',1};
// Belongs to one parser, not to a host slot or a process-global temporary file.
class Chunks {
    std::vector<uint8_t> bytes;
    uint32_t total=0;
public:
    void reset(){std::vector<uint8_t>().swap(bytes);total=0;}
    template<class Commit>
    bool append(uint32_t size,uint32_t offset,std::span<const uint8_t> part,Commit commit) {
        if(!size||size>maxChunkTransfer||part.empty()||part.size()>chunkBytes||
           offset>size||part.size()>size-offset){reset();return false;}
        if(!offset){reset();total=size;}
        if(size!=total||offset!=bytes.size()){reset();return false;}
        try {
            bytes.insert(bytes.end(),part.begin(),part.end());
            if(bytes.size()!=total)return true;
            const bool result=commit(bytes.data(),bytes.size());reset();return result;
        } catch(const std::exception &) {reset();return false;}
    }
};
}
