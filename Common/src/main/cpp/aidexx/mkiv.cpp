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
// md5_evp.c
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <cstddef>
#include <stdio.h>

#include <cstdint>
#include <cstddef>
#ifndef LOGGER
#define LOGGER printf
#define LOGAR(x) printf("%s\n",x)
#endif
static void print_hex(const unsigned char *buf, unsigned int len) {
  for (unsigned int i = 0; i < len; i++) printf("%02x", buf[i]);
  putchar('\n');
}


static inline uint8_t snCharToVal(uint8_t c) {
    // digits
    if (c >= '0' && c <= '9') return uint8_t(c - '0');

    // uppercase letters: 'A'..'Z' -> 0x0A..0x23
    if (c >= 'A' && c <= 'Z') return uint8_t(c - 0x37); // 'A'(0x41)->0x0A

    // lowercase letters: 'a'..'z' -> 0x0A..0x23
    if (c >= 'a' && c <= 'z') return uint8_t(c - 0x57); // 'a'(0x61)->0x0A

    // fallback: keep raw value (matches the permissive tail behavior)
    return c;
}

static void snToBytes(const uint8_t *buf,int len,uint8_t *out) {
    len &= 0xFFFFu;
    for (int i = 0; i < len; ++i) {
        out[i] = snCharToVal(buf[i]);
    }
}


#include "Showhex.hpp"
#ifdef __ANDROID_API__
#include "cryptoextern.h"
#include "cryptodefs.h"
#else
    bool initAidexXcrypto() {return true;}
#endif
extern bool initAidexXcrypto();
static bool md5sum(const unsigned char msg[],int msglen,unsigned char out[16]) {
  static bool init=initAidexXcrypto();
  unsigned int out_len = 0;
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    LOGAR("EVP_MD_CTX_new failed");
    return false;
    }

  if(EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) {
    LOGGER("EVP_DigestInit_ex %p  failed\n",ctx);
    return false;
    }
  if(EVP_DigestUpdate(ctx, msg, msglen) != 1) {
    LOGGER("EVP_DigestUpdate(%p,%p,%d) failed\n",ctx,msg,msglen);
    return false;
    }
  if(EVP_DigestFinal_ex(ctx, out, &out_len) != 1) {
    LOGGER("EVP_DigestFinal_ex(%p, %p, %d) failed\n",ctx, out, out_len); 
    return false;
    }

  EVP_MD_CTX_free(ctx);
  return true;
}
static bool mkiv(const char *sensorname,int len,unsigned char out[16]) {
    unsigned char bytes[len];
    snToBytes((uint8_t*)sensorname,len,bytes);
    Showhex hex0(bytes,len);
    LOGGER("snToBytes-> %s\n",hex0.data);
    for(int i=0;i<len;++i)
        bytes[i] = (bytes[i] * 17 + 0x13) & 0xFF;
    Showhex hex1(bytes,len);
    LOGGER("converted %s\n",hex1.data);
    bool res=md5sum(bytes,len,out);
    Showhex hex2(out,16);
    LOGGER("md5 %s\n",hex2.data);
    return res;
    }
#include <string_view>

extern bool    mkAidexXiv(std::string_view serial,unsigned char iv[16]);

bool    mkAidexXiv(std::string_view serial,unsigned char iv[16]) {
        return mkiv(serial.data(),serial.size(),iv);
        }


bool aidexXaskKey(std::string_view serial,uint8_t uit[16]) {
    const int len= serial.size();
    unsigned char bytes[len];
    snToBytes((uint8_t *)serial.data(),serial.size(),bytes);
    Showhex hex(bytes,len);
    LOGGER("bytes %s\n",hex.data);
    unsigned char trans[len];
    std::transform(bytes,bytes+len,trans,[](const uint8_t b) -> uint8_t { return (b * 13) + 61; });
    Showhex hex2(trans,len);
    LOGGER("trans %s\n",hex2.data);
    bool res=md5sum(trans,len,uit);
    Showhex hex3(uit,16);
    LOGGER("md5 %s\n",hex3.data);
    return res;
    }
    

//#define MAIN
#ifdef MAIN
int main() {
   // char name1[]{"22222A2WC8"};
//    std::string_view serial{"22222A2WC8"};
      std::string_view serial{"22222A532X"};
//    unsigned char md5[16];
 //   mkiv(serial,md5);
    unsigned char md5b[16];
    aidexXaskKey(serial,md5b);
    } 
#endif
