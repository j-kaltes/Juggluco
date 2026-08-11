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
#ifndef L3_CHALLENGE_CIPHER_CONTEXT_H
#define L3_CHALLENGE_CIPHER_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN = 0x10b0u,
    L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN = 0x42u
};

/* Static lookup material used to build and execute the generated challenge
 * block-cipher context.  `program` is the recovered f426 table region; the
 * transition table is the common 0x20000-byte white-box state table. */
typedef struct l3_challenge_block_tables {
    const uint8_t *program;
    size_t program_len;
    const uint8_t *state_transition_table;
    size_t state_transition_table_len;
} l3_challenge_block_tables;

int l3_challenge_cipher_context_encrypt(
    const l3_challenge_block_tables *tables,
    const uint8_t payload[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN],
    const uint8_t input[16],
    uint8_t output[16]);

int l3_challenge_build_context_payload(
    const l3_challenge_block_tables *tables,
    const uint8_t encoded_frame[L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN],
    uint8_t payload[L3_CHALLENGE_BLOCK_CONTEXT_PAYLOAD_LEN]);

#ifdef __cplusplus
}
#endif

#endif
