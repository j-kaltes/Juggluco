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
#ifndef L3_EPHEMERAL_KEYPAIR_STATE_H
#define L3_EPHEMERAL_KEYPAIR_STATE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Portable reconstruction of native selector-5 FUN_f3f64bc0 for one entropy
 * draw. Native reads 142 bytes, masks every byte with 7, and deterministically
 * materializes the 76-byte DB-key state. The context is the 0x1eb0 provider
 * context used by selector 5; dat429 is DAT_f4295564 (0x20000 bytes). */
int l3_p256_generate_ephemeral_keypair_state(const uint8_t *dat429, size_t dat429_len,
                                    const uint8_t context[0x1eb0],
                                    const uint8_t entropy142[142],
                                    uint8_t state76[76]);

#ifdef __cplusplus
}
#endif
#endif
