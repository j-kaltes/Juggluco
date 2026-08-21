/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Wed May 06 21:10:29 CEST 2026                                                */

#ifdef SIBIONICS

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <numeric>

#include "SiContext.hpp"
#include "inout.hpp"
#include "logs.hpp"
#include "share/hexstr.hpp"
class SensorGlucoseData;




void Rc4XorWithKey(const uint8_t *key, uint32_t key_len, int skip_count, const uint8_t *input, uint8_t *output, uint32_t out_len);



extern const uint8_t rc4key[16];
const uint8_t rc4key[16]{0x01,0x38,0x0b,0x9a,0x00,0x5b,0x02,0x5d,0xcd,0x9e,0xc3,0x99,0x09,0x37,0xaa,0xe8};

// ---------------------------------------------------------------------------
//  Constants
// ---------------------------------------------------------------------------


inline constexpr std::size_t   kPacketBufSize     = 250;     // 0xFA
inline constexpr std::size_t   kAckPacketLen      = 4;
inline constexpr std::size_t   kMaxOutputJsonSize = 0x960;   // 2400
inline constexpr std::uint32_t kRc4KeyLen         = 0x10;

// Status codes returned by v120_spilt_data
//inline constexpr jint kStatusOk            =  1;
inline constexpr jint kStatusEmpty         =  -1;   // valid header, zero records
inline constexpr jint kStatusBadArgs       = -2;
inline constexpr jint kStatusBadPacket     = -3;

// Magic bypass header: param==5 and data[0..3] = {0x04, 0, 0, 0} and data[4]==0xFC
inline constexpr std::uint8_t kBypassParam   = 5;
inline constexpr std::uint8_t kBypassMarker  = 0xFC;

// cmd_id values that escape the [0..0x66] range check
inline constexpr std::uint8_t kCmdAckExt         = 0xE3;  // routed to ACK group
inline constexpr std::uint8_t kCmdDeviceConfig   = 0xF0;
inline constexpr std::uint8_t kCmdErrorInfo      = 0xF2;

// ---------------------------------------------------------------------------
//  Packet-buffer view
// ---------------------------------------------------------------------------
//
//  The on-the-wire packet is laid out as:
//
//      offset 0 : payload length N (number of bytes from offset 0 to N-1)
//      offset 1 : cmd_id
//      offset 2 : status / sub_cmd
//      offset 3 : data[0]                 (or auxiliary status)
//      ...
//      offset N : checksum = -(sum of bytes 0..N-1) & 0xff
//
//  ACK packets are always 5 bytes total (length field == 4, checksum at [4]).
//
struct PacketBuf {
    std::array<std::uint8_t, kPacketBufSize> raw{};

    [[nodiscard]] std::uint8_t length()   const { return raw[0]; }
    [[nodiscard]] std::uint8_t cmd_id()   const { return raw[1]; }
    [[nodiscard]] std::uint8_t status()   const { return raw[2]; }
    [[nodiscard]] std::uint8_t aux()      const { return raw[3]; }
    [[nodiscard]] std::uint8_t checksum() const { return raw[length()]; }

    [[nodiscard]] std::span<const std::uint8_t> payload() const {
        return { raw.data() + 3, std::size_t(length() - 3) };
    }
};

// 1-byte two's-complement of the sum of  n  bytes, matching the firmware.
[[nodiscard]] std::uint8_t packet_checksum(const std::uint8_t* p, std::size_t n) {
    std::uint8_t s = 0;
    for (std::size_t i = 0; i < n; ++i) s = std::uint8_t(s + p[i]);
    return std::uint8_t(-s);
}

// Validate a fixed-length 5-byte ACK packet.
[[nodiscard]] bool ack_packet_is_valid(const PacketBuf& pkt, std::size_t received_size) {
    if (pkt.length() != kAckPacketLen || received_size <= kAckPacketLen) return false;
    const std::uint8_t expected =
        std::uint8_t(-(pkt.cmd_id() + pkt.status() + pkt.aux() + kAckPacketLen));
    return pkt.checksum() == expected;
}

