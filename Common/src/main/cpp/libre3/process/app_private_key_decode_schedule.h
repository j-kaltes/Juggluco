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
#ifndef L3_APP_PRIVATE_KEY_DECODE_SCHEDULE_H
#define L3_APP_PRIVATE_KEY_DECODE_SCHEDULE_H

#include <stddef.h>
#include <stdint.h>

#include "app_private_key_instruction_set.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_OUTER_OK 0
#define L3_OUTER_ERR_ARGUMENT -1
#define L3_OUTER_ERR_STAGE -2
#define L3_OUTER_ERR_NOT_COMPLETE -100

#define L3_OUTER_ERR_NEED_OBJECT_STATE -3

/*
 * V41: executes the native local_404 round loop after stage0.
 * Requires object_state_len >= 0x58c for the current object state layout.
 * out_frame_0x400 receives a synthetic full stack-frame image with abStack_384 at offset 0.
 */
int l3_private_key_decode_stage0_round_loop(const l3_private_key_tables *tables,
                                   const uint8_t *program_f80040,
                                   size_t program_f80040_len,
                                   const uint8_t input16[16],
                                   const uint8_t *object_state,
                                   size_t object_state_len,
                                   uint8_t out_frame_0x400[0x400]);


/*
 * Full-state variant of stage0. V40 used only 0x424 bytes because that was all
 * v35/v32 captured. V42 proves the native object state is at least 0x800 bytes,
 * and later stage accesses need more than 0x424. Use this for all v42+ tests.
 */
int l3_private_key_decode_stage0_expand_full_state(const l3_private_key_tables *tables,
                                                const uint8_t *program_f80040,
                                                size_t program_f80040_len,
                                                const uint8_t input16[16],
                                                const uint8_t *object_state,
                                                size_t object_state_len,
                                                uint8_t out_stage0[0x80]);

/*
 * Executes stage0 + local_404 round loop + final f80f0c dispatcher wrapper.
 * This stops immediately before the final f3f7a7dc tail construction.
 */
int l3_private_key_decode_stage0_round_loop_and_finalize(const l3_private_key_tables *tables,
                                          const uint8_t *program_f80040,
                                          size_t program_f80040_len,
                                          const uint8_t *program_f80f0c,
                                          size_t program_f80f0c_len,
                                          const uint8_t input16[16],
                                          const uint8_t *object_state,
                                          size_t object_state_len,
                                          uint8_t out_frame_0x400[0x400]);


/* Executes only the final post-f80f0c tail:
 *
 *   post_f80_frame
 *     -> chain of FUN_f3f7a7dc calls
 *     -> out42
 */

int l3_private_key_decode_final_state(const l3_private_key_tables *tables,
                            uint8_t frame_0x400[0x400],
                            uint8_t out42[0x42]);

/*
 * Explicit form for callers that must model the seven native stack bytes
 * preserved before the 0x3173 transform.  Each byte is a base-8 lane value.
 * The split prepare/finish API is also useful for independent oracle search:
 * prepare is invariant for a fixed input/state, while finish consumes residue.
 */
int l3_private_key_decode_final_state_prepare(const l3_private_key_tables *tables,
                                   uint8_t frame_0x400[0x400]);
int l3_private_key_decode_final_state_finish_with_residue(
    const l3_private_key_tables *tables,
    uint8_t frame_0x400[0x400],
    const uint8_t residue7[7],
    uint8_t out42[0x42]);
int l3_private_key_decode_final_state_with_residue(
    const l3_private_key_tables *tables,
    uint8_t frame_0x400[0x400],
    const uint8_t residue7[7],
    uint8_t out42[0x42]);

/* Full staged implementation currently wired as:
 * stage0 -> round loop -> f80f0c -> final tail.
 */
int l3_private_key_decode_object_block(const l3_private_key_tables *tables,
                                         const uint8_t *program_f80040,
                                         size_t program_f80040_len,
                                         const uint8_t *program_f80f0c,
                                         size_t program_f80f0c_len,
                                         const uint8_t input16[16],
                                         const uint8_t *object_state,
                                         size_t object_state_len,
                                         uint8_t out42[0x42]);

typedef struct {
    const l3_private_key_tables *tables;
    const uint8_t *program_f80040;
    size_t program_f80040_len;
    const uint8_t *program_f80f0c;
    size_t program_f80f0c_len;
    uint8_t object_state[0x5a4];
    size_t object_state_len;
} l3_private_key_decode_replay_context;

int l3_private_key_decode_replay_context_init_with_state(
    l3_private_key_decode_replay_context *ctx,
    const l3_private_key_tables *tables,
    const uint8_t *program_f80040,
    size_t program_f80040_len,
    const uint8_t *program_f80f0c,
    size_t program_f80f0c_len,
    const uint8_t *object_state,
    size_t object_state_len);

int l3_private_key_decode_replay_context_init_from_manager(
    l3_private_key_decode_replay_context *ctx,
    const l3_private_key_tables *tables,
    const uint8_t *program_f80040,
    size_t program_f80040_len,
    const uint8_t *program_f80f0c,
    size_t program_f80f0c_len,
    const uint8_t *manager_a8_selected,
    size_t manager_a8_selected_len);

int l3_private_key_decode_object_method_replay_cb(void *user,
                                        const uint8_t input16[16],
                                        uint8_t out42[0x42]);

/* Native RNG helpers used by FUN_f3f7e1d0: FUN_f3f02190/FUN_f3f027b0. */
typedef struct {
    uint32_t s38;
    uint32_t s3c;
    uint32_t s40;
    uint32_t s44;
    uint32_t s48;
} l3_private_key_rng;

void l3_private_key_rng_seed(l3_private_key_rng *rng, const uint8_t *seed, size_t seed_len);
uint32_t l3_private_key_rng_next(l3_private_key_rng *rng);
int l3_private_key_rng_permutation(l3_private_key_rng *rng, uint32_t n, uint32_t *out_perm);

/*
 * Implements the first outer-schedule stage of FUN_f3f7e1d0:
 *
 *   FUN_f3f02190(input16, 0x10)
 *   FUN_f3f027b0(0x10, &local_148)
 *   16 switch cases expanding input bytes into 16 six-byte blocks at abStack_204
 *   FUN_f3f80040(abStack_204, object_state)
 *
 * The output is the caller buffer after the native f80040 terminal copy-back.
 */

/* Compatibility name for the full v44 object-method body. */

#ifdef __cplusplus
}
#endif

#endif
