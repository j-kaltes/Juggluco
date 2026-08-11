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
#ifndef L3_DERIVED_POINT_FRAME_H
#define L3_DERIVED_POINT_FRAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_OK 0
#define L3_AUTH_ERR_ARGUMENT   -1
#define L3_AUTH_ERR_TABLE_SIZE -2
#define L3_AUTH_ERR_ALLOC      -3
#define L3_AUTH_ERR_STATE      -4
#define L3_AUTH_ERR_CALLBACK   -5

#define L3_AUTH_FRAME_LEN 0x42u

/*
 * Native FUN_f3f88a20 object shape, lifted into a normal C struct:
 *   word +0x04: type/kind, type 0 is the frame-vector kind used by processint(6)
 *   word +0x08: pointer to 0x42-byte frames
 *   word +0x0c: frame count
 *   word +0x10: byte length represented by the frame vector
 */
typedef struct {
    uint32_t type_word;
    uint32_t byte_len;
    uint32_t frame_count;
    uint8_t *frames;
} l3_authorization_frame_stream;

typedef struct {
    const uint8_t *state_transition_table;
    size_t state_transition_table_len;      /* >= 0x20000 */
    const uint8_t *digest_frame_program;  /* contiguous table used by FUN_f3f05ab8/FUN_f3f05ba4 */
    size_t digest_frame_program_len;      /* >= 0x186 */
    const uint8_t *range_extract_program;  /* contiguous table used by FUN_f3f06080/FUN_f3f06178 */
    size_t range_extract_program_len;      /* >= 0x14a */
} l3_authorization_frame_tables;

typedef struct {
    void *user;
    int (*init)(void *user);
    int (*update)(void *user, const uint8_t *bytes, size_t len);
    int (*update_frame)(void *user, const uint8_t frame66[L3_AUTH_FRAME_LEN], size_t byte_count);
    int (*final82)(void *user, uint8_t out82[0x82]);
} l3_authorization_digest_callbacks;

void l3_authorization_frame_stream_clear(l3_authorization_frame_stream *obj);
int l3_authorization_frame_stream_copy(l3_authorization_frame_stream *obj,
                           uint32_t type_word,
                           const uint8_t *frames,
                           uint32_t frame_count,
                           uint32_t byte_len);

/* Port of the type-0 branch of FUN_f3f32098 / FUN_f3f32120 selector 0x175b8a89. */
int l3_authorization_frames_concatenate(const l3_authorization_frame_stream *lhs,
                          const l3_authorization_frame_stream *rhs,
                          l3_authorization_frame_stream *out);

/* Port of FUN_f3f05cd8 selector 0x5d9235d5 for type-0 frame-vector objects. */
int l3_authorization_frames_digest(const l3_authorization_frame_tables *tables,
                        const l3_authorization_frame_stream *input,
                        const uint8_t *prefix, size_t prefix_len,
                        const uint8_t *suffix, size_t suffix_len,
                        const l3_authorization_digest_callbacks *digest,
                        l3_authorization_frame_stream *out);

/* Exposed for tests/reuse: the native digest32 -> two-frame wrapper part. */
int l3_authorization_encode_digest_result_frames(const l3_authorization_frame_tables *tables,
                                          const uint8_t final82[0x82],
                                          uint8_t out_two_frames[2 * L3_AUTH_FRAME_LEN]);

/* Port of FUN_f3f06328 selector 0x79ae1f61 for type-0 frame-vector objects. */
int l3_authorization_frames_extract_range(const l3_authorization_frame_tables *tables,
                         const l3_authorization_frame_stream *input,
                         uint32_t offset,
                         uint32_t length,
                         l3_authorization_frame_stream *out);

#ifdef __cplusplus
}
#endif

#endif