// Validate a variable-length packet whose length field is in raw[0].
[[nodiscard]] bool var_packet_is_valid(const PacketBuf& pkt, std::size_t received_size) {
    const std::size_t n = pkt.length();
    // n is the checksum offset, so the received packet must contain byte n.
    if (n < 3 || n >= kPacketBufSize || n >= received_size) return false;
    return pkt.checksum() == packet_checksum(pkt.raw.data(), n);
}



// Emit a generic CMD_ACK reply for a single ACK-shaped packet.
[[nodiscard]] jlong emit_cmd_ack(const PacketBuf& pkt, std::size_t received_size) {
    if(!ack_packet_is_valid(pkt, received_size)) return 8LL;
    auto type =(pkt.cmd_id() | 0xC000u) + 1u;
    switch(type) {
           case 0xC009 /*49161*/: return 9LL;
           case 0xC004 /*49156*/: return 7LL;
           case 0xC001 /*49153*/: return 4LL;
           case 0xC002 /*49154*/:  {
                   auto status   = pkt.status();
                   if(status!=1) {
                      if((pkt.aux() & 0xff) != 3) {
                          return 4LL;
                          }
                      }
                    return 6LL;
                    };
            case 0xC008 /*49160*/: return 5LL;
           };
      return 8LL;
}




struct glucoseRecordsStart {
        uint16_t index;
        uint32_t itime;
        }__attribute__ ((packed));

struct glucoseItem {
        uint16_t temp;
        uint16_t dump;
        uint16_t current;
        uint16_t extra;
        }__attribute__ ((packed));


struct glucoseRecordsEnd {
        uint16_t reindex;
        uint8_t sign;
        }__attribute__ ((packed));
        /*
        uint8_t trend;
        uint8_t gwarn;
        uint8_t twarn;
        uint8_t cwarn; */


 

jlong SiContext::handleGlucoses(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,const std::uint8_t* decoded_records, std::size_t record_count) {
      const auto *header= reinterpret_cast<const glucoseRecordsStart*>(decoded_records);
      uint32_t itime=header->itime; 
      const glucoseItem *item= reinterpret_cast<const glucoseItem*>(decoded_records+sizeof(glucoseRecordsStart));
      int startindex=header->index ;
      int endindex=startindex +record_count;
      auto *ends=reinterpret_cast<const glucoseRecordsEnd *>(item+record_count);
      int reindex=ends->reindex+record_count-1;
    for(int index = startindex; index<endindex; ++index,--reindex,++item,itime+=60) {
        time_t tim=itime;
        int current=item->current;
        LOGGER("index=%d current=%d value=%.1f itime=%zu %.24s temp=%d dump=%d reindex=%d\n",index,current,current/10.0,tim,ctime(&tim), item->temp,item->dump,reindex);
        auto res=handleOneGlucose(sens,sensorindex,nowsecs,index,item->temp,item->current,reindex,itime);
        if(res>=0)
                return res;
        
       }
    return 1LL;
    }





