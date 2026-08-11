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
#ifndef L3_EC_SCALAR_MULTIPLY_H
#define L3_EC_SCALAR_MULTIPLY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_EC_FIELD_BYTES 35u

/* Static reconstruction of FUN_f3f3c248/FUN_f3f3c480.
 *
 * Values use the native 0x118-bit little-endian byte representation.  The
 * routine performs scalar multiplication of affine (x,y) on the short
 * Weierstrass curve y^2=x^3+a*x+b over modulus p.  b is not needed by the
 * native point formulas.  When use_curve_a is zero the native optimized
 * a==-3 formula is selected; otherwise curve_a35 is reduced modulo p and
 * used.
 */
int l3_p256_scalar_multiply_point(
    const uint8_t modulus35[L3_EC_FIELD_BYTES],
    const uint8_t curve_a35[L3_EC_FIELD_BYTES],
    int use_curve_a,
    const uint8_t x35[L3_EC_FIELD_BYTES],
    const uint8_t y35[L3_EC_FIELD_BYTES],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    uint8_t out_x35[L3_EC_FIELD_BYTES],
    uint8_t out_y35[L3_EC_FIELD_BYTES]);

/* FUN_f3fbc5c0 preamble through both FUN_f3fc2110 calls.  full_parent_ctx
 * supplies modulus at +0x1d50, curve a at +0x1d73, and the a-present flag at
 * +0x1d96.  Only bytes 0..0x22 of each 0x42-byte input frame are consumed. */
int l3_authorization_core_initial_lanes10(
    const uint8_t *full_parent_ctx,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    int32_t out_first_lanes10[10],
    int32_t out_second_lanes10[10]);

#ifdef __cplusplus
}
#endif

#endif
