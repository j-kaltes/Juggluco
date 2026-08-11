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
#ifndef L3_APP_PRIVATE_KEY_INSTRUCTION_EXECUTOR_H
#define L3_APP_PRIVATE_KEY_INSTRUCTION_EXECUTOR_H

#include <stddef.h>
#include <stdint.h>
#include "app_private_key_whitebox_machine.h"
#include "app_private_key_instruction_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_DISPATCH_WORK_F80040 0x1000u
#define L3_DISPATCH_WORK_F80F0C 0x400u

int l3_private_key_execute_stage1_program(const l3_private_key_tables *tables,
                              const uint8_t *program, size_t program_len,
                              uint8_t *work, size_t work_len);

int l3_private_key_execute_stage2_program(const l3_private_key_tables *tables,
                              const uint8_t *program, size_t program_len,
                              uint8_t *work, size_t work_len);

#ifdef __cplusplus
}
#endif

#endif
