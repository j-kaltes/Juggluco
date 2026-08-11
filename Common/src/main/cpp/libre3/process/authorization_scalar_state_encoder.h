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
#ifndef L3_AUTHORIZATION_STATE_ENCODER_H
#define L3_AUTHORIZATION_STATE_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN  0x10cu
#define L3_AUTH_CONSTRUCTOR_STATE_WORDS      0x13u
#define L3_AUTH_CONSTRUCTOR_STATE_OUTPUT_LEN (L3_AUTH_CONSTRUCTOR_STATE_WORDS * 4u)

#define L3_AUTH_CONSTRUCTOR_STATE_OK             0
#define L3_AUTH_CONSTRUCTOR_STATE_ERR_ARGUMENT  -1
#define L3_AUTH_CONSTRUCTOR_STATE_ERR_TABLE     -2

typedef struct {
    const uint8_t *dat_f4295564;
    size_t dat_f4295564_len;
    const uint8_t *dat_f40c6a52;
    size_t dat_f40c6a52_len;
} l3_authorization_scalar_state_encoder_tables;

/* Literal port of FUN_f3efd5bc(param_1, param_2).
 * param_1 is the 0x10c-byte constructor/state material consumed by the native
 * table machine; param_2 receives 0x13 little-endian words, i.e. the 0x4c-byte
 * pre-tail state passed to FUN_f3fbab70. */
int l3_authorization_encode_scalar_state_words19(const l3_authorization_scalar_state_encoder_tables *tables,
                                  const uint8_t in10c[L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN],
                                  uint32_t out_words[L3_AUTH_CONSTRUCTOR_STATE_WORDS]);

int l3_authorization_encode_scalar_state_bytes76(const l3_authorization_scalar_state_encoder_tables *tables,
                                  const uint8_t in10c[L3_AUTH_CONSTRUCTOR_STATE_INPUT_LEN],
                                  uint8_t out76[L3_AUTH_CONSTRUCTOR_STATE_OUTPUT_LEN]);

#ifdef __cplusplus
}
#endif

#endif
