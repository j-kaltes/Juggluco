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
#ifndef L3_AUTHORIZATION_MATERIAL_FINALIZER_H
#define L3_AUTHORIZATION_MATERIAL_FINALIZER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_FINAL_STATE_RAW40_LEN 0x28u

/* Native final write block inside FUN_f3fd4a60.  This is the last small,
   non-branchy block before FUN_f3fde360's affine tail consumes raw40.

   Ghidra variables mapped to this struct:
     a_lanes[0..3] = local_188[0], local_188[1], local_180.lo32, local_180.hi32
     b_lanes[0..3] = local_178, iStack_174, iStack_170, uStack_16c
     scalar0       = local_168
     scalar1       = iStack_164
*/
typedef struct {
    uint32_t a_lanes[4];
    uint32_t b_lanes[4];
    uint32_t scalar0;
    uint32_t scalar1;
} l3_authorization_finalizer_final_scalars;

void l3_authorization_finalizer_final_raw40(const l3_authorization_finalizer_final_scalars *in,
                              uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN]);

/* Static topology/dependency description for the two large generated routines
   that remain before this tail. */
typedef struct {
    const char *name;
    const char *native_signature;
    const char *role;
    const char *inputs;
    const char *outputs;
    const char *direct_calls;
    const char *table_families;
    unsigned decompiled_lines;
    unsigned decompiled_chars;
} l3_authorization_finalizer_core_info;


/* Native branch predicate helper FUN_f3fde21c is used twice by FUN_f3fd4a60.
   The exact predicate depends on table DAT_f41a3b98 and the UNK_f4192030/2050/2070
   constants; this helper documents its control result contract for replay/capture hooks. */
typedef enum {
    L3_AUTH_FINAL_STATE_PREDICATE_CONTINUE = 0,
    L3_AUTH_FINAL_STATE_PREDICATE_TAKE_ALTERNATE = 1
} l3_authorization_finalizer_predicate_result;

/* Exact port of FUN_f3fde21c.  Native use in FUN_f3fd4a60:
   b = FUN_f3fde21c(local_188, local_258);

   Each vector is 13 little-endian/native u32 lanes.  The function scans lanes
   12..0, looking for the first lane pair whose generated affine combination
   does not equal the sentinel address 0xf4170da9.  That non-sentinel value is
   reduced by seven rounds through DAT_f41a3b98 and compared against nibble 9.
*/
#define L3_AUTH_FINAL_STATE_PREDICATE_LANES 13u

int l3_authorization_finalizer_predicate_exact(const uint32_t left[L3_AUTH_FINAL_STATE_PREDICATE_LANES],
                       const uint32_t right[L3_AUTH_FINAL_STATE_PREDICATE_LANES]);

uint32_t l3_authorization_finalizer_predicate_reduce7(uint32_t value);

/* v97: exact port of the first two seed-pair stages inside FUN_f3fd4a60.

   These are the two 13-iteration loops immediately after function entry:
     - the param_2/aux48 loop that seeds the local_328 pair array;
     - the param_1 object-context loop that seeds the local_258 pair array.

   Each native loop writes 13 little-endian u32 pairs.  Later generated code
   treats those pairs as affine-encoded 64-bit values and fans them into the
   13-lane predicate/scalar network already documented in v95/v96.
*/
#define L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT 13u
#define L3_AUTH_FINAL_STATE_SEED_WORD_COUNT (L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT * 2u)

typedef enum {
    L3_AUTH_FOLD64_TABLE_2EB0 = 0,
    L3_AUTH_FOLD64_TABLE_2F30 = 1,

    /* v98: additional middle-network fold tables used after the seed loops.
       These are the table families referenced from the first generated
       carry-propagating loop and the subsequent param_3 loop in FUN_f3fd4a60. */
    L3_AUTH_FOLD64_TABLE_2FB0 = 2,
    L3_AUTH_FOLD64_TABLE_3030 = 3,
    L3_AUTH_FOLD64_TABLE_30B0 = 4,
    L3_AUTH_FOLD64_TABLE_3130 = 5,
    L3_AUTH_FOLD64_TABLE_31B0 = 6,
    L3_AUTH_FOLD64_TABLE_3230 = 7,
    L3_AUTH_FOLD64_TABLE_32B0 = 8,
    L3_AUTH_FOLD64_TABLE_3330 = 9,
    L3_AUTH_FOLD64_TABLE_33B0 = 10,
    L3_AUTH_FOLD64_TABLE_3430 = 11,
    L3_AUTH_FOLD64_TABLE_34B0 = 12,
    L3_AUTH_FOLD64_TABLE_3530 = 13,
    L3_AUTH_FOLD64_TABLE_35B0 = 14,
    L3_AUTH_FOLD64_TABLE_3630 = 15,
    L3_AUTH_FOLD64_TABLE_36B0 = 16,
    L3_AUTH_FOLD64_TABLE_3730 = 17,
    L3_AUTH_FOLD64_TABLE_37B0 = 18
} l3_auth_fold64_table_id;

uint64_t l3_authorization_finalizer_fold64(l3_auth_fold64_table_id table_id,
                             uint64_t value,
                             unsigned rounds);

void l3_authorization_finalizer_seed_param2_pairs(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]);

void l3_authorization_finalizer_seed_param1_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]);

/* v98: table lookup metadata for the extracted fold tables. */
typedef struct {
    l3_auth_fold64_table_id id;
    const char *native_symbol;
    uint32_t base_addr;
    const char *first_known_user;
} l3_auth_fold64_table_info;


/* v99: first post-seed generated-loop coefficient slice.

   This is the scalar head of the first large carry-propagating loop after
   the two v97 seed arrays have been initialized.  It covers the native code
   from the parent-tail/current-pair mix through the first coefficient pair
   used to update the local_328 head pair.  Later NEON window updates in the
   same loop are intentionally not claimed by this helper. */
typedef struct {
    uint32_t mul_lo;   /* native uVar65: low word of the multiplicative coefficient */
    uint32_t mul_hi;   /* native iVar78: high/affine word paired with mul_lo */
    uint32_t add_lo;   /* native uVar66: low word of the additive coefficient */
    uint32_t add_hi;   /* native iVar80: high/affine word paired with add_lo */
} l3_authorization_finalizer_first_loop_coeffs;

void l3_authorization_finalizer_first_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out);

void l3_authorization_finalizer_first_loop_update_head_pair(
    const uint32_t current_pair[2],
    const uint32_t param1_pair0[2],
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    uint32_t out_pair[2]);

/* v100: continuation of the first post-seed generated loop.

   After the v99 local_328 head-pair update, native FUN_f3fd4a60 immediately
   runs two more 64-bit affine/fold chains over the updated local_328 pair
   using DAT_f41a3030 and DAT_f41a30b0.  The result writes the paired
   uStack_320 word at the same loop offset. */
typedef struct {
    uint32_t fold3030_lo;      /* native uVar73 after the DAT_f41a3030 chain and 0x207cb425 mix */
    uint32_t fold3030_hi;      /* native iVar49 paired with fold3030_lo */
    uint32_t fold30b0_tail_lo; /* native uVar48 after the extra DAT_f41a30b0 low-word step */
    uint32_t fold30b0_tail_mix;/* native (uVar48 >> 4) + DAT_f41a30b0[(uVar48 & 0xf)].lo */
    uint32_t out_lo;           /* written to uStack_320 + loop_offset */
    uint32_t out_hi;           /* written to uStack_320 + loop_offset + 4 */
} l3_authorization_finalizer_first_loop_tail_state;

void l3_authorization_finalizer_first_loop_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_first_loop_tail_state *out);

/* v101: shared NEON/window update used repeatedly in the first post-seed
   FUN_f3fd4a60 loop.  The ARM code constructs a two-lane q register from
   affine 64-bit pair products, then executes:

       vadd.i64 q_window, q_base, q_window
       vadd.i64 q_out,    q_window, q_product

   where q_base is the same {add_lo,add_hi} 64-bit pair in both lanes and
   q_product lanes are source_pair[lane] multiplied by {mul_lo,mul_hi}. */
typedef struct {
    uint32_t lo;
    uint32_t hi;
} l3_auth_pair32;

typedef struct {
    l3_auth_pair32 lane[2];
} l3_auth_qpair64;

void l3_authorization_finalizer_vadd_i64_product_window2(
    const l3_auth_qpair64 *window_in,
    const l3_auth_qpair64 *source_pairs,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_auth_qpair64 *window_out);

/* v102: concrete instantiation of the first-loop front window writes.

   This helper wires the v99/v101 primitives to the actual native source-window
   mapping in the first post-seed loop:
     local_328  += base + local_258[0..1]  * coeff
     auStack_2f0+= base + local_258[0..1]  * coeff
     uStack_320 += base + local_258[2..5]  * coeff
     local_310  += base + local_258[6..9]  * coeff
     uStack_300 += base + local_258[10..13] * coeff
*/
typedef struct {
    l3_auth_pair32 local_328;
    l3_auth_pair32 auStack_2f0;
    l3_auth_qpair64 uStack_320;
    l3_auth_qpair64 local_310;
    l3_auth_qpair64 uStack_300;
} l3_authorization_finalizer_first_loop_front_state;

