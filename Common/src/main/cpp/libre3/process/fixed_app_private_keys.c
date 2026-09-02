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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#include "fixed_app_private_keys.h"

#include <string.h>

static const uint8_t k_native_scalar35[L3_APP_SECURITY_VERSION_COUNT][L3_APP_PRIVATE_NATIVE_SCALAR_LEN] = {
    {
        0x74,0x59,0x3a,0x60,0xeb,0xb9,0x9f,0xd8,
        0xc7,0xce,0x9d,0x98,0x72,0x15,0x6d,0xd0,
        0x8c,0x74,0x88,0x05,0xbe,0xa7,0x8d,0x94,
        0xe2,0xce,0x4d,0x94,0xab,0x7c,0x79,0xd9,
        0x00,0x00,0x00
    },
    {
        0xa6,0x46,0xf3,0x0b,0x07,0xb0,0x32,0xe5,
        0xb5,0x7c,0xd0,0x2d,0x79,0xa7,0xe5,0xa0,
        0xb7,0xad,0x2f,0xbb,0x6b,0xd3,0xd2,0xe0,
        0xe5,0x70,0x81,0xde,0x01,0xb0,0xea,0x13,
        0x00,0x00,0x00
    }
};

int l3_fixed_app_private_native_scalar35(unsigned security_version,
                                         uint8_t out35[L3_APP_PRIVATE_NATIVE_SCALAR_LEN]) {
    if (!out35 || security_version >= L3_APP_SECURITY_VERSION_COUNT) return -1;
    memcpy(out35, k_native_scalar35[security_version], L3_APP_PRIVATE_NATIVE_SCALAR_LEN);
    return 0;
}
