/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Tue Aug 11 16:33:40 CEST 2026                                                */
#ifndef L3_CHALLENGE_WHITEBOX_BLOCK_CIPHER_H
#define L3_CHALLENGE_WHITEBOX_BLOCK_CIPHER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_CHALLENGE_BLOCK_CIPHER_OK 0
#define L3_CHALLENGE_BLOCK_CIPHER_ERR_ARGUMENT (-1)

typedef struct l3_challenge_whitebox_block_cipher {
    uint8_t frame84[0x84];
    uint32_t schedule[0x43c];
    uint32_t initialized;
} l3_challenge_whitebox_block_cipher;

int l3_challenge_whitebox_block_cipher_init(l3_challenge_whitebox_block_cipher *ctx, const uint8_t frame84[0x84]);
int l3_challenge_whitebox_encrypt_block(void *user, const uint8_t in[16], uint8_t out[16]);

#ifdef __cplusplus
}
#endif

#endif