typedef struct {
    l3_auth_pair32 local258_0_1;
    l3_auth_qpair64 local258_2_5;
    l3_auth_qpair64 local258_6_9;
    l3_auth_qpair64 local258_10_13;
} l3_authorization_finalizer_first_loop_front_sources;


/* v103: stitched front+tail helper for one first-loop offset.

   This combines the concrete v102 front mapping with the v100 3030/30b0
   scalar tail.  It models the native ordering visible in FUN_f3fd4a60:

     1. update local_328 / auStack_2f0 / uStack_320 / local_310 / uStack_300;
     2. run the DAT_f41a3030/DAT_f41a30b0 scalar tail from updated local_328;
     3. overwrite the low uStack_320 pair for this loop offset with the tail
        result.

   The later auStack_2e8/auStack_2d8/auStack_2c8 continuation remains outside
   this helper. */
typedef struct {
    l3_authorization_finalizer_first_loop_front_state front;
    l3_authorization_finalizer_first_loop_tail_state tail;
} l3_authorization_finalizer_first_loop_step_state;


/* v104: late continuation of the same first post-seed loop.

   After the v103 front/tail work, the native loop also updates:
     auStack_2c8  from the live {local_1f8, local_1f4} scalar pair;
     auStack_2e8  from the local_218 vector pair;
     auStack_2d8  from the local_208 vector pair.

   These instructions are the f3fd5764..f3fd5840 window block in the ARM
   listing.  The scalar store is the same affine 64-bit add/product formula
   used for local_328 and auStack_2f0; the two vector stores are the same
   vadd.i64 product-window primitive from v101. */
typedef struct {
    l3_auth_pair32 auStack_2c8;
    l3_auth_qpair64 auStack_2e8;
    l3_auth_qpair64 auStack_2d8;
} l3_authorization_finalizer_first_loop_late_state;

typedef struct {
    l3_auth_pair32 local_1f8_1f4;
    l3_auth_qpair64 local_218;
    l3_auth_qpair64 local_208;
} l3_authorization_finalizer_first_loop_late_sources;


typedef struct {
    l3_authorization_finalizer_first_loop_front_state front;
    l3_authorization_finalizer_first_loop_late_state late;
    l3_authorization_finalizer_first_loop_tail_state tail;
} l3_authorization_finalizer_first_loop_full_step_state;


/* v105: array-level wrapper for the first post-seed FUN_f3fd4a60 loop.

   The raw stack variables in Ghidra obscure that the loop walks pair offsets
   across overlapping 64-bit windows.  This view names the native sources as the
   13 encoded pairs produced by the second seed loop and applies one full native
   iteration at offsets 0..12.

   Important source correction versus the older v102 single-offset helper:
     - local_328 scalar update uses source pair 0;
     - auStack_2f0 scalar update uses source pair 7 (local_220/iStack_21c),
       not source pair 0;
     - auStack_2c8 scalar update uses source pair 12 (local_1f8/local_1f4).
*/
#define L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS 13u
#define L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS 14u

