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
#ifndef L3_APP_PRIVATE_KEY_INSTRUCTION_SET_H
#define L3_APP_PRIVATE_KEY_INSTRUCTION_SET_H

#include <stddef.h>
#include <stdint.h>

#include "app_private_key_instruction_executor.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Native FUN_f3f80040 also has a live third argument in r2 at the
 * FUN_f3f7e1d0 call site. Its terminal block copies sixteen six-byte
 * results to r2 + offsets. The two-arg wrapper below preserves the old
 * behaviour by using param1 as that destination.
 */
int l3_private_key_execute_stage1_instruction_group(const l3_private_key_tables *tables,
                    const uint8_t *program, size_t program_len,
                    uint8_t *param1, size_t param1_len,
                    const uint8_t *param2, size_t param2_len,
                    uint8_t *param3, size_t param3_len);


/*
 * Native FUN_f3f80f0c is decompiled as two-arg but the ARM caller passes
 * a live third argument in r2. The generated terminal block copies sixteen
 * six-byte results to r2 + offsets, not to param1. Use this overload for
 * FUN_f3f7e1d0.
 */
int l3_private_key_execute_stage2_instruction_group(const l3_private_key_tables *tables,
                    const uint8_t *program, size_t program_len,
                    uint8_t *param1, size_t param1_len,
                    const uint8_t *param2, size_t param2_len,
                    uint8_t *param3, size_t param3_len);

#ifdef __cplusplus
}
#endif

#endif
