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
#ifndef L3_AUTHORIZATION_DIGEST_H
#define L3_AUTHORIZATION_DIGEST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTHORIZATION_DIGEST_OK 0
#define L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT -1
#define L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE -2

#define L3_AUTHORIZATION_DIGEST_FRAME_LEN 0x42u

typedef struct {
    const uint8_t *state_transition_table;
    size_t state_transition_table_len;
    const uint8_t *digest_update_program;
    size_t digest_update_program_len;
    const uint8_t *digest_initialization_program;
    size_t digest_initialization_program_len;
} l3_authorization_digest_tables;

typedef struct {
    l3_authorization_digest_tables tables;
    uint8_t state[0x210];
    uint32_t finalized;
} l3_authorization_digest_context;

int l3_authorization_digest_init(l3_authorization_digest_context *ctx,
                                const l3_authorization_digest_tables *tables,
                                uint32_t mode_bytes);
int l3_authorization_digest_update(l3_authorization_digest_context *ctx,
                                  const uint8_t *bytes,
                                  size_t len);
int l3_authorization_digest_update_frame(l3_authorization_digest_context *ctx,
                                        const uint8_t frame66[L3_AUTHORIZATION_DIGEST_FRAME_LEN],
                                        size_t byte_count);
int l3_authorization_digest_finalize_frame82(l3_authorization_digest_context *ctx,
                                   uint8_t out82[0x82]);

#ifdef __cplusplus
}
#endif

#endif
