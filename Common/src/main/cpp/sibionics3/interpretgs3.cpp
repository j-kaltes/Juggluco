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
/*      Wed May 06 21:11:14 CEST 2026                                                */

#ifdef SIBIONICS
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <bit>
#include <cstdint>
#include <cstring>
#include "logs.hpp"
#include "hexstr.hpp"
#include "SensorGlucoseData.hpp"
#include "sensoren.hpp"
#include "datbackup.hpp"
#include "EverSense.hpp"
#include "glucose.hpp"

extern Sensoren *sensors;
using namespace std::literals;
 
typedef int64_t  jlong; 

extern const uint8_t rc4key[16];
extern void Rc4XorWithKey(const uint8_t *key, uint32_t key_len, int skip_count, const uint8_t *input, uint8_t *output, uint32_t out_len) ;


static uint64_t v120_gs3_raw_glouse_data(bool encrypt, uint16_t start, uint16_t end, uint8_t* buffer, uint16_t capacity) {
    uint32_t sum = start + (start >> 8) + end + (end >> 8);
    struct {
     uint16_t magic;
     uint16_t start;
     uint16_t end;
     uint8_t checksum;
     } __attribute__ ((packed)) cmd= {
         .magic=0x1406,
         .start=start,
         .end=end,
         .checksum= static_cast<uint8_t>(0xFFFFFFE6 - sum)
         };

    if (encrypt) {
        Rc4XorWithKey(rc4key, 16, 0, (const uint8_t *)&cmd, buffer,7);
    } else {
        memcpy(buffer,(const uint8_t *)&cmd , 7);
    }
   return 7; 
   }

int v120_device_information(bool encrypt,char i2,uint8_t *output,uint16_t maxoutput) {
    struct  {
        uint16_t magic;
        char num;
        uint8_t dminnum;
        } out{
         .magic=0xf003,
         .num=i2,
        .dminnum=(uint8_t)(0xd-i2)
        };
    if (encrypt) {
      Rc4XorWithKey(rc4key,0x10,0,(const uint8_t *)&out,(uint8_t *)output,4);
    }
    else {
        memcpy(output,&out,4);
    }
   return 4;
}




inline constexpr std::uint16_t kCommandId   = 0x130F; // header (LE)
inline constexpr std::size_t   kPacketSize  = 16;
inline constexpr std::size_t   kPayloadSize = 12;     // 8 + 4 bytes


std::int32_t v120_glouse_id_bound(bool doEncrypt, std::uint8_t seq, const std::uint8_t payload[kPayloadSize], std::uint8_t* out, std::uint16_t out_size) noexcept {
    std::uint8_t packet[kPacketSize]{};
    packet[0] = static_cast<std::uint8_t>(kCommandId & 0xFF);   // 0x0F
    packet[1] = static_cast<std::uint8_t>(kCommandId >> 8);     // 0x13
    packet[2] = seq;
    std::memcpy(&packet[3], payload, kPayloadSize);

    // Sum-to-zero checksum over the whole packet.
    std::uint8_t sum = 0;
    for (std::size_t i = 0; i < kPacketSize - 1; ++i) {
        sum = static_cast<std::uint8_t>(sum + packet[i]);
    }
    packet[kPacketSize - 1] = static_cast<std::uint8_t>(-sum);

    if (doEncrypt) {
        Rc4XorWithKey(rc4key, 16,0, packet, out, kPacketSize);
    } else {
        std::memcpy(out, packet, kPacketSize);
    }
    return static_cast<std::int32_t>(kPacketSize);
}


void si3AskNewData(std::vector<uint8_t> &vect,int nextid) {
    constexpr const int maxbuf=7;
    vect.resize(maxbuf);
    v120_gs3_raw_glouse_data(true, nextid, 0, vect.data(),maxbuf);
#ifndef NOLOG
    hexstr hex(vect.data(),maxbuf);
    LOGGER("si3AskNewData(nextid=%d)=%s\n",nextid,hex.str());
#endif
    }


void si3DeviceInfo(std::vector<uint8_t> &vect,char i) {
    constexpr const uint16_t maxbuf=4;
    vect.resize(maxbuf);
    v120_device_information(true, i, vect.data(),maxbuf);
#ifndef NOLOG
    hexstr hex(vect.data(),maxbuf);
    LOGGER("si3DeviceInfo(i=%d)=%s\n",i,hex.str());
#endif
    }

