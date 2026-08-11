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
#ifndef L3_PRIVATE_SCALAR_CODEC_H
#define L3_PRIVATE_SCALAR_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_PRIVATE_SCALAR_CODEC_OK = 0,
    L3_PRIVATE_SCALAR_CODEC_ERR_ARGUMENT = -1,
    L3_PRIVATE_SCALAR_CODEC_ERR_ZERO = -2
};

/* P-256 base point used by Libre 3 ephemeral-key generation, X || Y. */
extern const uint8_t l3_ephemeral_basepoint_public_xy64[64];

/* Convert a conventional 32-byte big-endian P-256 private scalar to the
 * recovered 35-byte little-endian representation used by the generated code. */
int l3_p256_encode_private_scalar_to_native35(
    const uint8_t scalar32_be[32],
    uint8_t native35_le[35]);

/* Convert the recovered 35-byte scalar representation back to 32-byte
 * big-endian form. */
int l3_p256_decode_private_scalar_from_native35(
    const uint8_t native35_le[35],
    uint8_t scalar32_be[32]);

#ifdef __cplusplus
}
#endif
#endif
