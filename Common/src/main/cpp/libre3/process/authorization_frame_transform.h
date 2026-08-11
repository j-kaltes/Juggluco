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
#ifndef L3_GENERATED_FRAME_TRANSFORM_H
#define L3_GENERATED_FRAME_TRANSFORM_H

#include <stddef.h>
#include <stdint.h>
#include "authorization_shared_point_frames.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_GENERATED_FRAME_STATE_LEN 0x10au
#define L3_AUTH_TRANSFORM_SCRATCH_LEN 0x42u

typedef struct {
    const uint8_t *dat_f4295564;
    size_t dat_f4295564_len;      /* >= 0x20000 */
    const uint8_t *dat_f40c6a52;  /* >= 0x1452 for the selectors used here */
    size_t dat_f40c6a52_len;
} l3_authorization_frame_transform_tables;

typedef struct {
    void *user;
    int (*derive_state10a)(void *user,
                           const uint8_t left_rev66[L3_AUTH_TRANSFORM_SCRATCH_LEN],
                           const uint8_t right_rev66[L3_AUTH_TRANSFORM_SCRATCH_LEN],
                           uint8_t out_state10a[L3_GENERATED_FRAME_STATE_LEN]);
} l3_auth_transform_state_callbacks;

/* Native frame count formula for FUN_f3f88a20/FUN_f3f3d3a0 bit-length objects. */
uint32_t l3_encoded_frame_count_for_bits(uint32_t bit_len);

/*
 * Port of the common downstream expander FUN_f3efe62c(state10a, bit_len, out_frames).
 * The caller must provide at least frame_count_for_bit_len(bit_len) * 0x42 bytes.
 */
int l3_authorization_expand_frame_state(const l3_authorization_frame_transform_tables *tables,
                                const uint8_t state10a[L3_GENERATED_FRAME_STATE_LEN],
                                uint32_t bit_len,
                                uint8_t *out_frames,
                                size_t out_len,
                                uint32_t *required_frames);

/*
 * Readable port of the common FUN_f3ef9940 -> PTR_FUN_f4292ad0[3] -> FUN_f3f3d3a0
 * wrapper used by processint(6).  The object-specific vtable+0x10 core plus the
 * following FUN_f3efe0f8 normalization are represented by derive_state10a().
 */

/* Exact port of FUN_f3efe0f8(raw76, normalized10a).
 * raw76 is the 0x4c-byte affine output from FUN_f3fde360.
 */
int l3_authorization_normalize_frame_state(const l3_authorization_frame_transform_tables *tables,
                             const uint8_t raw76[0x4c],
                             uint8_t out_state10a[L3_GENERATED_FRAME_STATE_LEN]);

/* Exact port of FUN_f3efdbe0(raw76, constructor10a), used when a newly
 * generated DB-key state is serialized into the common wrapper object. */


#ifdef __cplusplus
}
#endif

#endif