//i=1 or 7

void si3bendUser(std::vector<uint8_t> &vect,const uint8_t beNumber[kPayloadSize], uint8_t seq) {
    constexpr const int maxbuf=kPacketSize;
    vect.resize(maxbuf);
    uint8_t  *buf=vect.data();
    v120_glouse_id_bound(true, seq, beNumber, buf, maxbuf);
#ifndef NOLOG
    hexstr behex(beNumber,kPayloadSize);
    hexstr hex(buf,maxbuf);
    LOGGER("si3bendUser(seq=%hhi,beNumber=%s)=%s\n",seq,behex.str(),hex.str());
#endif
    }


namespace {

#pragma pack(push, 1)
struct ActivationPacket {
    std::uint8_t  cmd_lo;    // 0x06
    std::uint8_t  cmd_hi;    // 0x0F
    std::uint32_t timesec;   // little-endian on the wire
    std::uint8_t  checksum;  // sum of all 7 bytes == 0 (mod 256)
};
#pragma pack(pop)
static_assert(sizeof(ActivationPacket) == 7);

constexpr std::uint8_t kCmdLo = 0x06;
constexpr std::uint8_t kCmdHi = 0x0F;

// Sum of the four little-endian bytes of `v`, taken mod 256.
constexpr std::uint8_t byte_sum(std::uint32_t v) noexcept {
    return static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(v      ) +
        static_cast<std::uint8_t>(v >>  8) +
        static_cast<std::uint8_t>(v >> 16) +
        static_cast<std::uint8_t>(v >> 24));
        }

} // namespace


static int v120_glouse_ketone_activation_nosens(bool encrypt, std::uint32_t timesec, uint8_t *output, std::uint16_t maxoutput) noexcept {
    ActivationPacket pkt{};
    pkt.cmd_lo   = kCmdLo;
    pkt.cmd_hi   = kCmdHi;
    pkt.timesec  = timesec;
    pkt.checksum = static_cast<std::uint8_t>(-(kCmdLo + kCmdHi + byte_sum(timesec)));

    if (encrypt) {
        Rc4XorWithKey(rc4key, 16, /*skip=*/0,
                      reinterpret_cast<const std::uint8_t *>(&pkt),
                      output, sizeof(pkt));
    } else {
        std::memcpy(output, &pkt, sizeof(pkt));
    }

    return static_cast<int>(sizeof(pkt));
   }

void si3NoSense(std::vector<uint8_t> &vect,uint32_t timesec) {
    constexpr const int maxbuf=7;
    vect.resize(maxbuf);
    uint8_t  *buf=vect.data();
    v120_glouse_ketone_activation_nosens(true,timesec, buf, maxbuf);
#ifndef NOLOG
    hexstr hex(buf,maxbuf);
    LOGGER("si3NoSense(timesec=%u)=%s\n",timesec,hex.str());
#endif
    }






struct cgmv120_u16sens_t              { uint16_t value; };
struct cgmv120_u8activation_t         { uint8_t  value; };
struct cgmv120_device_time_t          { uint16_t f00; uint8_t f02; uint8_t _3; uint16_t f04, f06; };

namespace {

bool verify_checksum(const uint8_t* p, uint32_t payload_len) {
    uint8_t s = 0;
    for (uint32_t i = 0; i < payload_len; ++i) s += p[i];
    return p[payload_len] == (uint8_t)-s;
}


} // namespace



struct cgmv120_cmd_ack_t {
    uint16_t reply_ack_type;       // f00
    uint8_t  reply_ack_resule;     // f02 — firmware: "u8reply_ack_resule"
    uint8_t  error_code;           // f03
};                                                                  // 4 bytes
void struct_to_json_cgmv120_cmd_ack_t(const cgmv120_cmd_ack_t *param) {
LOGGER("ack:\nu16reply_ack_type=%u\n",param->reply_ack_type);
LOGGER("u8reply_ack_resule=%u\n",param->reply_ack_resule);
LOGGER("u8error_code=%u\n\n",param->error_code);
}
/*struct cgmv120_device_reset_t         { 
    uint16_t f00; 
    uint8_t f02, _3; 
    uint16_t f04; 
    uint32_t f08; }; */
