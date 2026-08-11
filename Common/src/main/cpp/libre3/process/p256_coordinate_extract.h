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
#ifndef L3_EC_COORDINATE_EXTRACT_H
#define L3_EC_COORDINATE_EXTRACT_H

#include "p256_coordinate_codec.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Extract the 35-byte packed field coordinate from a 66-byte generated frame. */
void l3_p256_extract_coordinate_35(
    const uint8_t *encoded_frame,
    uint8_t packed35[L3_EC_COORDINATE_PACKED_LEN]);

/* Extract both coordinates used as the input point for the generated
 * authorization scalar-multiplication stage. */
void l3_authorization_core_copy_inputs35(
    const uint8_t *x_frame66,
    const uint8_t *y_frame66,
    uint8_t x35[L3_EC_COORDINATE_PACKED_LEN],
    uint8_t y35[L3_EC_COORDINATE_PACKED_LEN]);

#ifdef __cplusplus
}
#endif
#endif
