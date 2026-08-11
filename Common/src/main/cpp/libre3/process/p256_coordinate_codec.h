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
#ifndef L3_EC_COORDINATE_CODEC_H
#define L3_EC_COORDINATE_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_EC_COORDINATE_PACKED_LEN 35u
#define L3_EC_COORDINATE_LANES 10u

/* Decode the recovered 35-byte little-endian field-coordinate representation
 * into ten 28-bit arithmetic lanes. */
void l3_p256_decode_coordinate_35_to_10_lanes(
    const uint8_t packed35[L3_EC_COORDINATE_PACKED_LEN],
    uint32_t lanes10[L3_EC_COORDINATE_LANES]);

/* Inverse packing used for the recovered 35-byte private-scalar/coordinate
 * representation. */

#ifdef __cplusplus
}
#endif
#endif
