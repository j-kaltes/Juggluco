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
#ifndef L3_APP_PRIVATE_KEY_RECORD_DECODER_H
#define L3_APP_PRIVATE_KEY_RECORD_DECODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_PRIVATE_KEY_OK 0
#define L3_PRIVATE_KEY_ERR_ARGUMENT   -1
#define L3_PRIVATE_KEY_ERR_TABLE_SIZE -2
#define L3_PRIVATE_KEY_ERR_SHORT_OUT  -3
#define L3_PRIVATE_KEY_ERR_NEED_OBJECT_METHOD -4
#define L3_PRIVATE_KEY_ERR_NOT_IMPLEMENTED -100 /* deprecated: no production path should return this */
#define L3_PRIVATE_KEY_ERR_AMBIGUOUS -101
#define L3_PRIVATE_KEY_ERR_VERIFY -102

#define L3_PRIVATE_KEY_MASKED_FRAMES_LEN 0xc6u
#define L3_PRIVATE_KEY_NORMALIZED_FRAMES_LEN  0xc6u
#define L3_PRIVATE_KEY_OBJECT_LEN  0x118u
#define L3_PRIVATE_KEY_PAYLOAD_LEN 0x10au
#define L3_PRIVATE_KEY_OBJECT_PAYLOAD_OFF 0x0cu
#define L3_PRIVATE_KEY_OBJECT_PAYLOAD_BITS 0x100u
#define L3_PRIVATE_KEY_OBJECT_OUTPUTS_LEN 0xc6u

typedef struct {
    const uint8_t *state_transition_table;
    size_t state_transition_table_len;       /* needs at least 0x20000 */
    const uint8_t *frame_normalization_program;
    size_t frame_normalization_program_len;       /* needs at least 0x84 */
    const uint8_t *payload_decode_program;
    size_t payload_decode_program_len;       /* needs at least 0x1300 */
    const uint8_t *payload_initial_state;
    size_t payload_initial_state_len;       /* needs at least 0x142 */
    const uint8_t *mask_frame_program;
    size_t mask_frame_program_len;       /* needs at least 0x200 */
} l3_private_key_record_decoder_tables;

typedef struct {
    uint8_t magic[4];
    uint8_t mode;
    uint32_t key_index;
    uint32_t bit_len;
    uint32_t selected_index;
    uint32_t byte_len;
    uint32_t block_count;
} l3_private_key_record_header;

/* Parses the 165-byte processint(2) private-key record header. */
int l3_private_key_parse_record_header(const uint8_t *key, size_t key_len, l3_private_key_record_header *out);

/* Verified: FUN_f3efb6e4 over 0x42-byte blocks: masked_frames -> normalized_frames. */
int l3_private_key_normalize_masked_frames(
    const l3_private_key_record_decoder_tables *tables,
    const uint8_t *masked_frames,
    size_t masked_frames_len,
    uint8_t *out_normalized_frames,
    size_t out_len);

/* Verified: FUN_f3efea1c(normalized_frames, bit_len) -> object payload at DAT_f4295188+0x0c. */
int l3_private_key_build_decoded_payload(
    const l3_private_key_record_decoder_tables *tables,
    const uint8_t *normalized_frames,
    size_t normalized_frames_len,
    uint32_t bit_len,
    uint8_t *out_payload,
    size_t out_len);

/* Convenience: masked_frames -> normalized_frames -> payload. Verified for captured v26 vector. */
int l3_private_key_build_payload_from_masked_frames(
    const l3_private_key_record_decoder_tables *tables,
    const uint8_t *masked_frames,
    size_t masked_frames_len,
    uint32_t bit_len,
    uint8_t *out_payload,
    size_t out_len);

/* Convenience: builds a synthetic native-like object header + payload. */


/*
 * Newly ported: FUN_f3f03d64/FUN_f3f03eb8 framing around the still-missing
 * FUN_f3f84990/FUN_f3f7e1d0 object method.
 */
int l3_private_key_build_mask_frames(
    const l3_private_key_record_decoder_tables *tables,
    const uint8_t *private_key_record,
    size_t private_key_record_len,
    const uint8_t manager_mask_b4_16[16],
    uint8_t *out_mask_frames,
    size_t out_len);

int l3_private_key_combine_object_outputs_with_mask_frames(
    const l3_private_key_record_decoder_tables *tables,
    const uint8_t *private_key_record,
    size_t private_key_record_len,
    const uint8_t manager_mask_b4_16[16],
    const uint8_t *object_outputs_0xc6,
    size_t object_outputs_len,
    uint8_t *out_masked_frames,
    size_t out_len);


typedef int (*l3_private_key_private_key_object_method_cb)(void *user,
                                                   const uint8_t input16[16],
                                                   uint8_t out42[0x42]);

/*
 * Direct production entry point for FUN_f3f03d64/FUN_f3f03eb8 framing around
 * the object method used by FUN_f3f84990/FUN_f3f7e1d0.  The callback receives
 * each 16-byte private-key block and must return the corresponding 0x42-byte
 * object-method output.  The callback can be a native-context replay bridge, a
 * real object-method port, or a strict captured-vector provider.
 */
int l3_private_key_decode_blocks_with_object_method(
    const l3_private_key_record_decoder_tables *tables,
    l3_private_key_private_key_object_method_cb cb,
    void *cb_user,
    const uint8_t *private_key_record,
    size_t private_key_record_len,
    const uint8_t *manager_a8_selected_0x424,
    size_t manager_a8_selected_len,
    const uint8_t manager_mask_b4_16[16],
    uint8_t *out_masked_frames,
    size_t out_len);

/*
 * Compatibility shim for older callers that cannot pass a callback directly.
 * Passing NULL for tables or cb clears the configured process-global context.
 */

/*
 * Historical API retained for existing harnesses.  It uses the configured
 * object-method context above and returns L3_PRIVATE_KEY_ERR_NEED_OBJECT_METHOD when
 * no context has been supplied.
 */

#ifdef __cplusplus
}
#endif

#endif
