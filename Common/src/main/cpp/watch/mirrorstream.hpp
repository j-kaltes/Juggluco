// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <cerrno>
#include <sys/socket.h>

namespace mirrorstream {
// Separate from legacy /data. Both peers must support /mirrorstream.
enum Kind:uint32_t {open=1,ready=2,data=3};
struct Header {
    uint32_t magic=0x32534d4a,kind=data;
    uint64_t sender=0,receiver=0,sequence=0;
};
static_assert(sizeof(Header)==32);
struct State {
    const bool initiator;
    const bool ble;
    uint64_t sender=0,receiver=0,next=0,out=0;
    bool established=false;
    State(bool initiator,bool ble,uint64_t nonce):initiator(initiator),ble(ble) {
        (initiator?sender:receiver)=nonce;
    }
    bool accept(const Header &h,bool fromBle,size_t bytes) {
        if(h.magic!=Header{}.magic||fromBle!=ble||!h.sender)return false;
        if(h.kind==open) {
            if(initiator||bytes||h.receiver||h.sequence||sender)return false;
            sender=h.sender;return true;
        }
        if(h.kind==ready) {
            if(!initiator||bytes||h.sequence||h.sender!=sender||!h.receiver||established)return false;
            receiver=h.receiver;established=true;return true;
        }
        if(h.kind!=data||!bytes||!established||h.sender!=sender||h.receiver!=receiver||
           h.sequence!=next||next==UINT64_MAX)return false;
        ++next;return true;
    }
    Header frame(Kind kind) {
        Header h;h.kind=kind;h.sender=sender;h.receiver=kind==open?0:receiver;
        if(kind==data)h.sequence=out++;
        return h;
    }
};
inline bool writeAll(int fd,const uint8_t *data,size_t length) {
    while(length) {
        const auto done=send(fd,data,length,MSG_NOSIGNAL);
        if(done<0&&errno==EINTR)continue;
        if(done<=0)return false;
        data+=done;length-=done;
    }
    return true;
}
}