jlong SiContext::interpret_data(SensorGlucoseData *sens,int sensorindex,uint32_t nowsecs,const uint8_t* data, jint datalen, jboolean doDecrypt) {
    if (!data || datalen <= 0 || static_cast<std::size_t>(datalen) > kPacketBufSize) {
        LOGGER("interpret_data invalid input data=%p datalen=%d\n", data, datalen);
        return kStatusBadArgs;
    }
    const std::size_t received_size = static_cast<std::size_t>(datalen);
    const std::uint8_t param_lo = std::uint8_t(datalen & 0xFFu);
    PacketBuf pkt{};
    const bool magic_bypass =
            (param_lo == kBypassParam) &&
            (data && data[0] == 0x04 && data[1] == 0 &&
                     data[2] == 0    && data[3] == 0    &&
                     data[4] == kBypassMarker);
    const bool  decr=!magic_bypass && (doDecrypt & 1u);
    if(decr) {
        Rc4XorWithKey(rc4key, kRc4KeyLen, 0, data, pkt.raw.data(), std::uint32_t(datalen));
        hexstr hex(pkt.raw.data(),datalen);
        LOGGER("interpret_data decrypted %s\n",hex.str());
    } else {
        std::memcpy(pkt.raw.data(), data, received_size);
    }


    const std::uint8_t cmd_id = pkt.cmd_id();

    const bool extended = (cmd_id == kCmdAckExt)       ||
                          (cmd_id == kCmdDeviceConfig) ||
                          (cmd_id == kCmdErrorInfo);

    if (cmd_id > 0x66 && !extended) { 
        return kStatusBadArgs;
        }

    // -----------------------------------------------------------------------
    // Stage 3 : dispatch on cmd_id
    // -----------------------------------------------------------------------
    switch (cmd_id) {

    // -------- Group A: simple ACK commands ---------------------------------
    // All of these accept only a 5-byte ACK packet; cmd_id==0xE3 is routed
    // here too (this is the case the previous decompilation got wrong).
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04:
    case 0x06: case 0x07: case 0x0B:
    case 0x10: case 0x11: case 0x12: case 0x16:
    case kCmdAckExt:                                   // 0xE3
        return emit_cmd_ack(pkt, received_size);

    // -------- Group B: ACK-or-data commands --------------------------------
    // For length==4 they behave like a Group A ACK; for length>4 they parse
    // a variable-length payload of typed records. Per-record byte sizes
    // were recovered from  &local_690 + i * N  in the Ghidra dump
    // (local_690 is uint16_t*, so each pointer-step advances by N * 2 bytes).
    case 0x08: {
        if(pkt.length() == kAckPacketLen)
            return emit_cmd_ack(pkt, received_size);

        if (!var_packet_is_valid(pkt, received_size)) return kStatusBadPacket;
        if(pkt.status() == 0) {
            return kStatusEmpty;
           }
        const std::size_t record_count = pkt.status();
        const std::size_t decoded_size = std::size_t(pkt.length()) - 3u;
        const std::size_t required_size = sizeof(glucoseRecordsStart) +
                record_count * sizeof(glucoseItem) + sizeof(glucoseRecordsEnd);
        if (required_size > decoded_size) {
            LOGGER("glucose packet too short: length=%u records=%zu decoded=%zu required=%zu\n",
                   pkt.length(), record_count, decoded_size, required_size);
            return kStatusBadPacket;
        }
        // glouse_info_t is 20 bytes (uint16 index, temp, current, dump,
        // reindex, glouse + 4 packed warning flags + 4-byte itime).
        return handleGlucoses(sens,sensorindex, nowsecs,pkt.raw.data() + 3,record_count);
    }

    case 0x0A:  
    case 0x3E:                                       // ACK or three_glouse_info_t array
    case 0x40:                                       // ACK or day_info_t (single)
    case 0x41:                                       // ACK or b_act_info_t array (10 bytes/rec)
    case 0x42:                                       // ACK or glouse_ble_info_t array (24 bytes/rec)
    case 0x66:                                       // ACK or tang_glouse_info_t array (44 bytes/rec)
    case kCmdErrorInfo:
    // ACK or only_glouse_info_t array (16 bytes/record)
        if (pkt.length() == kAckPacketLen)
            return emit_cmd_ack(pkt, received_size);
        if (!var_packet_is_valid(pkt, received_size)) return kStatusBadPacket;
        if (pkt.status() == 0) { return kStatusEmpty; }
        return 8LL;

    case 0x17:   
    case 0x18:   
    // algoVersion_ack_t  (single object, variable length)
        if (!var_packet_is_valid(pkt, received_size)) return kStatusBadPacket;
        return 8LL ;

    // -------- Extended cmd: 0xF2  (error info / ACK) -----------------------
    // -------- Extended cmd: 0xF0  (device-config sub-switch) ---------------
    case kCmdDeviceConfig: {
        if (!var_packet_is_valid(pkt, received_size)) return kStatusBadPacket;
        const std::uint8_t  sub  = pkt.status();
        if(sub==4) {
            return 10LL;
            }
        }

    default:
        return kStatusBadArgs;
    }
  }