struct cgmv120_device_reset_t {
    uint8_t  reset_iswatchdog;     // f00
    uint8_t  reset_times;          // f01
    uint8_t  reset_synchron;       // f02
    uint8_t  _pad3;
    uint16_t reset_index;          // f04
    uint16_t _pad6;
    uint32_t reset_time_stamp;     // f08
};                                                                  // 12 bytes
#ifndef NOLOG
void struct_to_json_cgmv120_device_reset_t(cgmv120_device_reset_t *param) {
    LOGAR("device_reset");
    LOGGER("u8reset_iswatchdog=%u\n",param->reset_iswatchdog);
    LOGGER("u8reset_times=%u\n",param->reset_times);
    LOGGER("u8reset_synchron=%u\n",param->reset_synchron);
    LOGGER("u16reset_index=%u\n",param->reset_index);
    LOGGER("ireset_time_stamp=%u\n\n",param->reset_time_stamp);
    }
void struct_to_json_cgmv120_device_time_t(cgmv120_device_time_t *param)
{
LOGAR("device_time_t\n");
}
#endif


uint8_t sub_28ad0(uint8_t value, uint16_t ack_id) {
    if (ack_id < 0xC002 || ack_id > 0xC067) return 0x0B;

    switch (ack_id) {
    case 0xC002:
        if (value == 3) return 0x09;
        if (value == 2) return 0x08;
        return 0x0B;                                 // (no special "out-of-range" return here)

    case 0xC004:
        if (value == 4) return 0x02;
        if (value == 2) return 0x01;
        return 0xFF;

    case 0xC005:
        return value == 5 ? 0x07 : 0xFF;

    case 0xC006: case 0xC008: case 0xC010:
        if (value == 5) return 0x04;
        if (value == 4) return 0x03;
        return 0xFF;

    case 0xC007:
        if (value == 5) return 0x0C;
        if (value == 4) return 0x02;
        return 0xFF;

    case 0xC011:
        if (value == 5) return 0x06;
        if (value == 4) return 0x02;
        return 0xFF;

    case 0xC012:
        if (value == 5) return 0x0F;
        if (value == 4) return 0x0E;
        return 0xFF;

    case 0xC013: {                                    // table { 0x11, 0x09, 0x10 } for value=2..4
        uint8_t i = (uint8_t)(value - 2);
        if (i >= 3) return 0xFF;
        return (uint8_t)((0x100911u >> (i * 8)) & 0xFF);
    }

    case 0xC014: {                                    // tbb-driven; values 1 and 3 fall through
        if ((uint8_t)(value - 1) > 4) return 0xFF;
        switch (value) {
        case 2: return 0x15;
        case 4: return 0x13;
        case 5: return 0x14;
        default: return 0xFF;                         // value == 1 or 3
        }
    }

    case 0xC015: case 0xC016:
    case 0xC03F: case 0xC067: {                       // table { 0x02, 0x05, 0x0D } for value=4..6
        uint8_t i = (uint8_t)(value - 4);
        if (i >= 3) return 0xFF;
        return (uint8_t)((0x0D0502u >> (i * 8)) & 0xFF);
    }

    default:
        return 0x0B;
    }
}

static   float sib3totrend(int sib3trend) {
                switch(sib3trend) {
                       case 0: return 0.0f;
                       case 1: return 1.05f;
                       case 2: return 4.0f;
                       case 3: return -1.05f;
                       case 4: return -4.0f;
                       default: return NAN;
                       }
                }

static int sib3trend2abbotttrend(int sib3trend) {
        switch(sib3trend) {
               case 0: return 3;
               case 1: return 4;
               case 2: return 5;
               case 3: return 2;
               case 4: return 1;
               default: return 0;
               }
        }
static bool strangeStarttime(SensorGlucoseData *sens,int index,uint32_t eventTime) {
   const uint32_t should=eventTime-index*60;
   auto starttime= sens->getinfo()->starttime;
   if(abs((int)(should-starttime))>60)
      return true;
    return false;   
    }

