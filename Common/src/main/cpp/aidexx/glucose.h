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
/*      Fri Mar 27 17:12:36 CET 2026                                                 */
#include <string_view>
#include <stdint.h>

#include <time.h>
#include "crc.h"
#ifndef LOGGER
#define LOGGER printf
#endif

struct CurrentGlucose {
    uint16_t one16; 
    uint8_t zero1;
    int8_t trend;
    uint16_t minfromstart;
    uint16_t glucose:10;
    uint16_t warmup:1;
    uint16_t unknown:4;
    uint16_t valid:1;
    uint8_t rest[7];
    } __attribute__ ((packed)) ;
struct HistoryGlucose {
    uint16_t glucose:10;
    uint16_t warmup:1;
    uint16_t unknown:4;
    uint16_t valid:1;
    } __attribute__ ((packed)) ;
struct NextGlucose {
    uint16_t glucose:10;
    uint16_t unknown:5;
    uint16_t valid:1;
    uint8_t quality;
    } __attribute__ ((packed)) ;
struct LastPast {
    uint16_t minfromstart;
    uint8_t status;
    uint8_t calTemp;
    int8_t trend;
    uint16_t glucose:10;
    uint16_t unknown:5;
    uint16_t valid:1;
    uint8_t quality;
    void log(std::string_view label,uint32_t starttime) const {
            const auto id=minfromstart;
            time_t tim=starttime+id*60;
            LOGGER("%.*s id=%d glucose=%d %.1f mmol/L trend=%d quality=%d %s",(int)label.size(),label.data(),id,glucose, glucose/18.0f, trend,quality,ctime(&tim));
            }
    } __attribute__ ((packed)) ;

/*
2026-02-28T23:02:08.893 26216 Java: WRITE_TYPE_NO_RESPONSE writeCharacteristic UUID 0000f002-0000-1000-8000-00805f9b34fb 64 65 E9
2026-02-28T23:02:08.954 26232 Java: AidexBle onCharacteristicChanged 0000f002-0000-1000-8000-00805f9b34fb 64 84 9E 27 4D 55 9E 9C CF 15 62 BA 12 81
2026-02-28T23:02:08.955 26216 start Java_com_microtechmd_blecomm_BleAdapter_onReceiveData__I_3B(61442, 64 84 9E 27 4D 55 9E 9C CF 15 62 BA 12 81)


2026-02-28T23:02:08.966 26216 I/gumshim   ret = 0x1  end AidexXController::getDevCommType() const
2026-02-28T23:02:08.981 26216 I/gumshim start AesEncryptor::CFB_decrypt(unsigned char*, unsigned char) bytes(14)   param0 before hex: 64 84 9e 27 4d 55 9e 9c cf 15 62 ba 12 81
2026-02-28T23:02:09.012 26216 I/gumshim end AesEncryptor::CFB_decrypt(unsigned char*, unsigned char)                  arg1 after hex: 11 01 94 4b 00 00 fb 77 80 4d 00 00 e8 44
2026-02-28T23:02:09.034 26216 Java: Operation:101, Success:true, Data:94,4B,00,00,FB,77,80,4D,00,00,
2026-02-28T23:02:09.034 26216 Java: D/onMessage= {resCode=1, operation=101, success=true, data=[-108, 75, 0, 0, -5, 119, -128, 77, 0, 0], messageType=NORMAL} AiDEX-X
2026-02-28T23:02:09.035 26216 Java: Advertising (type:2) --> AidexXBroadcastEntity{abstractEntity=AidexXAbstractEntity{timeOffset=19348, status=0, calTemp=0, trend=-5, calIndex=0}, history=[history:[timeOffset=19348, glucose=119, status=0, quality=77, isValid=1]], historyCount=1}
*/                                                              
#include "Showhex.hpp"
union    AdvertiseBytes {
    uint8_t bytes[62];
struct {
      uint8_t flags_len;      // 0x02
      uint8_t flags_type;     // 0x01
      uint8_t flags_val;      // 0x06

        // --- AD Structure 2: Service UUID (4 bytes) ---
      uint8_t uuid_len;       // 0x03
      uint8_t uuid_type;      // 0x02
      uint16_t service_uuid;  // 0x181F (Weight Scale)

        // --- AD Structure 3: Manufacturer Data (25 bytes) ---
     uint8_t mfg_len;        // 0x17 (23 bytes following)
     uint8_t mfg_type;       // 0xFF
     uint16_t company_id;    // 0x0059 (Nordic)

    LastPast lastglucose;
    NextGlucose prev[2];
    uint16_t reserved1;   // 0x0000 in your samples
    uint32_t crc32;         // 

    uint8_t name_len;       // 0x13 (19 bytes following)
    uint8_t name_type;      // 0x09 (Complete Local Name)
    char deviceName[18];
     uint8_t extra_len;      // 0x08
      uint8_t extra_type;     // 0xFF
      uint16_t extra_company; // 0x0059
      uint8_t extra_payload[7];
     } __attribute__ ((packed)) ;

static inline uint32_t load_le32(const void* p) {
    uint32_t v;
    memcpy(&v, p, sizeof v);
    return v;
    }
uint32_t mkseed() const {
       const uint8_t *buf=bytes+11;
       uint32_t w0 = load_le32(&buf[0]);
       uint32_t w1 = load_le32(&buf[4]);
       uint32_t w2 = load_le32(&buf[8]);
       uint32_t w3 = load_le32(&buf[12]);
       return  (w0 + w1 + w2 + w3) % 0x7FA777;
       }
uint32_t calculateCRC() const {
        return crc32_normal(bytes+11,0x10, mkseed());
        }
     bool goodcrc() const {
        return calculateCRC()==crc32;
        }
    void log(uint32_t starttime) const {
        if(!goodcrc()) {
                LOGGER("ERROR: valid=%d crc=%x should %x\n",lastglucose.valid,calculateCRC(),crc32);
                }
        if(lastglucose.valid) {
                int id=lastglucose.minfromstart;
                time_t tim=starttime+id*60;
                LOGGER("advertise %.18s\nid=%d glucose=%d %.1f mmol/L trend=%d quality=%d %s",deviceName,id,lastglucose.glucose, lastglucose.glucose/18.0f, lastglucose.trend,lastglucose.quality,ctime(&tim));
                for(auto &el: prev) {
                        --id;
                        if(el.valid&&el.glucose) {
                                time_t tim=starttime+id*60;
                                LOGGER("id=%d glucose=%d %.1f mmol/L quality=%d %s",id,el.glucose, el.glucose/18.0f, el.quality,ctime(&tim));
                                }
                        }
                }
        else {
                LOGGER("\nadvertise %.18s error\n",deviceName);
                }

        }
    } __attribute__ ((packed)) ;

