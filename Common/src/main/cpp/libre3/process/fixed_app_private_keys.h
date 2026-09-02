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

#ifndef L3_FIXED_APP_PRIVATE_KEYS_H
#define L3_FIXED_APP_PRIVATE_KEYS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_APP_SECURITY_VERSION_COUNT = 2,
    L3_APP_PRIVATE_NATIVE_SCALAR_LEN = 35
};

/*
 * The two application private-key records embedded in KEYSCrypto.java are
 * fixed.  The recovered decoder maps them deterministically to these native
 * 35-byte scalar values.  Bytes 0..31, interpreted little-endian, are the
 * actual P-256 scalar consumed by the recovered scalar-multiply routine;
 * bytes 32..34 are zero.
 */
int l3_fixed_app_private_native_scalar35(unsigned security_version,
                                         uint8_t out35[L3_APP_PRIVATE_NATIVE_SCALAR_LEN]);

#ifdef __cplusplus
}
#endif

#endif