extern int rate2changeindex(float rate);
#include "../calibrate/calculate.hpp"
static float makearrow(const SensorGlucoseData *sens,float mgdL,uint32_t was)  {
    std::vector<double> w {1.0,1.0,0.5};
    std::vector<double> x,y;
    x.push_back(was);
    y.push_back(mgdL);

    const auto stream=sens->getPolldata();
    int el=1;
    for(const ScanData *iter=&stream.back();el<3&&iter>=&stream.begin()[0];--iter,++el) {
            y.push_back(iter->getmgdL());
            x.push_back(iter->gettime());
            }
    if(el>1) {
          return getA(w,x,y,x.size())*50;
        }
    return NAN;
    }
jlong gs3Glucose(SensorGlucoseData *sens,std::vector<uint8_t> &vect,std::string &message,const uint8_t* in_packet, int in_len, uint32_t &nowsecs) {
    constexpr const bool use_rc4=true;
    // BN names: var_228 / var_128 / var_540 — see header comment.
    uint8_t plain[0xFA] = {};   // var_228, scratch (zeroed up front)
    uint8_t buf  [0xFA];        // var_128, working copy of the packet

    // -------------------------------------------------------------
    // 1. Copy or RC4-decrypt the incoming packet into `buf`.
    //    The 5-byte bootstrap form (04 00 00 00 FC) is always
    //    plaintext; otherwise honor `use_rc4`.
    // -------------------------------------------------------------
    bool magic_hdr  = (in_len    == 5 &&
                       in_packet[0] == 4 &&
                       in_packet[1] == 0 &&
                       in_packet[2] == 0 &&
                       in_packet[3] == 0);
    bool is_unencrypted_special = false;     // BN: r5_1

    if (magic_hdr && (in_packet[4] == 0xFC || !use_rc4)) {
        is_unencrypted_special = (in_packet[4] == 0xFC);
        std::memcpy(buf, in_packet, in_len);
    } else if (!use_rc4) {
        std::memcpy(buf, in_packet, in_len);
    } else {
        // RC4(key @ 0x2ffd9 [arm32] / 0x122e9e [arm64], len = 0x10, iv = 0)
        Rc4XorWithKey(rc4key, 0x10, 0, in_packet, buf, in_len);
    }
#ifndef NOLOG
    hexstr decrypted(buf,in_len);
    LOGGER("gs3Glucose decrypted %s\n",decrypted.str());
#endif
    jlong result = 1LL;                              // -3

    const uint8_t length_byte = buf[0];               // payload length in `buf`
    const uint8_t cmd         = buf[1];               // command byte



    auto handle_ack = [&]() -> bool {
        if (length_byte != 4) return false;
        uint8_t b1 = buf[1], b2 = buf[2], b3 = buf[3];
        uint8_t expect = (uint8_t)(-4 - (b1 + b2 + b3));
        if (buf[4] != expect) {  return true; }

        cgmv120_cmd_ack_t ack{};
        ack.reply_ack_type   = (uint16_t)((cmd | 0xC000u) + 1);  // -> 0xC0?? family
        ack.reply_ack_resule = b2;

        if (cmd == 0x13) {
            ack.error_code = b3;
        } else if (b2 == 0) {
            uint8_t v = sub_28ad0(b3, ack.reply_ack_type);
            if (is_unencrypted_special) v = 0x0A;
            ack.error_code = v;
            }
#ifndef NOLOG
        struct_to_json_cgmv120_cmd_ack_t(&ack);
#endif
        switch(ack.reply_ack_type) {
                case 49154: 
                        message="bindUser 1"s;
                        si3bendUser(vect,settings->data()->gs3id, 1);
                        break;
                case 49172: {
                        int error=ack.error_code;
                        if(!error) {
                            message="deviceinfo 1"s;
                            si3DeviceInfo(vect,1);
                             }
                         else {
                            if(ack.reply_ack_resule==2) {
                                message="Wrong account ID";
                                result=2LL;
                            }
                            else {
                                 message="error "s+std::to_string(error)+", bindUser 2"s;
                                 si3bendUser(vect,settings->data()->gs3id, 2);
                                }
                            }

                        break;

                        }
                case 49168: {
                        message="SItime"s;
                        extern void   getSItimecmd(std::vector<uint8_t> &vect);
                         getSItimecmd(vect);break;
                         }
                case 49156:{
                        int nextid=1;
                        if(const ScanData *last=sens->lastValidStream()) {
                                nextid=last->getid()+1;
                                }
                        message.append("AskNewData "sv);
                        message.append(std::to_string(nextid));
                        si3AskNewData(vect,nextid);
                        break;
                        }
                case 49173: {
                    result=3LL;
                    };break;
                };
        return true;
    };




    switch(cmd) {
        case 0x14:
     {
        if (handle_ack()) break;                      // length==4 -> ACK form

        std::memcpy(plain, buf, length_byte + 1);
        if (!verify_checksum(plain, length_byte)) { break; }

        const uint8_t count = plain[2];
        uint16_t startIndex = *(const uint16_t*)&plain[3];
        uint32_t startTime = *(const uint32_t*)&plain[5];
        uint16_t last_reindex= (uint16_t)(plain[length_byte-2] | (plain[length_byte-1] << 8));
        LOGGER("interpret count=%d\n",count);
        if(count==0) {
            break;
            }
        int mgdL;
        float flchange;
        uint32_t eventTime=0;
        uint16_t index=0;
        for(uint32_t i = 0;i<count;++i) {
                const uint8_t* r = &plain[9 + i * 8];
    //            uint8_t  b9 = r[0], ba = r[1];
    //            uint16_t bb = *(const uint16_t*)&r[2];     // raw +0xb halfword
    //            uint16_t bd = *(const uint16_t*)&r[4];     // raw +0xd halfword
                uint8_t  bf = r[6], b10 = r[7];
    /*
                auto temp     = (uint16_t)((ba  << 2) | (b9 >> 6));
                auto c1       = bd;                     // <- raw +0xd
                auto dump     = bb;                     // <- raw +0xb
              //  assert(reindex== (uint16_t)(end_idx + (uint16_t)~i));
                auto gcwarn   = (uint8_t )( bf       & 1);
                auto twarn    = (uint8_t )( b9       & 1);
                auto shedding = (uint8_t )((b9 >> 1) & 1);
                */
                int mmolLx10      = (uint16_t)((b10 << 2) | (bf >> 6));


                 auto hdr_index=startIndex+i;
            if(hdr_index%5==0) {
                 mgdL=round(mmolLx10*.1*convfactordL);
                index=hdr_index;
                eventTime=startTime+i*60;                          

                flchange=makearrow(sens, mgdL,eventTime) ;
                int abbott=rate2changeindex(flchange);

                sens->savestream(eventTime,index,mgdL,abbott,flchange);
                 #ifndef NOLOG
                 time_t tim=startTime+i*60;
                 uint8_t schange  = (uint8_t )((bf >> 3) & 7);
                 auto abbotts=sib3trend2abbotttrend(schange);
                 LOGGER("interpretgs3 index=%d %d %.1f mmol/L raw change=%d %s  my %s ch=%.1f %u %s",hdr_index,mgdL,mgdL/18.0,schange,GlucoseNow::trendString[abbotts],GlucoseNow::trendString[abbott],flchange,eventTime,ctime(&tim));
                 #endif
                }
             else {
                 }
            }
    if(index)  {
     const int sensorindex=sens->sensorIndex;
     sensor *sensor=sensors->getsensor(sensorindex);
        if(index==5||strangeStarttime(sens,index,eventTime)) {
extern uint32_t makestarttime(int index,uint32_t eventTime);
                   const auto starttime=makestarttime(index,eventTime);
                   sens->getinfo()->starttime=starttime;
                   sensor->starttime=starttime;
                   sensors->setindices();
                   backup->resendResetDevices(&updateone::sendstream);
                   }
        if(!last_reindex) {
                uint32_t starttime=sens->getinfo()->starttime;
                uint32_t warminutes=sens->getinfo()->manualwarmup;
                uint32_t endwarmup=starttime+warminutes*60;
                if((nowsecs-eventTime)<maxbluetoothage) {
                      if(eventTime>endwarmup) {
                             sens->sensorerror=false;
                             if(sensor->finished) {
                                    sensor->finished=0;
                                    LOGGER("gs3Glucose finished=%d\n", sensor->finished);
                                    backup->resensordata(sensorindex);
                                    }
        extern jlong glucoseback(uint32_t nu,uint32_t glval,float drate,SensorGlucoseData *hist) ;
                             auto res=glucoseback(eventTime,mgdL,flchange,sens);
                             backup->wakebackup(wakestream);
                             extern void wakewithcurrent();
                             wakewithcurrent();

            #ifdef OLDEVERSENSE
                              sendEverSenseold(sens,5);
            #endif
                               nowsecs=eventTime;
                               return res;
                            }
                         else {
                              sens->getinfo()->warmupstartpos=std::min((int)sens->getinfo()->pollcount-1,0);
                              }
                      }
                 }
         else {
                 sens->receivehistory=nowsecs;
                 }
           const int last=sens->pollcount()-1;
           if(last<sens->getbroadcastfrom()) sens->setbroadcastfrom(last);
        }
        
        break;
    };

    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04:
    case 0x05: case 0x06: case 0x0C:
    case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
        if (!handle_ack()) {
                LOGGER("other command %x wrong\n",cmd);
                }
         else
                LOGGER("other command %x\n",cmd);
        break;

    case 0x15: {
        if (handle_ack()) break;
        LOGAR("gs3_only_glouse_info");
        break;
         }

    case 0x88: {
        if (handle_ack()) break;
        LOGAR("only_glouse_ketone_adc_info_t");
        break;
        }

    case 0xF0: {
        std::memcpy(plain, buf, length_byte + 1);
        if (!verify_checksum(plain, length_byte)) { break; }

        const uint8_t sub = plain[2];
        if ((uint32_t)(sub - 1) > 0xC) {              // sub == 0 or sub > 0xD
             LOGGER("after 0xF0 %x too large\n",sub);
            break;
          }

        switch (sub) {
            case 0x01: {                                  // u16 sensor reading
                cgmv120_u16sens_t s{};
                s.value = *(const uint16_t*)&plain[3];
                LOGGER("u16sens_t %x\n",s.value);
                message="deviceinfo 7"s;
                si3DeviceInfo(vect,7);
                break;
            }
            case 0x02: {                                  // u8 activation
                cgmv120_u8activation_t a{};
                a.value = plain[3];
                LOGGER("u16activiation_t %x\n",a.value);
     //           struct_to_json_cgmv120_u8activation(&a);
                break;
            }
            case 0x03: {                                  // device time
#ifndef NOLOG
                cgmv120_device_time_t t{};
                t.f00 = *(const uint16_t*)&plain[3];
                t.f02 = plain[5];
                t.f04 = *(const uint16_t*)&plain[11];
                struct_to_json_cgmv120_device_time_t(&t);
#endif
                break;
            }
            case 0x04: {                                  // device storage
                LOGAR("storage");
                break;
            }
            case 0x05: {                                  // device calibration (4 x uint32)
              LOGAR("calibration");
                break;
            }
            case 0x06: {                                  // secret key (16 bytes)
                LOGAR("secret_key");
                break;
            }

            case 0x07: {                                  // device reset info
#ifndef NOLOG
                cgmv120_device_reset_t r{};
                r.reset_iswatchdog = plain[3];
                r.reset_times      = plain[4];
                r.reset_synchron   = plain[5];
                r.reset_index      = *(const uint16_t*)&plain[6];
                r.reset_time_stamp = *(const uint32_t*)&plain[8];

                struct_to_json_cgmv120_device_reset_t(&r);
#endif
                si3NoSense(vect,time(nullptr));
                message="NoSense"s;
                break;
            }
            case 0x08: {                                  // glucose threshold
                LOGAR("glousethreshold");
                break;
            }
            case 0x09: {                                  // oscillator
                LOGAR("oscillator");
                break;
            }
            case 0x0A: {                                  // watchdog
                LOGAR("watchdog_t");
                break;
            }
            case 0x0B: {                                  // glucose+ketone u16 sensor pair
                LOGAR("glouse_ketone_u16sens_t");
                break;
            }
            case 0x0C: {                                  // life
                LOGAR("life_t");
                break;
            }
            case 0x0D: {                                  // device id (1 + 8 bytes)
                LOGAR("id_t");
                break;
            }
        }

        break;
    }

    default: LOGGER("default: other command=%x\n",cmd);
    }

    return result;
  }
  #endif
