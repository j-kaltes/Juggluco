#pragma once
#include "v3protocol.hpp"
#include <map>
#include <string>

namespace gs3v3 {
struct TimedRecord { Record record; uint32_t time; };
struct Reply {
    Bytes command;
    std::vector<TimedRecord> records;
    std::string message;
    bool failed=false;
    bool authenticated=false;
};

// Owned by si3stream, never shared between sensors. No change to the mmap
// database format. Reconstruct the clock from saved index/time pairs on reconnect.
struct Session {
    Address address{};
    Account account{};
    bool addressValid=false;
    int legacySubtype=-1;
    bool v3=false;
    bool attempted=false;
    bool authenticated=false;
    bool historyDone=false;
    uint32_t anchor=0;
    uint32_t liveTime=0;
    uint16_t liveIndex=0;
    uint32_t historyRequestedAt=0;
    uint16_t savedIndex=0;
    uint16_t cachedIndex=0;
    unsigned queryPosition=0;
    DeviceTime deviceTime{};
    std::map<uint16_t,Record> pending;
    static constexpr uint8_t queries[]{0x0f,0x0e,0x0f,0x03,0x11,0x01};

    void restore(uint16_t index,uint32_t time) {
        savedIndex=index;
        if(time>uint32_t(index)*60) anchor=time-uint32_t(index)*60;
    }
    void observeLive(uint16_t index,uint32_t now) {
        const uint32_t elapsed=uint32_t(index)*60;
        if(now<=elapsed||(liveTime&&index<=liveIndex)) return;
        const uint32_t candidate=now-elapsed;
        if(!anchor) anchor=candidate;
        // A restored clock can already be wrong. Confirm it against two
        // consecutive minute records, including records without a display value.
        // Reject bursts and use the earlier estimate to limit delivery jitter.
        constexpr int64_t tolerance=15;
        if(liveTime&&uint32_t(index)==uint32_t(liveIndex)+1) {
            const int64_t spacingError=int64_t(now)-liveTime-60;
            if(spacingError>=-tolerance&&spacingError<=tolerance) {
                const uint32_t confirmed=std::min(candidate,liveTime-uint32_t(liveIndex)*60);
                const int64_t correction=int64_t(confirmed)-anchor;
                if(correction>tolerance||correction<-tolerance) anchor=confirmed;
            }
        }
        liveTime=now;
        liveIndex=index;
    }
    Bytes encrypt(const Bytes &plain,EncryptBlock aes) const {
        Bytes out;
        if(addressValid) transport(address,plain,out,aes);
        return out;
    }
    Bytes authentication(EncryptBlock aes) {
        v3=true;
        attempted=true;
        return encrypt(auth_plain(address,account),aes);
    }
    static bool displayPresent(const Record &r) {
        // Values >= ff00 are protocol sentinels. The encrypted minute algorithm
        // field is not a fresh display result. present_cstate=1 marks invalid data.
        return r.display_glucose>0 && r.display_glucose<0xff00 && !r.present_cstate;
    }
    void flush(Reply &out) {
        if(!anchor||!historyDone) return;
        for(const auto &[index,record]:pending) {
            if(index>savedIndex) out.records.push_back({record,anchor+uint32_t(index)*60});
        }
        pending.clear();
    }
    void nextQuery(Reply &out, uint32_t now, EncryptBlock aes) {
        if(queryPosition<sizeof(queries)) {
            const auto query=queries[queryPosition];
            out.command=encrypt(query_plain(query),aes);
            out.message="GS3 V3 query "+std::to_string(query);
        } else if(cachedIndex&&savedIndex>=cachedIndex) {
            historyDone=true;
            out.message="GS3 V3 connected";
            flush(out);
        } else {
            if(savedIndex==0xffff) {historyDone=true;return;}
            historyRequestedAt=now;
            out.command=encrypt(history_plain(savedIndex+1),aes);
            out.message="GS3 V3 history from "+std::to_string(savedIndex+1);
        }
    }

    Reply process(const Bytes &p,uint32_t now,EncryptBlock aes) {
        Reply out;
        if(!valid_frame(p)) {out.message="GS3 V3 invalid packet";return out;}
        const auto cmd=p[1];
        if(p.size()==5&&cmd==0xe2) {
            if(p[2]!=1||p[3]!=0) {
                out.failed=true;
                out.message="GS3 V3 authentication failed: status "+std::to_string(p[2])+", error "+std::to_string(p[3]);
            } else if(!authenticated) {
                authenticated=true;
                out.authenticated=true;
                queryPosition=0;
                nextQuery(out,now,aes);
            }
            return out;
        }
        // Unsolicited bootstrap replies also occur during an authenticated session
        // in the capture. They must not restart a successful handshake.
        if(!authenticated) {
            if(p==Bytes{4,0,0,0,0xfc}) {
                out.failed=true;
                out.message="GS3 V3 authentication rejected";
            }
            return out;
        }
        if(cmd==0xf0) {
            if(queryPosition>=sizeof(queries)||p[2]!=queries[queryPosition]) return out;
            // Validate each query's observed response layout before advancing.
            const auto sub=p[2];
            if((sub==0x0f&&p.size()!=5)||(sub==0x0e&&p.size()!=29)||
               (sub==0x11&&p.size()!=5)||(sub==1&&p.size()!=8)) {
                out.failed=true;out.message="GS3 V3 malformed information response";return out;
            }
            if(sub==3) {
                if(!parse_device_time(p,deviceTime)) {
                    out.failed=true;out.message="GS3 V3 malformed time response";return out;
                }
                // Never apply a timezone offset or rewrite the sensor clock here.
                // This session covers sensors already activated by the official app.
                if(!deviceTime.activation_time) {
                    out.failed=true;
                    out.message="Activate this GS3 V3 sensor with the Sibionics app first";
                    return out;
                }
            }
            ++queryPosition;
            nextQuery(out,now,aes);
            return out;
        }
        if(cmd!=0x32&&cmd!=0x36&&cmd!=0x39) return out;
        if(p.size()==5) {
            if(p[2]!=1||p[3]!=0) {
                out.failed=true;
                out.message="GS3 V3 data request failed: status "+std::to_string(p[2])+", error "+std::to_string(p[3]);
            }
            return out; // A successful 39 ACK precedes the history, not its end.
        }
        if(cmd==0x39&&p.size()==8&&p[2]==0&&le16(p.data()+5)==0) {
            historyDone=true;flush(out);return out;
        }
        std::vector<Record> records;
        if(!parse_records(p,records,aes)) {
            out.message="GS3 V3 malformed glucose packet";return out;
        }
        // Only a single live record with no remaining backlog establishes/corrects time.
        // Neither the cached 36 record nor an arriving history burst may do this.
        if(cmd==0x32&&records.size()==1&&!records[0].remaining)
            observeLive(records[0].index,now);
        for(const auto &r:records) {
            if(cmd==0x36) cachedIndex=std::max(cachedIndex,r.index);
            if(r.index>savedIndex&&displayPresent(r)) pending[r.index]=r;
        }
        if(cmd==0x39&&!records.back().remaining) historyDone=true;
        if(!historyDone&&historyRequestedAt&&now-historyRequestedAt>120) {
            // Reconnect and request again from the last SAVED index. Do not append
            // a cached/live value ahead of missing history and thereby lose it.
            out.failed=true;out.message="GS3 V3 history did not finish; reconnecting";
            return out;
        }
        flush(out);
        if(!out.records.empty()) out.message="GS3 V3 receiving glucose";
        else if(!anchor) out.message="GS3 V3 waiting for a live timestamp";
        return out;
    }
};
}
