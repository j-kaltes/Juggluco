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
#ifndef L3_APP_PRIVATE_KEY_WHITEBOX_MACHINE_H
#define L3_APP_PRIVATE_KEY_WHITEBOX_MACHINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_PRIVATE_KEY_MACHINE_OK 0
#define L3_PRIVATE_KEY_MACHINE_ERR_ARGUMENT -1
#define L3_PRIVATE_KEY_MACHINE_ERR_TABLE_SIZE -2
#define L3_PRIVATE_KEY_MACHINE_ERR_NEED_DISPATCH_PROGRAMS -3
#define L3_PRIVATE_KEY_MACHINE_ERR_NEED_FULL_METHOD -4
#define L3_PRIVATE_KEY_MACHINE_ERR_NOT_IMPLEMENTED -100 /* deprecated: no production path should return this */

typedef struct {
    const uint8_t *state_transition_table;  size_t state_transition_table_len;
    const uint8_t *mix_program_table;  size_t mix_program_table_len;
    const uint8_t *map_program_table;  size_t map_program_table_len;
    /* Optional runtime snapshot of DAT_f4369990 used as the f80040 static work prefix.
     * When NULL, the compiled package prefix is used for backward compatibility. */
    const uint8_t *decoder_static_prefix; size_t decoder_static_prefix_len;

    /* Optional generated dispatch programs.  When these are populated, the
     * legacy l3_private_key_decode_object_method(...) entrypoint can execute the full
     * native-equivalent v44 object method directly, without an installed
     * callback/context shim. */
    const uint8_t *stage1_program; size_t stage1_program_len;
    const uint8_t *stage2_program; size_t stage2_program_len;
} l3_private_key_tables;

/* Ported scalar table helpers used heavily by FUN_f3f7e1d0. */
int l3_private_key_table_mix6_a(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]);

int l3_private_key_table_map6(const l3_private_key_tables *t, uint32_t k,
                const uint8_t in[6], uint8_t out[6]);

int l3_private_key_table_mix6_c(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]);

int l3_private_key_table_mix6_b(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[6]);
int l3_private_key_table_mix6_to4_a(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]);
int l3_private_key_table_mix6_to4_b(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]);
int l3_private_key_table_mix5_to4(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[5], const uint8_t b[5], uint8_t out[4]);
int l3_private_key_table_mix6_to4_c(const l3_private_key_tables *t, uint32_t k,
                const uint8_t a[6], const uint8_t b[6], uint8_t out[4]);
int l3_private_key_table_map6_to4(const l3_private_key_tables *t, uint32_t k,
                const uint8_t in[6], uint8_t out[4]);

int l3_private_key_table_transform(const l3_private_key_tables *t, uint32_t param1, uint32_t param2,
                const uint8_t *p3, const uint8_t *p4, uint8_t *out, size_t out_len);

/*
 * Port of FUN_f3f7da3c, the FUN_f3f84990 object-state constructor.
 * It expands the compact manager A8 entry into the object method state at
 * native self+4.  Native allocates 0x5a8 bytes for the object; this routine
 * fills the 0x5a4-byte state region after the vtable word.
 */
int l3_private_key_construct_object_state(const uint8_t *manager_a8_selected,
                                size_t manager_a8_selected_len,
                                uint8_t *out_state_0x5a4,
                                size_t out_state_len);

typedef int (*l3_private_key_object_method_cb)(void *user,
                                       const uint8_t input16[16],
                                       uint8_t out42[0x42]);

/*
 * Compatibility boundary for the native chain:
 *   FUN_f3f84990(manager_a8_selected) -> object
 *   object vtable[0] wrapper f3f85c4c(input16, out42, object)
 *   f3f85c4c -> FUN_f3f7e1d0(input16, out42, object+4)
 *
 * The full generated-dispatch implementation lives in f3f7_outer_schedule_ref.
 * v162 lets this legacy entrypoint run it directly when the table descriptor
 * carries both generated dispatch programs.  l3_private_key_decode_configure_object_method
 * remains available as a compatibility override for older callers that bind a
 * replay context explicitly.
 */


#ifdef __cplusplus
}
#endif

#endif