void Rc4XorWithKey(const uint8_t *key, uint32_t key_len, int skip_count, const uint8_t *input, uint8_t *output, uint32_t out_len) {
    uint8_t S[256];

    /* KSA: initialise S-box */
    for (int i = 0; i < 256; i++)
        S[i] = (uint8_t)i;

    uint32_t j = 0;
    for (uint32_t i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) & 0xFF;
        uint8_t tmp = S[i]; S[i] = S[j]; S[j] = tmp;
    }

    /* PRGA: i and j are reset to 0 regardless of skip */
    uint32_t si = 0, sj = 0;

    /* Optional keystream discard (RC4-drop) */
    for (int k = 0; k < skip_count; k++) {
        si = (si + 1) & 0xFF;
        uint8_t a = S[si];
        sj = (sj + a) & 0xFF;
        uint8_t b = S[sj];
        S[si] = b; S[sj] = a;
    }

    /* XOR phase */
    for (uint32_t k = 0; k < out_len; k++) {
        si = (si + 1) & 0xFF;
        uint8_t a = S[si];
        sj = (sj + a) & 0xFF;
        uint8_t b = S[sj];
        S[si] = b;
        S[sj] = a;
        output[k] = S[(a + b) & 0xFF] ^ input[k];
    }
}

typedef unsigned int uint;
static int v120_raw_data(bool doencrypt,int index,int start0,uint8_t *output,int maxoutput) {
    struct rawpacket {
         uint16_t cmd;
         uint16_t index;
         uint16_t start;
         int8_t checksum;
        }  __attribute__ ((packed));
    char inb2 = (char)((uint)index >> 8);
    char startb2 = (char)((uint)start0 >> 8);
    rawpacket pack{.cmd=0x806,.index=(uint16_t)index,.start=(uint16_t)start0,
         .checksum=(int8_t)(-0xe - ((char)index + inb2 + (char)start0 + startb2))};


  if ((output != nullptr && (6 < maxoutput))) {
    if (doencrypt) {
         Rc4XorWithKey(rc4key,0x10,0,(uint8_t *)&pack,output,7);
        }
    else {
        memcpy(output,&pack,7);
      }
  return 7;
 }
 else return -2;
}

extern Data_t askindexdata(jlong index);
Data_t askindexdata(jlong index) {
   Data_t dat(30);
   int res=v120_raw_data(true,index, 0,(uint8_t *)dat.data->data(), dat.capacity());
   dat.used=res;
  return dat;
   }

struct SecPayload {
    uint16_t magic;
    union {
        uint32_t time;   
        uint8_t tb[4];
        };
    uint8_t checksum; 
} __attribute__ ((packed));


int v120_isec_update(bool btrue, uint32_t timesec, uint8_t *output, uint16_t maxoutput) {
    if (output == NULL || maxoutput <= 6) {
        return -2;
    }

    struct SecPayload payload {.magic = 0x0306,.time=timesec};
    payload.checksum = (uint8_t)(-9 - (payload.tb[0] + payload.tb[1] + payload.tb[2] + payload.tb[3]));
    
    if (btrue) {
        Rc4XorWithKey(rc4key, 0x10, 0, (uint8_t*)&payload, output, 7);
    } else {
        memcpy(output, &payload, 7);
    }

   hexstr fillhex(output,7);
   LOGGER("getIsecUpdate(%u) res=%s\n",timesec,fillhex.str());
    return 7;
   }

