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
/*      Thu Oct 15 12:50:12 CEST 2026                                                */

/* SPDX-License-Identifier: GPL-3.0-or-later */
package tk.glucodata;

/** Protocol credentials for Lingo security version 3, verified against its app certificate. */
final class LingoCredentials {
    private LingoCredentials() { }
    static final byte[] APP_PRIVATE = {
        (byte)0xc3, (byte)0x2f, (byte)0x76, (byte)0x54, (byte)0x39, (byte)0x02, (byte)0x1e, (byte)0xae, (byte)0x2d, (byte)0xc8, (byte)0x83, (byte)0x3f,
        (byte)0x8f, (byte)0x58, (byte)0x17, (byte)0xa0, (byte)0x49, (byte)0x7d, (byte)0xcb, (byte)0x81, (byte)0xb5, (byte)0x9d, (byte)0x13, (byte)0xda,
        (byte)0x82, (byte)0x64, (byte)0xd4, (byte)0xb3, (byte)0x38, (byte)0x29, (byte)0xfd, (byte)0xb3,
    };

    static final byte[] APP_CERTIFICATE = {
        (byte)0x03, (byte)0x01, (byte)0x44, (byte)0x52, (byte)0x4f, (byte)0x49, (byte)0x44, (byte)0x41, (byte)0x4c, (byte)0x44, (byte)0x41, (byte)0x44,
        (byte)0x43, (byte)0x30, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x02, (byte)0x5f, (byte)0x14, (byte)0x9f, (byte)0xe1,
        (byte)0x01, (byte)0x0f, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x04, (byte)0xe3, (byte)0x63,
        (byte)0x5a, (byte)0xe3, (byte)0xc7, (byte)0xa7, (byte)0x47, (byte)0xd3, (byte)0x3d, (byte)0xed, (byte)0xd7, (byte)0x01, (byte)0x74, (byte)0x57,
        (byte)0x12, (byte)0xdb, (byte)0xf1, (byte)0xeb, (byte)0xe7, (byte)0xbb, (byte)0x56, (byte)0xb8, (byte)0x2d, (byte)0x93, (byte)0x92, (byte)0x8e,
        (byte)0xbb, (byte)0x20, (byte)0x19, (byte)0xd1, (byte)0x7d, (byte)0x16, (byte)0xcd, (byte)0x90, (byte)0x9b, (byte)0x55, (byte)0x04, (byte)0xf9,
        (byte)0x1f, (byte)0x4b, (byte)0xaa, (byte)0x2a, (byte)0x2a, (byte)0x0f, (byte)0x4d, (byte)0xb7, (byte)0xab, (byte)0x71, (byte)0x01, (byte)0xa7,
        (byte)0x58, (byte)0x8c, (byte)0xe9, (byte)0x19, (byte)0x82, (byte)0xf1, (byte)0x5c, (byte)0x17, (byte)0x3e, (byte)0x0d, (byte)0x1d, (byte)0x6c,
        (byte)0x0f, (byte)0x29, (byte)0x02, (byte)0xe1, (byte)0xa6, (byte)0x19, (byte)0xd0, (byte)0xf1, (byte)0x55, (byte)0x00, (byte)0x98, (byte)0x01,
        (byte)0xdf, (byte)0xb9, (byte)0x9c, (byte)0x82, (byte)0xb9, (byte)0xa0, (byte)0xed, (byte)0x2b, (byte)0x8e, (byte)0x75, (byte)0x53, (byte)0xac,
        (byte)0x5c, (byte)0x69, (byte)0x9a, (byte)0x22, (byte)0x6f, (byte)0xb3, (byte)0x51, (byte)0xf1, (byte)0x4e, (byte)0xa8, (byte)0x68, (byte)0x0f,
        (byte)0x51, (byte)0x1c, (byte)0xac, (byte)0x6c, (byte)0x07, (byte)0x7e, (byte)0x05, (byte)0xdc, (byte)0x93, (byte)0x33, (byte)0xf0, (byte)0x69,
        (byte)0xb3, (byte)0xac, (byte)0xb7, (byte)0x6e, (byte)0x16, (byte)0x3e, (byte)0xab, (byte)0x9f, (byte)0x0c, (byte)0x66, (byte)0x0a, (byte)0xc9,
        (byte)0x5a, (byte)0x0b, (byte)0x52, (byte)0x04, (byte)0x52, (byte)0x9d,
    };

    static final byte[] PATCH_SIGNING_PUBLIC = {
        (byte)0x04, (byte)0xe7, (byte)0x88, (byte)0x62, (byte)0x30, (byte)0xf7, (byte)0xdc, (byte)0x77, (byte)0xa2, (byte)0xc1, (byte)0x20, (byte)0x65,
        (byte)0xe1, (byte)0xa2, (byte)0x6c, (byte)0xac, (byte)0xca, (byte)0x94, (byte)0x82, (byte)0xe9, (byte)0x03, (byte)0x86, (byte)0xff, (byte)0x26,
        (byte)0x46, (byte)0x95, (byte)0x31, (byte)0x51, (byte)0x49, (byte)0x3f, (byte)0x6e, (byte)0x3c, (byte)0xf2, (byte)0xd0, (byte)0xc6, (byte)0x0c,
        (byte)0x43, (byte)0x7d, (byte)0x0e, (byte)0x48, (byte)0x80, (byte)0x79, (byte)0xb9, (byte)0xe1, (byte)0x1d, (byte)0x26, (byte)0xbe, (byte)0xd8,
        (byte)0x0e, (byte)0x7c, (byte)0x3d, (byte)0xa2, (byte)0x2e, (byte)0x66, (byte)0x35, (byte)0x0d, (byte)0x7f, (byte)0xac, (byte)0x54, (byte)0xc0,
        (byte)0x05, (byte)0x1f, (byte)0x68, (byte)0xc2, (byte)0x7d,
    };
}
