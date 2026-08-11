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
#ifndef L3_SAVED_AUTHORIZATION_RECORD_H
#define L3_SAVED_AUTHORIZATION_RECORD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_SAVED_AUTHORIZATION_RECORD_SIZE = 149u,

    L3_SAVED_AUTHORIZATION_MAGIC_OFFSET          = 0x00u,
    L3_SAVED_AUTHORIZATION_MODE_OFFSET           = 0x04u,
    L3_SAVED_AUTHORIZATION_KIND_OFFSET           = 0x05u,
    L3_SAVED_AUTHORIZATION_BIT_LENGTH_OFFSET     = 0x09u,
    L3_SAVED_AUTHORIZATION_MANAGER_INDEX_OFFSET  = 0x0du,
    L3_SAVED_AUTHORIZATION_MANAGER_ID_OFFSET     = 0x11u,
    L3_SAVED_AUTHORIZATION_PUBLIC_DATA_OFFSET    = 0x21u,
    L3_SAVED_AUTHORIZATION_NONCE_OFFSET          = 0x31u,
    L3_SAVED_AUTHORIZATION_COUNT_OFFSET          = 0x41u,
    L3_SAVED_AUTHORIZATION_HEADER_MAC_OFFSET     = 0x45u,
    L3_SAVED_AUTHORIZATION_HEADER_MAC_SIZE       = 0x14u,
    L3_SAVED_AUTHORIZATION_PAYLOAD_TAG_OFFSET    = 0x59u,
    L3_SAVED_AUTHORIZATION_PAYLOAD_TAG_SIZE      = 0x04u,
    L3_SAVED_AUTHORIZATION_PAYLOAD_LENGTH_OFFSET = 0x5du,
    L3_SAVED_AUTHORIZATION_ROOT_OFFSET           = 0x61u,
    L3_SAVED_AUTHORIZATION_ZERO_BLOCK_OFFSET     = 0x71u,
    L3_SAVED_AUTHORIZATION_TRAILER_OFFSET        = 0x81u,
    L3_SAVED_AUTHORIZATION_TRAILER_SIZE          = 0x14u
};

enum {
    L3_SAVED_AUTHORIZATION_OK = 0,
    L3_SAVED_AUTHORIZATION_ERR_ARGUMENT = -1,
    L3_SAVED_AUTHORIZATION_ERR_FORMAT = -2,
    L3_SAVED_AUTHORIZATION_ERR_CRYPTO = -3
};

/* Validate the fixed 149-byte persisted authorization envelope and its
 * HMAC-SHA1 header authenticator. */
int l3_saved_authorization_record_is_plausible(const uint8_t *bytes, size_t len);
int l3_saved_authorization_record_verify_header_mac(const uint8_t *bytes, size_t len);

int l3_saved_authorization_record_make_header_mac(
    const uint8_t nonce16[16],
    const uint8_t header_prefix[0x45],
    uint8_t out20[20]);

/* Serialize the persisted authorization envelope.  The recovered backend
 * supplies the white-box-derived payload and trailer bytes. */
int l3_saved_authorization_record_build_with_options(
    const uint8_t magic4[4],
    const uint8_t payload_tag4[4],
    const uint8_t manager_id16[16],
    const uint8_t public16[16],
    const uint8_t nonce16[16],
    const uint8_t payload16[16],
    const uint8_t trailer20[20],
    uint8_t out[L3_SAVED_AUTHORIZATION_RECORD_SIZE]);

int l3_saved_authorization_record_build(
    const uint8_t magic4[4],
    const uint8_t manager_id16[16],
    const uint8_t public16[16],
    const uint8_t nonce16[16],
    const uint8_t payload16[16],
    const uint8_t trailer20[20],
    uint8_t out[L3_SAVED_AUTHORIZATION_RECORD_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* L3_SAVED_AUTHORIZATION_RECORD_H */
