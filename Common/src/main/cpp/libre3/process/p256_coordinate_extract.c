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
#include "p256_coordinate_extract.h"

#include <string.h>

void l3_p256_extract_coordinate_35(const uint8_t *src, uint8_t out35[L3_EC_COORDINATE_PACKED_LEN]) {
    if (!src || !out35) return;
    memcpy(out35, src, L3_EC_COORDINATE_PACKED_LEN);
}

void l3_authorization_core_copy_inputs35(const uint8_t *left66,
                                  const uint8_t *right66,
                                  uint8_t left35[L3_EC_COORDINATE_PACKED_LEN],
                                  uint8_t right35[L3_EC_COORDINATE_PACKED_LEN]) {
    l3_p256_extract_coordinate_35(left66, left35);
    l3_p256_extract_coordinate_35(right66, right35);
}