static const uint8_t *getkey(int hema) {
     static constexpr const char *keys[] {
        "THE544U0TYITE461",// com.sisensing.sijoy 
        "LQSS54U0RURUA99J",// com.sisensing.rusibionics 
        "GKSHGDU0TYA456G4",// com.sisensing.sisensingcgm 
        "GKSHGDU0TYA456G4",// com.sisensing.eco 
        "THE544U0TYITE461",// com.sisensing.gs3 
        "GKSHGDU0TYA456G4",// com.sibionics.gstoc  Guess
        };
     return (const uint8_t*)keys[hema];
    }
   // int authlen=v120_apply_authentication(3,true,0, (const uint8_t*)dev.data(), authbuf, maxbuf);
int64_t v120_apply_authentication(int sisubtype, const uint8_t* deviceName, uint8_t* dest_buf, uint16_t dest_size) {
    auto *registered_block=getkey(sisubtype);
    constexpr const uint64_t flag=true;
    constexpr const uint8_t param_2=0;


    // Basic validation: dest buffer must exist and size must be at least 26 bytes
    if (dest_buf == NULL) {
        return -2;
    }
    if (dest_size < 26) {
        return -2;
    }

    // Allocate the 26-byte buffer on the stack (local_48 in assembly)
    uint8_t buf[26];

    // 1. Initialize Header
    buf[0] = 0x19;      // Assembly stored 0x0119 as a uint16_t (little-endian)
    buf[1] = 0x01;
    buf[2] = param_2;

    // 2. Copy 6 bytes from deviceName
    // Assembly does this via a 32-bit load/store and a 16-bit load/store
    memcpy(&buf[3], deviceName, 6);

    // 3. Copy 16 bytes from DAT_00128170
    // Assembly does this via a 128-bit vector register copy (q0)
    memcpy(&buf[9], registered_block, 16);

    // 4. Calculate Checksum
    // The assembly sums the bytes and effectively calculates:
    // checksum = -(sum of buf[0..24]) so that sum(buf[0..25]) == 0 modulo 256
    uint8_t sum = 0;
    for (int i = 0; i < 25; i++) {
        sum += buf[i];
    }
    buf[25] = (uint8_t)(0 - sum); 

    // 5. Final output handling
    // tbz param_1, #0x0 checks the lowest bit of param_1
    if ((flag & 1) != 0) {
        // If bit 0 is set, encrypt the buffer into dest_buf
        Rc4XorWithKey(rc4key, 16, 0, buf, dest_buf,dest_size);
    } else {
        // If bit 0 is clear, just do a raw copy. 
        // Assembly uses overlapping 128-bit loads/stores (q0, q1) to copy 26 bytes efficiently
        memcpy(dest_buf, buf, 26);
    }

    return 26; // Return the number of bytes written
}

static int v120_activation(bool doencrypt,uint32_t timesec,uint32_t param_3,uint8_t *output, uint16_t maxoutput) {
  if((output == (uint8_t *)0x0) ||( maxoutput<=10)) 
      return -2;
   struct activation {
     uint16_t magic;
        union {
            struct {
                 uint32_t time;
                 uint32_t num;
                 };
             uint8_t ar[sizeof(time)+sizeof(num)];
             } __attribute__ ((packed));
     int8_t checksum;
     } __attribute__ ((packed));
     activation unencrypt {.magic=0x70a,.time=timesec,.num=param_3};
     unencrypt.checksum=-0x11-std::reduce(std::begin(unencrypt.ar),std::end(unencrypt.ar));
    if(doencrypt) {
      Rc4XorWithKey(rc4key,0x10,0,(const uint8_t*)&unencrypt,output,11);
       }
    else {
        memcpy(output,&unencrypt,11);
      ;
      }
 return 11;
}

int getActivation(uint32_t timesec,uint8_t *output,int maxoutput) {
   int res=v120_activation(true, timesec,1234,output,  maxoutput);
   hexstr fillhex(output,res);
   LOGGER("getActivation(%u) res=%s\n",timesec,fillhex.str());
   return res;
   }
#endif
