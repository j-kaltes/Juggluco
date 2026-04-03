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
/*      Fri Mar 27 17:13:08 CET 2026                                                 */
#include <stdint.h>
#include <stdio.h>

#include "crc.h"

uint8_t crc8_maxim(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0x8C) : (crc >> 1);
            }
        }
    return crc;
    }

uint16_t crc16_ccitt_false(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
            }
        }
    return crc;
    }

uint32_t crc32_normal(const uint8_t *buf, int len, uint32_t crc) {
    while (len--) {
        crc ^= (uint32_t)(*buf++) << 24;

        for (int i = 0; i < 8; i++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc <<= 1;
            }
        }
    return crc;
    }

void addcrc16(uint8_t *data, size_t len) {
     uint16_t crc=crc16_ccitt_false(data,  len);
     uint8_t *crcbuf=(uint8_t*)&crc;
     data[len]=crcbuf[0];
     data[len+1]=crcbuf[1];
    };
#ifdef TEST
int main() {
 uint8_t data[]{0xdd,0x3f,0xa6,0x5f,0xed,0x53,0x0f,0x30,0x73,0x3d,0x2c,0x04,0x59,0x7b,0x02,0x78,0x64};
uint8_t crc8=crc8_maxim(data,16);
printf("calc crc8=%x  given %x\n",crc8,data[16]);
}
#endif
//#define TEST2
#ifdef TEST2
int main() {
uint8_t data[]{0x23,0x1,0x00};
uint8_t signbuf[]{0x3b,0x20};
//encrypt: 2301003B20 =>  AEEDCE60E5

uint16_t sign=crc16_ccitt_false(data, sizeof(data));
printf("crc is=%d should=%d\n",*(uint16_t*) signbuf,sign);
}

uint32_t mkseed(const uint8_t *bytes) {
       auto le32{[](const uint8_t *el) {return *(uint32_t*)el;}};
       const uint8_t *buf=bytes+11;
       uint32_t w0 = le32(&buf[0]);
       uint32_t w1 = le32(&buf[4]);
       uint32_t w2 = le32(&buf[8]);
       uint32_t w3 = le32(&buf[12]);
       return  (w0 + w1 + w2 + w3) % 0x7FA777;
       }
#include "glucose.h"
int main()  {
uint8_t data[]{0x02,0x01,0x06,0x03,0x02,0x1F,0x18,0x17,0xFF,0x59,0x00,0x60,0x54,0x01,0x00,0x1F,0x5C,0x80,0x3E,0x54,0x80,0x3A,0x4E,0x80,0x36,0x00,0x00,0x7E,0xAE,0xDD,0x01,0x13,0x09,0x41,0x69,0x44,0x45,0x58,0x20,0x58,0x2D,0x32,0x32,0x32,0x32,0x32,0x41,0x32,0x57,0x43,0x38,0x08,0xFF,0x59,0x00,0x00,0xF4,0x05,0x8A,0x36,0x00,0x00};
auto crc= crc32_normal(data+11,0x10, mkseed(data));
printf("crc32=%u\n",crc);
auto *adv= reinterpret_cast<const AdvertiseBytes*>(data);
adv->goodcrc() ;
}
#endif
