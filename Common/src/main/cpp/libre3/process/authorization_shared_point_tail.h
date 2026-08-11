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
#ifndef L3_DERIVED_POINT_TAIL_TRANSFORM_H
#define L3_DERIVED_POINT_TAIL_TRANSFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_DERIVED_POINT_TAIL_TRANSFORM_RAW_LEN 0x28u
#define L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN 0x4cu

enum {
    L3_DERIVED_POINT_TAIL_TRANSFORM_OK = 0,
    L3_DERIVED_POINT_TAIL_TRANSFORM_ERR_ARGUMENT = -1
};

/*
 * Convert the 40-byte output of the generated authorization finalizer into the
 * 76-byte encoded state consumed by the generated frame normalizer.
 *
 * This is the recovered affine/tail transformation that sits between those
 * two stages.  The original binary address is intentionally not part of the API.
 */
int l3_authorization_transform_shared_point_tail(
    const uint8_t raw40[L3_DERIVED_POINT_TAIL_TRANSFORM_RAW_LEN],
    uint8_t encoded76[L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN]);

#ifdef __cplusplus
}
#endif
#endif