typedef struct {
    l3_auth_pair32 pair[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
} l3_authorization_finalizer_pair_array13;

typedef struct {
    l3_auth_pair32 local_328[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 auStack_2f0[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 uStack_320[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    l3_auth_pair32 local_310[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    l3_auth_pair32 uStack_300[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    l3_auth_pair32 auStack_2e8[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    l3_auth_pair32 auStack_2d8[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    l3_auth_pair32 auStack_2c8[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
} l3_authorization_finalizer_first_loop_arrays;

typedef struct {
    l3_authorization_finalizer_first_loop_arrays arrays;
    l3_auth_pair32 final_loop_carry;  /* value carried in uVar61/iVar50 after offset 12 */
    l3_authorization_finalizer_first_loop_tail_state last_tail;
} l3_authorization_finalizer_first_loop_run13_state;

void l3_authorization_finalizer_first_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_first_loop_run13_state *state_out);

/* v106: exact port of the DAT_f41a3130 reduction that immediately follows
   the first 13-offset FUN_f3fd4a60 post-seed loop.  Native sets puVar60 to
   auStack_2c8, increments it by two words before each iteration, and emits
   local_35c[0..12].  Therefore the reduction consumes auStack_2c8 pair
   indices 1..13 from the 14-pair window produced by v105. */
typedef struct {
    uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 final_state;  /* hi=iVar55, lo=uVar61 after element 12 */
} l3_authorization_finalizer_reduce3130_state;

typedef struct {
    uint32_t input_lo[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_hi[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t fold_seed_lo[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t fold_seed_hi[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t fold7_lo[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t fold7_hi[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t partial_low[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t partial_product[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t state_hi_base[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t state_lo[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t state_hi[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
} l3_authorization_finalizer_reduce3130_trace;

void l3_authorization_finalizer_reduce_workspace_first(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce3130_state *out);



/* v107: param_3 seed loop beginning at LAB_f3fd5c50.

   Native writes 26 pairs starting at &local_328: the first 13 pairs are
   derived from param_3[0..12] using DAT_f4191850/DAT_f4191870 and the
   DAT_f41a31b0 fold table; the second 13 pairs are the repeated sentinel
   pair {0x0a452a36, 0xec81fecb}.  This helper exposes both the computed
   13-pair prefix and the full 26-pair native overwrite footprint. */
#define L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT 26u
#define L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT (L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT * 2u)

void l3_authorization_finalizer_seed_param3_pairs(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]);

void l3_authorization_finalizer_seed_param3_pairs26(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT]);

/* v108: second parent/object-context seed loop after LAB_f3fd5c50.

   After the v107 param_3 overwrite of the local_328 region, native FUN_f3fd4a60
   reseeds local_258 from the parent object context (*param_1)[0..12].  The
   formula is structurally parallel to the earlier param_1 seed loop, but uses
   a distinct affine input table pair DAT_f4191890/DAT_f41918b0 and the
   DAT_f41a3230 fold table.  It writes exactly 13 pairs. */
void l3_authorization_finalizer_seed_param1_second_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]);

/* v109: coefficient derivation at the head of the generated carry/window loop
   immediately after the v108 local_258 reseed.

   This is the native block beginning after the loads of local_328/local_258 and
   before the first local_328/uStack_320/auStack_2f0 stores.  It uses the
   DAT_f41a32b0 fold table to derive the new multiply/add coefficient pair.

   Native decompiler names:
     parent_lo,parent_hi  = uVar51/iVar47, the outer carried parent state;
     current_lo,current_hi= uVar61/iVar50, the current local_328 pair;
     out->mul_*           = uVar64/iVar80;
     out->add_*           = uVar65/iVar78.
*/
typedef struct {
    uint32_t affine_lo;
    uint32_t affine_hi;
    uint32_t fold_seed_lo;
    uint32_t fold_seed_hi;
    uint32_t fold32b0_lo;
    uint32_t fold32b0_hi;
    uint32_t mix_a_lo;
    uint32_t mix_a_hi;
    uint32_t coeff_mul_lo;
    uint32_t coeff_mul_hi;
    uint32_t coeff_add_lo;
    uint32_t coeff_add_hi;
} l3_authorization_finalizer_second_loop_coeff_trace;

void l3_authorization_finalizer_second_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out);

void l3_authorization_finalizer_second_loop_coeffs_from_head_trace(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out,
    l3_authorization_finalizer_second_loop_coeff_trace *trace);

/* v110: downstream store mapping for the second carry/window loop after the
   v109 DAT_f41a32b0 coefficient head.

   This helper wires the v109 coefficient pair to the native stores at
   fd4a60+0x944..0x998 in the decompiler slice:
     local_328  <- source pair 0
     uStack_320 <- source pairs 1,2
     auStack_2f0<- source pair 7 (local_220/iStack_21c)
     local_310  <- source pairs 3,4
     uStack_300 <- source pairs 5,6

   It intentionally stops before the DAT_f41a3330 scalar tail beginning after
   the local_328 high word is written. */
typedef struct {
    l3_auth_pair32 local258_0_1;
    l3_auth_qpair64 local258_2_5;
    l3_auth_qpair64 local258_6_9;
    l3_auth_qpair64 local258_10_13;
    l3_auth_pair32 local220_21c;
} l3_authorization_finalizer_second_loop_front_sources;

void l3_authorization_finalizer_second_loop_front_update(
    const l3_authorization_finalizer_first_loop_front_state *state_in,
    const l3_authorization_finalizer_second_loop_front_sources *sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_first_loop_front_state *state_out);


/* v111: DAT_f41a3330 / DAT_f41a33b0 scalar tail in the second
   carry/window loop.

   Native position: immediately after the v110 local_328 pair write and before
   the later auStack_2c8/2e8/2d8 stores.  The block derives a scalar from the
   updated local_328 pair, runs seven DAT_f41a3330 folds, applies an affine mix,
   then runs the DAT_f41a33b0 low-tail sequence and overwrites the low
   uStack_320 pair. */
typedef struct {
    uint32_t fold3330_lo;
    uint32_t fold3330_hi;
    uint32_t mid_lo;
    uint32_t mid_hi;
    uint32_t fold33b0_tail_lo;
    uint32_t fold33b0_tail_mix;
    uint32_t out_lo;
    uint32_t out_hi;
} l3_authorization_finalizer_second_loop_scalar_tail_state;

void l3_authorization_finalizer_second_loop_scalar_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_second_loop_scalar_tail_state *out);


/* v112: late second-loop stores after the DAT_f41a3330/DAT_f41a33b0
   scalar tail.  This mirrors the late block already ported for the first
   post-seed loop, but its native position is after the v111 tail inside the
   second carry/window loop.

   Native stores covered:
     auStack_2c8 <- local_1f8/local_1f4 scalar source pair
     auStack_2e8 <- local_218 vector source window
     auStack_2d8 <- local_208 vector source window

   The coefficient pair is the same v109 {uVar64/iVar80, uVar65/iVar78}
   live at this point in FUN_f3fd4a60. */
typedef struct {
    l3_auth_pair32 auStack_2c8;
    l3_auth_qpair64 auStack_2e8;
    l3_auth_qpair64 auStack_2d8;
} l3_authorization_finalizer_second_loop_late_state;

typedef struct {
    l3_auth_pair32 local_1f8_1f4;
    l3_auth_qpair64 local_218;
    l3_auth_qpair64 local_208;
} l3_authorization_finalizer_second_loop_late_sources;

void l3_authorization_finalizer_second_loop_late_update(
    const l3_authorization_finalizer_second_loop_late_state *state_in,
    const l3_authorization_finalizer_second_loop_late_sources *sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_second_loop_late_state *state_out);

typedef struct {
    l3_authorization_finalizer_first_loop_front_state front;
    l3_authorization_finalizer_second_loop_late_state late;
    l3_authorization_finalizer_second_loop_scalar_tail_state tail;
} l3_authorization_finalizer_second_loop_full_step_state;

void l3_authorization_finalizer_second_loop_full_step(
    const l3_authorization_finalizer_first_loop_front_state *front_in,
    const l3_authorization_finalizer_second_loop_front_sources *front_sources,
    const l3_authorization_finalizer_second_loop_late_state *late_in,
    const l3_authorization_finalizer_second_loop_late_sources *late_sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_second_loop_full_step_state *state_out);


/* v113: native-order 13-offset runner for the second FUN_f3fd4a60
   carry/window loop.  This is the loop that follows the v108 local_258 reseed
   and uses the v109 DAT_f41a32b0 coefficient head, v110 front stores, v111
   scalar tail, and v112 late stores at each offset.

   As in the first loop, the source windows are loaded once before the loop and
   reused for every offset.  The carried current pair starts as local_328[0]
   and is replaced after every iteration by the scalar-tail overwrite of the low
   uStack_320 pair. */
typedef struct {
    l3_authorization_finalizer_first_loop_arrays arrays;
    l3_auth_pair32 iteration0_physical[L3_AUTH_FINAL_STATE_SEED_FULL_PAIR_COUNT];
    l3_auth_pair32 final_loop_carry;
    l3_authorization_finalizer_second_loop_scalar_tail_state last_tail;
} l3_authorization_finalizer_second_loop_run13_state;

void l3_authorization_finalizer_second_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_second_loop_run13_state *state_out);



/* v114: DAT_f41a3430 reduction after the second 13-offset loop.

   Native position: after the v113 second carry/window loop.  The decompiler
   names the consumed pointer `param_5 + 0x1a`; in the normalized array model
   this is the second-loop auStack_2c8 window pair[1..13], matching the earlier
   v106 reduction indexing pattern.  It emits local_390[0..12]. */
typedef struct {
    uint32_t local_390[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 final_state;
} l3_authorization_finalizer_reduce3430_state;

void l3_authorization_finalizer_reduce_workspace_second(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce3430_state *out);


/* v115: seed staging immediately after the local_390 reduction.

   Native FUN_f3fd4a60 next builds two paired streams used by the following
   convolution/control block:
     - param_3[0..12] -> local_258 raw pairs plus auStack_128 prefix pairs
       through UNK_f4191910/1930 and DAT_f41a34b0;
     - local_390[0..12] -> local_b8 raw pairs plus local_188 prefix pairs
       through UNK_f4191950/1970 and DAT_f41a3530.
*/
typedef struct {
    l3_auth_pair32 local_258[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 auStack_128_prefix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 local_b8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 local_188_prefix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
} l3_authorization_finalizer_post_convolution_seed_state;

void l3_authorization_finalizer_post_convolution_seed_staging(
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_390[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_post_convolution_seed_state *out);


/* v116: convolution/range-correction block after post-local_390 seed staging.

   Native FUN_f3fd4a60 computes 26 output pairs into the local_328 region.
   It is a degree-12-by-degree-12 convolution of local_b8 and local_258,
   corrected by prefix ranges from auStack_128 and local_188.  The decompiler
   expresses this with pointers rooted at auStack_58 and auStack_2c8; this
   API exposes the normalized arrays produced by v115 instead. */
#define L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS 26u

typedef struct {
    l3_auth_pair32 local_328[L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS];
    l3_auth_pair32 product_accum[L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS];
    l3_auth_pair32 param3_range[L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS];
    l3_auth_pair32 localb8_range[L3_AUTH_FINAL_STATE_CONV_OUTPUT_PAIRS];
} l3_authorization_finalizer_post_convolution_convolution_state;

void l3_authorization_finalizer_post_convolution_convolution_range_correct(
    const l3_authorization_finalizer_post_convolution_seed_state *seed,
    l3_authorization_finalizer_post_convolution_convolution_state *out);


/* v117: parent-context reseed immediately after the v116 convolution.

   Native FUN_f3fd4a60 overwrites local_258[0..12] from (*param_1)[0..12]
   using DAT_f4191990/DAT_f41919b0 and the DAT_f41a35b0 fold table.
   This is the seed stage for the following carry/window loop whose
   coefficient head begins with DAT_f41a3630. */
void l3_authorization_finalizer_postconv_seed_param1_pairs(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_WORD_COUNT]);


/* v118: coefficient head for the carry/window loop after the v117
   parent-context reseed.

   Native position: immediately after the post-convolution local_258 reseed.
   The block derives the multiply/add coefficient pair using DAT_f41a3630
   before the loop writes local_328, uStack_320, auStack_2f0, local_310,
   uStack_300, and the later scalar tail. */
typedef struct {
    uint32_t fold3630_lo;
    uint32_t fold3630_hi;
    uint32_t mix_lo;
    uint32_t mix_hi;
    uint32_t coeff_mul_lo;
    uint32_t coeff_mul_hi;
    uint32_t coeff_add_lo;
    uint32_t coeff_add_hi;
} l3_authorization_finalizer_postconv_loop_coeff_trace;

void l3_authorization_finalizer_postconv_loop_coeffs_from_head(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out);

void l3_authorization_finalizer_postconv_loop_coeffs_from_head_trace(
    uint32_t parent_lo, uint32_t parent_hi,
    uint32_t current_lo, uint32_t current_hi,
    l3_authorization_finalizer_first_loop_coeffs *out,
    l3_authorization_finalizer_postconv_loop_coeff_trace *trace);


/* v119: scalar tail for the post-convolution carry/window loop.

   After the v118 coefficient-driven local_328 update, native runs a
   DAT_f41a36b0 fold family, an affine bridge, a DAT_f41a3730 low-tail
   family, and overwrites the low uStack_320 pair for the same loop offset. */
typedef struct {
    uint32_t fold36b0_lo;
    uint32_t fold36b0_hi;
    uint32_t mid_lo;
    uint32_t mid_hi;
    uint32_t fold3730_tail_lo;
    uint32_t fold3730_tail_mix;
    uint32_t out_lo;
    uint32_t out_hi;
} l3_authorization_finalizer_postconv_loop_scalar_tail_state;

void l3_authorization_finalizer_postconv_loop_scalar_tail_from_updated_head(
    const uint32_t updated_head_pair[2],
    const uint32_t window_pair_after_vector_add[2],
    l3_authorization_finalizer_postconv_loop_scalar_tail_state *out);


/* v120: native-order 13-offset runner for the post-convolution
   carry/window loop.  This loop follows the v117 parent-context reseed and
   uses the v118 DAT_f41a3630 coefficient head, the same front/late window
   store topology as the second loop, and the v119 DAT_f41a36b0/DAT_f41a3730
   scalar tail at each offset. */
typedef struct {
    l3_authorization_finalizer_first_loop_front_state front;
    l3_authorization_finalizer_second_loop_late_state late;
    l3_authorization_finalizer_postconv_loop_scalar_tail_state tail;
} l3_authorization_finalizer_postconv_loop_full_step_state;

void l3_authorization_finalizer_postconv_loop_full_step(
    const l3_authorization_finalizer_first_loop_front_state *front_in,
    const l3_authorization_finalizer_second_loop_front_sources *front_sources,
    const l3_authorization_finalizer_second_loop_late_state *late_in,
    const l3_authorization_finalizer_second_loop_late_sources *late_sources,
    const l3_authorization_finalizer_first_loop_coeffs *coeffs,
    l3_authorization_finalizer_postconv_loop_full_step_state *state_out);

typedef struct {
    l3_authorization_finalizer_first_loop_arrays arrays;
    l3_auth_pair32 final_loop_carry;
    l3_authorization_finalizer_postconv_loop_scalar_tail_state last_tail;
} l3_authorization_finalizer_postconv_loop_run13_state;

void l3_authorization_finalizer_postconv_loop_run13_exact_sources(
    uint32_t parent_lo,
    uint32_t parent_hi,
    const l3_authorization_finalizer_pair_array13 *sources,
    const l3_authorization_finalizer_first_loop_arrays *state_in,
    l3_authorization_finalizer_postconv_loop_run13_state *state_out);


/* v121: DAT_f41a37b0 reduction over post-convolution auStack_2c8.

   This mirrors the v106/v114 reduction shape, but consumes the auStack_2c8
   window after the v120 post-convolution loop and emits the 13-word local_3c4
   stream used by the following vector-affine finalization block. */
typedef struct {
    uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 final_state;
} l3_authorization_finalizer_reduce37b0_state;

void l3_authorization_finalizer_reduce_workspace_post_convolution(
    const l3_auth_pair32 auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS],
    l3_authorization_finalizer_reduce37b0_state *out);


/* v122: vector-affine setup immediately after local_3c4 generation.

   Native FUN_f3fd4a60 now treats the two 13-word reduction streams as
   three 128-bit chunks plus a scalar tail.  This helper ports the bounded
   setup from the DAT_f41a3830/3838/3840/3848 local_258 preload through the
   initial VectorMultiplyAccumulate operations over local_35c/local_3c4.
   Later predicate loops may overwrite local_188/local_258; this struct records
   the pre-predicate values exactly at this native boundary. */
typedef struct {
    uint32_t local_258_seed[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_188_head[4];
    uint32_t local_34c_affine[4];
    uint32_t local_33c_affine[4];
    uint32_t local_3c4_affine[4];
    uint32_t local_3b4_affine[4];
    uint32_t local_3a4_affine[4];
    uint32_t local_158;
    uint32_t local_18c;
} l3_authorization_finalizer_vector_affine_setup_state;

void l3_authorization_finalizer_vector_affine_setup(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_vector_affine_setup_state *out);



/* v123: parent-context DAT_f41a3898 reducer feeding auStack_128.

   Native position: immediately after the v122 vector-affine setup preloads the
   parent-object affine lanes.  FUN_f3fd4a60 uses parent context word 0 to seed a
   hidden driver, then walks parent words 1..12 through a DAT_f41a3898
   seven-nibble reducer.  The output is the 13-word stream stored at
   auStack_128[2..14] in the decompiler's stack-relative view. */
typedef struct {
    uint32_t first_affine;
    uint32_t first_reduced;
    uint32_t initial_driver;
    uint32_t lane_input_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_parent_context_stage01_state;

uint32_t l3_authorization_finalizer_reduce_vector_stage01(uint32_t value, unsigned rounds);

void l3_authorization_finalizer_parent_context_stage01_seed(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_parent_context_stage01_state *out);


/* v124: DAT_f41a38d8 reduction that generates local_b8[0..12].

   Native position: immediately after v123 has generated auStack_128[2..14].
   The ARM block first forms a 13-word affine local_328 backing stream from
   auStack_128 with three NEON vmla groups plus a scalar tail, then walks that
   stream through a seven-round DAT_f41a38d8 reducer to populate local_b8. */
typedef struct {
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced38d8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver38d8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_b8_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_seed_state_stage02_state;

/* Native live NEON lanes carried from the predicate constructor into the
   parity regenerators and the later 3b58/3c58 branches.  These values are
   deterministic affine expressions of the parent context plus the initial
   local_b8/local_188 arrays; they are not external entropy or hidden input. */
typedef struct {
    uint32_t add_39d8_a[4];
    uint32_t add_39d8_b[4];
    uint32_t add_39d8_c[4];
    uint32_t add_3ad8_a[4];
    uint32_t add_3ad8_b[4];
    uint32_t add_3ad8_c[4];
    uint32_t tail_accum_39d8;
    uint32_t tail_accum_3ad8;
    uint32_t late_parent_q_3b58_words[4];
    uint32_t preserved_tail12_3b58;
    uint32_t late_parent_q5_3c58_words[4];
} l3_authorization_finalizer_live_context;

void l3_authorization_finalizer_live_context_derive(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_b8_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t initial_local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_live_context *out);

uint32_t l3_authorization_finalizer_reduce_vector_stage02(uint32_t value, unsigned rounds);

void l3_authorization_finalizer_seed_state_stage02_seed(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_seed_state_stage02_state *out);


/* v125: first generated predicate-network slice after local_b8.

   Native first scans the 13-word local_1f0 vector against DAT_f41a3864 and
   DAT_f4191af0/1b10/1b30.  A complete match reaches the small raw40 finalizer;
   the first mismatch enters the generated branch network.  The first concrete
   branch ported here is the even-local_1bc[0] DAT_f41a3918 refresh path that
   rewrites local_1bc[0..12]. */
typedef struct {
    uint32_t lane_value[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t first_mismatch_lane;
    uint32_t first_mismatch_value;
    uint32_t all_match;
} l3_authorization_finalizer_predicate_terminal_gate_state;

void l3_authorization_finalizer_scan_reduction_predicate(
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_terminal_gate_state *out);

uint32_t l3_authorization_finalizer_reduce_vector_stage03(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_lane_state_a_stage03_state;

void l3_authorization_finalizer_lane_state_a_even_stage03_refresh(
    const uint32_t local_1bc_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_a_stage03_state *out);



/* v126: generated local_188 branch reducers.

   Native position: after the v125 local_1bc refresh has completed, the
   predicate network branches on local_188[0] parity.  Odd values run the
   DAT_f41a3958 reducer and rewrite local_188[0..12].  Even values run the
   DAT_f41a3998 reducer and seed auStack_128[2..14] for the following
   DAT_f41a39d8 local_188 regeneration block. */
uint32_t l3_authorization_finalizer_reduce_vector_stage04(uint32_t value, unsigned rounds);
uint32_t l3_authorization_finalizer_reduce_vector_stage05(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_a_stage04_state;

void l3_authorization_finalizer_feedback_state_a_odd_stage04_refresh(
    const uint32_t local_188_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage04_state *out);

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_a_stage05_state;

void l3_authorization_finalizer_feedback_state_a_even_stage05_refresh_reduced_words(
    const uint32_t local_188_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage05_state *out);


/* v127: DAT_f41a39d8 regeneration after the even local_188 DAT_f41a3998 path.

   Native position: after v126 has populated auStack_128[2..14].  ARM NEON
   multiplications materialize a new 13-word local_328 backing stream, including
   the scalar tail accumulator carried from the surrounding branch network, then
   a seven-round DAT_f41a39d8 loop rewrites local_188[0..12]. */
typedef struct {
    uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t tail_accum;
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced39d8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver39d8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_feedback_state_a_stage06_state;

uint32_t l3_authorization_finalizer_reduce_vector_stage06(uint32_t value, unsigned rounds);

void l3_authorization_finalizer_feedback_state_a_stage06_regenerate_from_reduced_words(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    l3_authorization_finalizer_feedback_state_a_stage06_state *out);

void l3_authorization_finalizer_feedback_state_a_stage06_regenerate_with_adds(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    l3_authorization_finalizer_feedback_state_a_stage06_state *out);


/* v128: DAT_f41a3a18 odd local_1f0 branch reducer.

   Native position: after the local_188 refresh/regeneration logic returns to
   the local_1bc predicate check, an odd local_1f0[0] enters the DAT_f41a3a18
   reducer and rewrites local_1f0[0..12].  The following local_258 parity
   branches are intentionally left as the next bounded slice. */
uint32_t l3_authorization_finalizer_reduce_vector_stage07(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_lane_state_b_stage07_state;

void l3_authorization_finalizer_lane_state_b_odd_stage07_refresh(
    const uint32_t local_1f0_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_b_stage07_state *out);


/* v129: local_258 parity branch reducers.

   Native position: immediately after the v128 DAT_f41a3a18 local_1f0 rewrite,
   the generated predicate network splits on local_258[0] parity.  Odd
   local_258[0] enters UNK_f41a3a58 and rewrites local_258[0..12]; even
   local_258[0] enters UNK_f41a3a98 and writes auStack_128[2..14] for the
   following DAT_f41a3ad8 regeneration block. */
uint32_t l3_authorization_finalizer_reduce_vector_stage08(uint32_t value, unsigned rounds);
uint32_t l3_authorization_finalizer_reduce_vector_stage09(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_b_stage08_state;

typedef struct {
    uint32_t input_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_affine[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t write_driver[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t initial_driver;
    uint32_t final_driver;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_b_stage09_state;

void l3_authorization_finalizer_feedback_state_b_odd_stage08_refresh(
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage08_state *out);

void l3_authorization_finalizer_feedback_state_b_even_stage09_refresh_reduced_words(
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage09_state *out);


/* v130: DAT_f41a3ad8 regeneration after the even local_258 UNK_f41a3a98 path.

   Native position: after v129 has populated auStack_128[2..14].  The native
   NEON block materializes a new local_328 backing stream with two four-lane
   multiplier vectors and one scalar tail, then a seven-round DAT_f41a3ad8 loop
   rewrites local_258[0..12]. */
typedef struct {
    uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t tail_accum;
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3ad8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3ad8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_feedback_state_b_stage10_state;

uint32_t l3_authorization_finalizer_reduce_vector_stage10(uint32_t value, unsigned rounds);

void l3_authorization_finalizer_feedback_state_b_stage10_regenerate_from_reduced_words(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    l3_authorization_finalizer_feedback_state_b_stage10_state *out);

void l3_authorization_finalizer_feedback_state_b_stage10_regenerate_with_adds(
    const uint32_t auStack_128_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t tail_accum,
    const uint32_t add_a[4],
    const uint32_t add_b[4],
    const uint32_t add_c[4],
    l3_authorization_finalizer_feedback_state_b_stage10_state *out);


/* v131: local_1f0-even DAT_f41a3b18 predicate scan and DAT_f41a3b58
   local_1bc rewrite.

   Native position: after LAB_f3fd99e4, an even local_1f0[0] enters a
   backwards scan over local_1bc/local_1f0 using DAT_f41a3b18.  If the
   selected lane does not send control to the later DAT_f41a3c58/final path,
   a vector-affine backing stream is materialized and DAT_f41a3b58 rewrites
   local_1bc[0..12]. */
uint32_t l3_authorization_finalizer_reduce_vector_stage11(uint32_t value, unsigned rounds);
uint32_t l3_authorization_finalizer_reduce_vector_stage12(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_mix[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t lane_reduced[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t selected_index;
    uint32_t selected_mix;
    uint32_t selected_reduced;
    uint32_t visited_mask;
    uint32_t sentinel_mask;
    uint32_t branch_applicable;
    uint32_t goes_to_3b58_rewrite;
    uint32_t goes_to_c58_final_path;
} l3_authorization_finalizer_predicate_stage11_state;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3b58[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3b58[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
    uint32_t branch_applicable;
    l3_authorization_finalizer_predicate_stage11_state scan;
} l3_authorization_finalizer_lane_state_a_stage12_state;

void l3_authorization_finalizer_predicate_stage11_scan(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_stage11_state *out);

void l3_authorization_finalizer_lane_state_a_stage12_rewrite(
    const uint32_t local_1bc_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_a_stage12_state *out);


/* v132: alternate DAT_f41a3b18 branch into LAB_f3fda514 / DAT_f41a3c58.

   When the v131 backwards scan selects the final-path condition, native code
   materializes a different vector-affine local_328 stream from local_1bc and
   local_1f0, then rewrites local_1f0[0..12] through the DAT_f41a3c58 seven-
   round reducer.  The later FUN_f3fde21c-dependent final branches are kept as
   separate follow-on increments. */
uint32_t l3_authorization_finalizer_reduce_vector_stage15(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3c58[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3c58[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
    uint32_t branch_applicable;
    l3_authorization_finalizer_predicate_stage11_state scan;
} l3_authorization_finalizer_lane_state_b_stage15_state;

void l3_authorization_finalizer_lane_state_b_stage15_rewrite(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_lane_state_b_stage15_state *out);



/* v133: post-DAT_f41a3c58 FUN_f3fde21c split.

   This increment ports the two local_258 rewrite loops immediately after the
   alternate DAT_f41a3c58 local_1f0 branch.  The first loop is guarded by the
   exact FUN_f3fde21c(local_188, local_258) predicate and uses DAT_f41a3c98;
   the second loop is the common DAT_f41a3cd8 rewrite that follows regardless
   of the predicate result.  Both helpers take the already-materialized native
   local_328 backing stream explicitly, so the next increment can attach the
   surrounding parent-context vector-affine setup without changing the reducer
   semantics. */
uint32_t l3_authorization_finalizer_reduce_vector_stage16(uint32_t value, unsigned rounds);
uint32_t l3_authorization_finalizer_reduce_vector_stage17(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3c98[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3c98[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
    uint32_t fde21c_result;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_b_stage16_state;

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3cd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3cd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_feedback_state_b_stage17_state;

void l3_authorization_finalizer_feedback_state_b_stage16_optional_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage16_state *out);

void l3_authorization_finalizer_feedback_state_b_stage17_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_in[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_b_stage17_state *out);


/* v134: materialized native local_328 streams for the post-DAT_f41a3c58
   local_258 rewrite pair.  The DAT_f41a3c98 setup uses a callee-saved
   q5-derived late-parent vector for lanes 7..10; this is passed explicitly
   until the upstream q5 producer is folded into the generated core. */
void l3_authorization_finalizer_make_reduction_state_stage16_from_parent_feedback_b_tail(
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]);

void l3_authorization_finalizer_make_reduction_state_stage17_from_188_258(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]);

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q5_words[4];
    uint32_t local328_3c98[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_3cd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_authorization_finalizer_feedback_state_b_stage16_state after_3c98;
    l3_authorization_finalizer_feedback_state_b_stage17_state after_3cd8;
} l3_authorization_finalizer_feedback_state_b_stage16_stage17_affine_state;

void l3_authorization_finalizer_feedback_state_b_stage16_stage17_rewrite_materialized(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    l3_authorization_finalizer_feedback_state_b_stage16_stage17_affine_state *out);

/* v135: terminal predicate/output bridge.

   Native position: the all-match exit from LAB_f3fd8e70.  If every
   DAT_f41a3864 affine gate lane equals 0x72e328f5, FUN_f3fd4a60 stops the
   predicate loop and writes the final 0x28-byte raw40 block from local_188.
   This wraps the existing v95 final_raw40 formula with the real v125 gate. */
typedef struct {
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_authorization_finalizer_predicate_terminal_gate_state gate;
    l3_authorization_finalizer_final_scalars scalars;
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
    uint32_t terminal_taken;
} l3_authorization_finalizer_terminal_gate_raw40_state;

void l3_authorization_finalizer_terminal_gate_raw40(
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_terminal_gate_raw40_state *out);



/* v136: post-DAT_f41a3b58 FUN_f3fde21c split.

   This ports the pair of local_188 rewrite loops after the v131 DAT_f41a3b58
   local_1bc rewrite path.  If FUN_f3fde21c(local_188, local_258) is true,
   native first rewrites local_188 with DAT_f41a3bd8; it then always rewrites
   local_188 through DAT_f41a3c18.  The helpers take the already-materialized
   native local_328 backing streams explicitly, matching the v133 staging for
   local_258. */
uint32_t l3_authorization_finalizer_reduce_vector_stage13(uint32_t value, unsigned rounds);
uint32_t l3_authorization_finalizer_reduce_vector_stage14(uint32_t value, unsigned rounds);

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3bd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3bd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
    uint32_t fde21c_result;
    uint32_t branch_applicable;
} l3_authorization_finalizer_feedback_state_a_stage13_state;

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t reduced3c18[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t driver3c18[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t final_driver;
} l3_authorization_finalizer_feedback_state_a_stage14_state;

void l3_authorization_finalizer_feedback_state_a_stage13_optional_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage13_state *out);

void l3_authorization_finalizer_feedback_state_a_stage14_rewrite_from_stream(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_feedback_state_a_stage14_state *out);

/* v137: materialized native local_328 streams for the post-DAT_f41a3b58
   local_188 rewrite pair.  The DAT_f41a3bd8 setup overwrites lanes 0..11
   and leaves the tail lane inherited from the preceding local_328 buffer;
   the live q-vector contribution for lanes 7..10 is still passed explicitly. */
void l3_authorization_finalizer_make_reduction_state_stage13_from_parent_feedback_a_tail(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]);

void l3_authorization_finalizer_make_reduction_state_stage14_from_188_258(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t local_328_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS]);

typedef struct {
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q_words[4];
    uint32_t preserved_tail12;
    uint32_t local328_3bd8[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_3c18[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_authorization_finalizer_feedback_state_a_stage13_state after_3bd8;
    l3_authorization_finalizer_feedback_state_a_stage14_state after_3c18;
} l3_authorization_finalizer_feedback_state_a_stage13_stage14_affine_state;

void l3_authorization_finalizer_feedback_state_a_stage13_stage14_rewrite_materialized(
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    l3_authorization_finalizer_feedback_state_a_stage13_stage14_affine_state *out);

/* v138: composed materialized predicate-loop step for the v131 3b58 path.

   This helper wires the verified v131 DAT_f41a3b18/DAT_f41a3b58 local_1bc
   branch to the v137 materialized DAT_f41a3bd8/DAT_f41a3c18 local_188 branch.
   It intentionally carries the two still-external native carry-ins from v137
   instead of guessing their producer. */
typedef enum {
    L3_AUTH_FINAL_STATE_STEP_3B58_NOT_APPLICABLE = 0,
    L3_AUTH_FINAL_STATE_STEP_3B58_TAKEN = 1,
    L3_AUTH_FINAL_STATE_STEP_3C58_ALTERNATE = 2
} l3_authorization_finalizer_predicate_loop_stage12_path;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q_words[4];
    uint32_t preserved_tail12;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t path;
    uint32_t branch_completed;
    l3_authorization_finalizer_lane_state_a_stage12_state after_3b58;
    l3_authorization_finalizer_feedback_state_a_stage13_stage14_affine_state after_3bd8_3c18;
} l3_authorization_finalizer_predicate_loop_stage12_step_state;

void l3_authorization_finalizer_predicate_loop_stage12_step_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_words[4],
    uint32_t preserved_tail12,
    l3_authorization_finalizer_predicate_loop_stage12_step_state *out);


/* v139: composed materialized predicate-loop step for the sibling v132 3c58 path.

   This helper wires the verified v132 DAT_f41a3c58 local_1f0 alternate branch
   to the v134/v133 materialized DAT_f41a3c98/DAT_f41a3cd8 local_258 rewrite
   pair.  The still-external q5-derived carry-in from v134 remains explicit. */
typedef enum {
    L3_AUTH_FINAL_STATE_STEP_3C58_NOT_APPLICABLE = 0,
    L3_AUTH_FINAL_STATE_STEP_3C58_TAKEN = 1,
    L3_AUTH_FINAL_STATE_STEP_3B58_SIBLING = 2
} l3_authorization_finalizer_predicate_loop_stage15_path;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q5_words[4];
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t path;
    uint32_t branch_completed;
    l3_authorization_finalizer_lane_state_b_stage15_state after_3c58;
    l3_authorization_finalizer_feedback_state_b_stage16_stage17_affine_state after_3c98_3cd8;
} l3_authorization_finalizer_predicate_loop_stage15_step_state;

void l3_authorization_finalizer_predicate_loop_stage15_step_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q5_words[4],
    l3_authorization_finalizer_predicate_loop_stage15_step_state *out);


/* v140: one-cycle materialized predicate-network state machine.

   This composes the terminal v135 gate/raw40 bridge with the two verified
   sibling predicate-loop steps from v138 and v139.  The remaining native
   live-register/carry inputs stay explicit: the 3b58 side needs the v137
   late q-vector and preserved local_328[12] tail, while the 3c58 side needs
   the v134 q5-derived carry vector. */
typedef enum {
    L3_AUTH_FINAL_STATE_CYCLE_NO_PROGRESS = 0,
    L3_AUTH_FINAL_STATE_CYCLE_TERMINAL_RAW40 = 1,
    L3_AUTH_FINAL_STATE_CYCLE_3B58_STEP = 2,
    L3_AUTH_FINAL_STATE_CYCLE_3C58_STEP = 3
} l3_authorization_finalizer_predicate_cycle_path;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q_3b58_words[4];
    uint32_t late_parent_q5_3c58_words[4];
    uint32_t preserved_tail12_3b58;

    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];

    uint32_t path;
    uint32_t terminal_taken;
    uint32_t branch_completed;
    l3_authorization_finalizer_terminal_gate_raw40_state terminal;
    l3_authorization_finalizer_predicate_loop_stage12_step_state step_3b58;
    l3_authorization_finalizer_predicate_loop_stage15_step_state step_3c58;
} l3_authorization_finalizer_predicate_cycle_state;

void l3_authorization_finalizer_predicate_cycle_materialized(
    const uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t late_parent_q_3b58_words[4],
    uint32_t preserved_tail12_3b58,
    const uint32_t late_parent_q5_3c58_words[4],
    l3_authorization_finalizer_predicate_cycle_state *out);



/* v141: bounded materialized predicate-network loop driver.

   Iterates the v140 one-cycle state machine until terminal raw40, no-progress,
   or a caller supplied cycle limit.  This is still materialized: the native
   live-register/carry inputs exposed by v134/v137 remain explicit. */
#define L3_AUTH_FINAL_STATE_PREDICATE_LOOP_MAX_TRACE 32u

typedef enum {
    L3_AUTH_FINAL_STATE_LOOP_CYCLE_LIMIT = 0,
    L3_AUTH_FINAL_STATE_LOOP_TERMINAL_RAW40 = 1,
    L3_AUTH_FINAL_STATE_LOOP_NO_PROGRESS = 2
} l3_authorization_finalizer_predicate_loop_stop;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t late_parent_q_3b58_words[4];
    uint32_t late_parent_q5_3c58_words[4];
    uint32_t preserved_tail12_3b58;
    uint32_t max_cycles;

    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];

    uint32_t stop_reason;
    uint32_t cycles_executed;
    uint32_t terminal_taken;
    uint32_t trace_len;
    uint32_t path_trace[L3_AUTH_FINAL_STATE_PREDICATE_LOOP_MAX_TRACE];
    l3_authorization_finalizer_predicate_cycle_state last_cycle;

    /* v155: retain the last concrete producer for each sibling branch, not
       only the final cycle.  This lets higher-level adapters extract the
       streams needed by the v154 producer-provider bridge from an actual
       bounded-loop run state. */
    uint32_t seen_3b58_cycle;
    uint32_t seen_3c58_cycle;
    l3_authorization_finalizer_predicate_cycle_state last_3b58_cycle;
    l3_authorization_finalizer_predicate_cycle_state last_3c58_cycle;
} l3_authorization_finalizer_predicate_loop_run_state;



/* v142: upstream predicate preloop and bounded driver bridge.

   Native reaches the v141 3b18/3c58 predicate loop only after two preceding
   parity-refresh loops have stabilized: local_1bc/local_188, then
   local_1f0/local_258.  This materialized bridge composes the already verified
   v125-v130 reducers, keeps the two regeneration scalar tails explicit, and
   optionally continues into the v141 bounded predicate-loop driver. */
#define L3_AUTH_FINAL_STATE_PREDICATE_PRELOOP_MAX_TRACE 32u

typedef enum {
    L3_AUTH_FINAL_STATE_PRELOOP_READY_FOR_DRIVER = 0,
    L3_AUTH_FINAL_STATE_PRELOOP_TERMINAL_RAW40 = 1,
    L3_AUTH_FINAL_STATE_PRELOOP_CYCLE_LIMIT = 2
} l3_authorization_finalizer_predicate_preloop_stop;

typedef enum {
    L3_AUTH_FINAL_STATE_PRELOOP_TRACE_1BC188 = 1,
    L3_AUTH_FINAL_STATE_PRELOOP_TRACE_1F0258 = 2,
    L3_AUTH_FINAL_STATE_PRELOOP_TRACE_TERMINAL = 3
} l3_authorization_finalizer_predicate_preloop_trace;

typedef struct {
    uint32_t input_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t tail_accum_39d8;
    uint32_t tail_accum_3ad8;
    uint32_t max_1bc188_cycles;
    uint32_t max_1f0258_cycles;

    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];

    uint32_t stop_reason;
    uint32_t terminal_taken;
    uint32_t cycles_1bc188;
    uint32_t cycles_1f0258;
    uint32_t trace_len;
    uint32_t path_trace[L3_AUTH_FINAL_STATE_PREDICATE_PRELOOP_MAX_TRACE];

    l3_authorization_finalizer_terminal_gate_raw40_state terminal;
    l3_authorization_finalizer_lane_state_a_stage03_state last_3918;
    l3_authorization_finalizer_feedback_state_a_stage04_state last_3958;
    l3_authorization_finalizer_feedback_state_a_stage05_state last_3998;
    l3_authorization_finalizer_feedback_state_a_stage06_state last_39d8;
    l3_authorization_finalizer_lane_state_b_stage07_state last_3a18;
    l3_authorization_finalizer_feedback_state_b_stage08_state last_3a58;
    l3_authorization_finalizer_feedback_state_b_stage09_state last_3a98;
    l3_authorization_finalizer_feedback_state_b_stage10_state last_3ad8;
} l3_authorization_finalizer_predicate_preloop_state;



typedef struct {
    l3_authorization_finalizer_predicate_preloop_state preloop;
    l3_authorization_finalizer_predicate_loop_run_state loop;
    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_preloop_state;




/* v143: constructor for the native predicate-network initial state.

   This stitches together the already verified post-v122/v123/v124 boundary:
   vector-affine setup over local_35c/local_3c4, parent-context auStack_128
   seeding, and local_b8 generation.  It then materializes the four live
   13-word predicate arrays consumed by the v142 preloop bridge. */
typedef struct {
    uint32_t input_local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t input_parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];

    l3_authorization_finalizer_vector_affine_setup_state vector_setup;
    l3_authorization_finalizer_parent_context_stage01_state parent3898;
    l3_authorization_finalizer_seed_state_stage02_state local_b8_38d8;

    uint32_t local_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local_b8_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
} l3_authorization_finalizer_predicate_initial_state;

void l3_authorization_finalizer_predicate_initial_state_construct(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_predicate_initial_state *out);

typedef struct {
    l3_authorization_finalizer_predicate_initial_state initial;
    l3_authorization_finalizer_predicate_run_from_preloop_state run;
    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_constructor_state;


/* Complete constructor-to-terminal path with every former carry/q input
   derived from the native live-lane expressions. */
void l3_authorization_finalizer_predicate_run_from_constructor_derived(
    const uint32_t local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t local_3c4[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    uint32_t max_predicate_cycles,
    l3_authorization_finalizer_live_context *live_out,
    l3_authorization_finalizer_predicate_run_from_constructor_state *out);

typedef struct {
    l3_authorization_finalizer_live_context live;
    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t predicate_cycles;
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_final_result;

/* Exact standalone FUN_f3fd4a60 entry.  param2 is aux48 padded to 13 words;
   param3 is vec_b13; parent_pair is param1[0x1c7]+{8,12}. */
int l3_authorization_finalize_round_result(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    uint32_t parent_pair_lo,
    uint32_t parent_pair_hi,
    uint32_t max_1bc188_cycles,
    uint32_t max_1f0258_cycles,
    uint32_t max_predicate_cycles,
    l3_authorization_final_result *out);


/* v144: bridge from the post-convolution loop/reduction into the predicate
   constructor/runner.

   This wraps the v121 DAT_f41a37b0 reduction over the post-convolution
   auStack_2c8 window and immediately feeds the resulting local_3c4 stream to
   the v143 predicate constructor.  It intentionally keeps the remaining native
   live-register/carry inputs explicit. */
typedef struct {
    uint32_t input_local_35c[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 input_auStack_2c8_window[L3_AUTH_FINAL_STATE_FIRST_LOOP_WINDOW_PAIRS];
    uint32_t input_parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    l3_auth_pair32 postconv_final_loop_carry;
    l3_authorization_finalizer_postconv_loop_scalar_tail_state postconv_last_tail;

    l3_authorization_finalizer_reduce37b0_state reduce37b0;
    l3_authorization_finalizer_predicate_run_from_constructor_state run;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_postconv_state;




/* v145: bridge from the post-local_390 convolution footprint into the
   post-convolution loop and predicate runner.

   Native v116 writes 26 contiguous 64-bit pairs beginning at &local_328.
   Subsequent generated loops address overlapping stack windows rooted at
   local_328, uStack_320, local_310, uStack_300, auStack_2f0,
   auStack_2e8, auStack_2d8, and auStack_2c8.  This helper materializes that
   overlap explicitly, then runs the v117 parent reseed, v120 post-convolution
   loop, and v144 predicate bridge.  Remaining native live-register/carry
   values stay explicit. */
typedef struct {
    l3_authorization_finalizer_first_loop_arrays arrays;
} l3_authorization_finalizer_post_convolution_convolution_windows_state;

void l3_authorization_finalizer_post_convolution_convolution_to_windows(
    const l3_authorization_finalizer_post_convolution_convolution_state *conv,
    l3_authorization_finalizer_post_convolution_convolution_windows_state *out);

typedef struct {
    l3_authorization_finalizer_post_convolution_convolution_state convolution;
    l3_authorization_finalizer_post_convolution_convolution_windows_state windows;
    l3_authorization_finalizer_pair_array13 postconv_sources;
    l3_authorization_finalizer_postconv_loop_run13_state postconv_loop;
    l3_authorization_finalizer_predicate_run_from_postconv_state bridge;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_post_convolution_state;




/* v146: second-loop reduction -> post_convolution bridge.

   This connects the v113/v114 second carry/window loop output to the v145
   post-local_390 path.  It runs the DAT_f41a3430 reduction internally to
   materialize local_390, performs the v115 post_convolution seed staging, and then
   invokes the v145 post_convolution convolution/postconv/predicate bridge. */
typedef struct {
    l3_authorization_finalizer_second_loop_run13_state second_loop;
    l3_authorization_finalizer_reduce3430_state reduce3430;
    l3_authorization_finalizer_post_convolution_seed_state seed;
    l3_authorization_finalizer_predicate_run_from_post_convolution_state bridge;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_second_loop_state;




/* v147: first-loop reduction -> second-loop seed/source construction -> v146 bridge.

   This moves the integration boundary one generated loop earlier.  It runs the
   DAT_f41a3130 reduction over the first-loop auStack_2c8 window to materialize
   local_35c, maps the v107 26-pair param_3 seed footprint into the overlapping
   second-loop stack windows, derives the v108 second-loop local_258 source
   pairs from parent_words, and then invokes the v146 second-loop bridge. */
void l3_authorization_finalizer_param3_seed26_to_second_loop_windows(
    const uint32_t seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT],
    l3_authorization_finalizer_first_loop_arrays *out);

void l3_authorization_finalizer_second_loop_sources_from_parent(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_pair_array13 *out);

typedef struct {
    l3_authorization_finalizer_first_loop_run13_state first_loop;
    l3_authorization_finalizer_reduce3130_state reduce3130;
    uint32_t param3_seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT];
    l3_authorization_finalizer_first_loop_arrays second_loop_initial;
    l3_authorization_finalizer_pair_array13 second_loop_sources;
    l3_authorization_finalizer_predicate_run_from_second_loop_state bridge;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_first_loop_state;



/* v148: initial FUN_f3fd4a60 seed loops -> first-loop bridge.

   This moves the materialized integration boundary to the top seed loops.  It
   derives the first-loop stack footprint from param_2, derives the first-loop
   source pairs from the parent/object-context seed loop, and then invokes the
   v147 first-loop/full-predicate bridge. */
void l3_authorization_finalizer_seed_param2_pairs26(
    const uint32_t param2_words[L3_AUTH_FINAL_STATE_SEED_PAIR_COUNT],
    uint32_t out_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT]);

void l3_authorization_finalizer_param2_seed26_to_first_loop_windows(
    const uint32_t seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT],
    l3_authorization_finalizer_first_loop_arrays *out);

void l3_authorization_finalizer_first_loop_sources_from_parent(
    const uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS],
    l3_authorization_finalizer_pair_array13 *out);

typedef struct {
    uint32_t param2_seed26_words[L3_AUTH_FINAL_STATE_SEED_FULL_WORD_COUNT];
    l3_authorization_finalizer_first_loop_arrays first_loop_initial;
    l3_authorization_finalizer_pair_array13 first_loop_sources;
    l3_authorization_finalizer_predicate_run_from_first_loop_state bridge;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_predicate_run_from_initial_seed_state;


/* v149: materialized FUN_f3fd4a60 core entry wrapper.

   This is not a new native transform.  It packages the v148 entry boundary into
   a single explicit input/state object so downstream integration no longer has
   to thread twenty scalar/q-vector arguments by hand.  Values whose provenance
   is still not proven remain explicitly represented in the input struct. */
typedef struct {
    uint32_t param2_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t parent_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];

    uint32_t first_loop_parent_lo;
    uint32_t first_loop_parent_hi;
    uint32_t second_loop_parent_lo;
    uint32_t second_loop_parent_hi;
    uint32_t postconv_loop_parent_lo;
    uint32_t postconv_loop_parent_hi;

    uint32_t tail_accum_39d8;
    uint32_t tail_accum_3ad8;

    uint32_t max_1bc188_cycles;
    uint32_t max_1f0258_cycles;
    uint32_t max_predicate_cycles;

    uint32_t late_parent_q_3b58_words[4];
    uint32_t preserved_tail12_3b58;
    uint32_t late_parent_q5_3c58_words[4];
} l3_authorization_finalizer_core_materialized_inputs;

typedef struct {
    l3_authorization_finalizer_core_materialized_inputs inputs;
    l3_authorization_finalizer_predicate_run_from_initial_seed_state generated_core;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_core_materialized_state;



/* v150: native caller/context adapter for FUN_f3fd4a60.

   The decompiled function loads a single parent-context pair once at entry:

       uVar51 = *(uint *)(param_1[0x1c7] + 8);
       iVar47 = *(int  *)(param_1[0x1c7] + 0xc);

   That live pair feeds the first, second, and post-convolution generated loop
   coefficient heads before those temporaries are repurposed by the later
   DAT_f41a37b0 reduction.  This adapter collapses the three formerly explicit
   parent pairs into that one proven native-context pair. */
typedef struct {
    uint32_t param2_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t param3_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t param1_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];

    uint32_t param1_1c7_word8;
    uint32_t param1_1c7_word12;

    uint32_t tail_accum_39d8;
    uint32_t tail_accum_3ad8;

    uint32_t max_1bc188_cycles;
    uint32_t max_1f0258_cycles;
    uint32_t max_predicate_cycles;

    uint32_t late_parent_q_3b58_words[4];
    uint32_t preserved_tail12_3b58;
    uint32_t late_parent_q5_3c58_words[4];
} l3_authorization_finalizer_native_context_inputs;

typedef struct {
    l3_authorization_finalizer_native_context_inputs native_inputs;
    l3_authorization_finalizer_core_materialized_inputs materialized_inputs;
    l3_authorization_finalizer_core_materialized_state core;

    uint32_t parent_pair_lo;
    uint32_t parent_pair_hi;
    uint32_t collapsed_parent_pair_xor;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_context_state;




/* v151: resolver for native scalar/q-vector carry inputs.

   These helpers do not guess the caller's hidden register provenance.  They
   invert the exact affine tail slots already ported for v127/v130/v137/v134
   when the corresponding native local_328 backing stream is available.  This
   converts the remaining opaque boundary from five unrelated explicit inputs
   into values that can be recovered from captured or later-proven native
   backing streams. */
typedef struct {
    uint32_t tail_accum_39d8;
    uint32_t tail_accum_3ad8;
    uint32_t late_parent_q_3b58_words[4];
    uint32_t preserved_tail12_3b58;
    uint32_t late_parent_q5_3c58_words[4];

    uint32_t local328_tail_39d8;
    uint32_t local328_tail_3ad8;
    uint32_t recovered_q_3b58_xor;
    uint32_t recovered_q5_3c58_xor;
} l3_authorization_finalizer_native_carry_resolved_state;








/* v152: native context run with resolved local_328 stream carries.

   This adapter attaches the v151 carry resolver directly to the v150 native
   context run.  The caller supplies the native local_328 backing streams at the
   points where they are known/captured/proven; the wrapper resolves all scalar
   and q-vector carry fields, applies them to the native context, and runs the
   materialized core. */
typedef struct {
    l3_authorization_finalizer_native_context_inputs base_native_inputs;

    uint32_t au3998_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_tail_39d8;
    uint32_t au3a98_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_tail_3ad8;

    uint32_t local188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_3bd8_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t local328_3c98_stream[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
} l3_authorization_finalizer_native_resolved_stream_inputs;

typedef struct {
    l3_authorization_finalizer_native_resolved_stream_inputs stream_inputs;
    l3_authorization_finalizer_native_carry_resolved_state resolved;
    l3_authorization_finalizer_native_context_inputs applied_native_inputs;
    l3_authorization_finalizer_native_context_state native_run;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_resolved_stream_state;



/* v153: native local_328 stream provider adapter.

   v152 still accepted raw native stream snapshots.  This adapter raises that
   boundary one level: callers provide the named materialized native states that
   own those streams, and the wrapper extracts the exact fields needed by the
   v151/v152 carry resolver.  It does not invent missing provenance; it simply
   prevents downstream code from treating local_328 snapshots as anonymous
   byte/word blobs. */
typedef struct {
    l3_authorization_finalizer_native_context_inputs base_native_inputs;

    l3_authorization_finalizer_feedback_state_a_stage05_state local188_3998;
    l3_authorization_finalizer_feedback_state_a_stage06_state local188_39d8;
    l3_authorization_finalizer_feedback_state_b_stage09_state local258_3a98;
    l3_authorization_finalizer_feedback_state_b_stage10_state local258_3ad8;

    l3_authorization_finalizer_feedback_state_a_stage13_stage14_affine_state local188_3bd8_3c18;
    l3_authorization_finalizer_feedback_state_b_stage16_stage17_affine_state local258_3c98_3cd8;
} l3_authorization_finalizer_native_stream_provider_inputs;

typedef struct {
    l3_authorization_finalizer_native_stream_provider_inputs provider_inputs;
    l3_authorization_finalizer_native_resolved_stream_inputs stream_inputs;
    l3_authorization_finalizer_native_resolved_stream_state resolved_run;

    uint32_t provider_flags;
    uint32_t stream_checksum;
    uint32_t resolved_checksum;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_stream_provider_state;




/* v154: stream-provider extraction from concrete predicate producer states.

   v153 accepted named provider states directly.  This adapter attaches those
   provider slots to the concrete materialized native producer states that own
   them: the v142 preloop records the 3998/39d8 and 3a98/3ad8 states, and
   v140/v141 cycle states record the sibling 3b58 and 3c58 branch providers. */
typedef struct {
    l3_authorization_finalizer_native_context_inputs base_native_inputs;

    l3_authorization_finalizer_predicate_preloop_state preloop;
    l3_authorization_finalizer_predicate_cycle_state cycle_3b58;
    l3_authorization_finalizer_predicate_cycle_state cycle_3c58;
} l3_authorization_finalizer_native_producer_provider_inputs;

typedef struct {
    l3_authorization_finalizer_native_producer_provider_inputs producer_inputs;
    l3_authorization_finalizer_native_stream_provider_inputs provider_inputs;
    l3_authorization_finalizer_native_stream_provider_state provider_run;

    uint32_t producer_flags;
    uint32_t producer_checksum;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_producer_provider_state;



/* v155: top-level native-run producer-provider bridge.

   v154 can resolve the remaining scalar/q-vector carry inputs from concrete
   predicate producer states, but callers still had to assemble those producer
   states by hand.  v155 exposes extraction from a top-level native-context run
   whose bounded predicate loop now records the last observed 3b58 and 3c58
   sibling producers.  If both sibling producers are present, the adapter can
   run the v154 provider bridge; otherwise it reports the partial snapshot and
   leaves the original native run outputs intact. */
#define L3_AUTH_FINAL_STATE_NATIVE_RUN_PROVIDER_HAS_PRELOOP 0x01u
#define L3_AUTH_FINAL_STATE_NATIVE_RUN_PROVIDER_HAS_3B58    0x02u
#define L3_AUTH_FINAL_STATE_NATIVE_RUN_PROVIDER_HAS_3C58    0x04u
#define L3_AUTH_FINAL_STATE_NATIVE_RUN_PROVIDER_COMPLETE    0x08u

typedef struct {
    l3_authorization_finalizer_native_context_state native_run;
    l3_authorization_finalizer_native_producer_provider_inputs producers;
    l3_authorization_finalizer_native_producer_provider_state provider_run;

    uint32_t snapshot_flags;
    uint32_t provider_ran;
    uint32_t preloop_stop_reason;
    uint32_t loop_stop_reason;
    uint32_t loop_trace_len;
    uint32_t last_cycle_path;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_run_provider_bridge_state;




/* v156: sibling-provider probe bridge.

   v155 extracts only the sibling producers that actually appeared during a
   bounded native predicate run.  This diagnostic adapter replays both sibling
   step helpers against the same loop-entry state and labels any non-native
   provider as a probe.  It does not change native control flow; it merely makes
   the path-selector outcome explicit and, when possible, can complete the
   provider set using clearly marked probe states. */
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_HAS_PRELOOP       0x00000001u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_NATIVE_3B58       0x00000002u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_NATIVE_3C58       0x00000004u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_STEP_3B58         0x00000010u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_STEP_3C58         0x00000020u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_COMPLETE_NATIVE   0x00000100u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_COMPLETE_PROBED   0x00000200u
#define L3_AUTH_FINAL_STATE_SIBLING_PROBE_ENTRY_TERMINAL    0x00000400u

typedef struct {
    l3_authorization_finalizer_native_context_state native_run;
    l3_authorization_finalizer_native_producer_provider_inputs native_producers;
    l3_authorization_finalizer_native_producer_provider_inputs probed_producers;
    l3_authorization_finalizer_native_producer_provider_state provider_run;

    l3_authorization_finalizer_predicate_cycle_state probe_3b58_cycle;
    l3_authorization_finalizer_predicate_cycle_state probe_3c58_cycle;

    uint32_t native_snapshot_flags;
    uint32_t probe_flags;
    uint32_t provider_ran;
    uint32_t native_last_path;
    uint32_t probe_3b58_path;
    uint32_t probe_3c58_path;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_sibling_probe_bridge_state;


/* v157: native 3c58-provider witness bridge.

   v156 showed that one native-context vector can take only the 3b58 sibling
   and therefore cannot provide the 3c58 stream without a diagnostic probe.
   This observer wraps the v155 native-run provider bridge and exposes an
   explicit completeness summary, so tests can pin real native vectors that
   retain both sibling providers without using the probe path. */
typedef struct {
    l3_authorization_finalizer_native_run_provider_bridge_state bridge;

    uint32_t has_preloop;
    uint32_t has_native_3b58;
    uint32_t has_native_3c58;
    uint32_t has_complete_native_provider;
    uint32_t provider_ran;
    uint32_t loop_trace_len;
    uint32_t loop_stop_reason;
    uint32_t last_cycle_path;
    uint32_t first_trace_path;
    uint32_t last_trace_path;

    uint32_t final_stop_reason;
    uint32_t terminal_taken;
    uint32_t output_1bc_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_1f0_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_188_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint32_t output_258_words[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    uint8_t raw40[L3_AUTH_FINAL_STATE_RAW40_LEN];
} l3_authorization_finalizer_native_stage15_provider_witness_state;



/* v158: complete native-provider replay comparison.

   When a bounded native-context run retains both sibling provider states, the
   v151/v153/v154 path can resolve the remaining scalar/q-vector carry inputs
   from the retained native local_328 streams.  This observer compares those
   resolved carries with the caller-supplied explicit boundary, applies the
   resolved carries back to the native context, and checks that the replayed
   v150 native-context run matches the provider bridge.  It does not synthesize
   missing providers; incomplete native snapshots are reported as such. */
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_COMPLETE_NATIVE       0x00000001u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_PROVIDER_RAN          0x00000002u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_TAIL39_MATCH          0x00000010u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_TAIL3A_MATCH          0x00000020u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_Q3B58_MATCH           0x00000040u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_PRESERVED_MATCH       0x00000080u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_Q5_3C58_MATCH         0x00000100u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_ORIGINAL_MATCHES_PROVIDER 0x00001000u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_REPLAY_MATCHES_PROVIDER   0x00002000u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_ORIGINAL_MATCHES_REPLAY   0x00004000u

#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_CHANGED_TAIL39        0x00000001u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_CHANGED_TAIL3A        0x00000002u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_CHANGED_Q3B58         0x00000004u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_CHANGED_PRESERVED     0x00000008u
#define L3_AUTH_FINAL_STATE_PROVIDER_REPLAY_CHANGED_Q5_3C58       0x00000010u

typedef struct {
    l3_authorization_finalizer_native_stage15_provider_witness_state witness;
    l3_authorization_finalizer_native_carry_resolved_state resolved;
    l3_authorization_finalizer_native_context_inputs replay_inputs;
    l3_authorization_finalizer_native_context_state replay_run;

    uint32_t comparison_flags;
    uint32_t changed_carry_mask;

    uint32_t input_tail_accum_39d8;
    uint32_t input_tail_accum_3ad8;
    uint32_t resolved_tail_accum_39d8;
    uint32_t resolved_tail_accum_3ad8;
    uint32_t input_q_3b58_xor;
    uint32_t resolved_q_3b58_xor;
    uint32_t input_q5_3c58_xor;
    uint32_t resolved_q5_3c58_xor;
    uint32_t input_preserved_tail12_3b58;
    uint32_t resolved_preserved_tail12_3b58;

    uint32_t original_output_checksum;
    uint32_t provider_output_checksum;
    uint32_t replay_output_checksum;
} l3_authorization_finalizer_native_provider_replay_compare_state;


#ifdef __cplusplus
}
#endif

#endif
