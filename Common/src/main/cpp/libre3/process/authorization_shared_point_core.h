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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#ifndef L3_AUTHORIZATION_SHARED_POINT_CORE_H
#define L3_AUTHORIZATION_SHARED_POINT_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "authorization_shared_point_frames.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN 0x42u
#define L3_AUTHORIZATION_SHARED_POINT_FRAME_COUNT 2u
#define L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN \
    (L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN * \
     L3_AUTHORIZATION_SHARED_POINT_FRAME_COUNT)

enum {
    L3_AUTHORIZATION_SHARED_POINT_OK = 0,
    L3_AUTHORIZATION_SHARED_POINT_ERR_ARGUMENT = -1,
    L3_AUTHORIZATION_SHARED_POINT_ERR_WORKSPACE = -2,
    L3_AUTHORIZATION_SHARED_POINT_ERR_ENGINE = -3
};

/* Shared fixed encoding tables used by the generic Android compatibility
   transforms as well as by the allocator-free fixed core. */
extern const l3_authorization_frame_tables
    l3_authorization_shared_point_frame_tables;

/* Fixed-shape, OpenSSL-free authorization leaf.  Scratch may be static or
   caller-owned and can be reused for the next point immediately after return. */
size_t l3_authorization_shared_point_workspace_size(void);
size_t l3_authorization_shared_point_workspace_alignment(void);

int l3_authorization_shared_point_frames_into(
    const uint8_t native_scalar35[35],
    const uint8_t peer_public64[64],
    void *workspace,
    size_t workspace_size,
    uint8_t out_frames[L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN]);

#ifdef __cplusplus
}
#endif

#endif
