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
#include "saved_authorization_record.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include "openssl_symbols.h"
#include "openssl_function_redirects.h"

#include <string.h>

static const uint8_t k_manager_id16_default[16] = {
    0x96,0x95,0x77,0x4b,0x9a,0x04,0x53,0x51,
    0xfb,0x16,0x0b,0xec,0x5f,0x49,0xdb,0xdf
};

static const uint8_t k_public16_default[16] = {
    0x9f,0xa7,0x91,0x24,0xfc,0x10,0x04,0x75,
    0xd7,0xb8,0x17,0x3d,0x99,0x15,0x56,0x77
};

static const uint8_t k_export_fixed_65_68[4] = {0x00,0x00,0x00,0x01};
static const uint8_t k_export_payload_tag[4] = {0xe8,0x20,0x54,0x69};
static const uint8_t k_export_fixed_93_96[4] = {0x00,0x00,0x00,0x10};
static const uint8_t k_export_zero_113_128[16] = {0};

static void wrbe32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

int l3_saved_authorization_record_make_header_mac(const uint8_t nonce16[16],
                                        const uint8_t header_prefix[0x45],
                                        uint8_t out20[20]) {
    if (!nonce16 || !header_prefix || !out20) return L3_SAVED_AUTHORIZATION_ERR_ARGUMENT;
    unsigned int n = 0;
    uint8_t tmp[EVP_MAX_MD_SIZE];
    if (!HMAC(EVP_sha1(), nonce16, 16, header_prefix, 0x45u, tmp, &n) || n != 20u) {
        return L3_SAVED_AUTHORIZATION_ERR_CRYPTO;
    }
    memcpy(out20, tmp, 20u);
    return L3_SAVED_AUTHORIZATION_OK;
}

int l3_saved_authorization_record_build_with_options(const uint8_t magic4[4],
                                       const uint8_t payload_tag4[4],
                                       const uint8_t manager_id16[16],
                                       const uint8_t public16[16],
                                       const uint8_t nonce16[16],
                                       const uint8_t payload16[16],
                                       const uint8_t trailer20[20],
                                       uint8_t out[L3_SAVED_AUTHORIZATION_RECORD_SIZE]) {
    if (!magic4 || !nonce16 || !payload16 || !trailer20 || !out)
        return L3_SAVED_AUTHORIZATION_ERR_ARGUMENT;
    if (!payload_tag4) payload_tag4 = k_export_payload_tag;
    if (!manager_id16) manager_id16 = k_manager_id16_default;
    if (!public16) public16 = k_public16_default;

    memset(out, 0, L3_SAVED_AUTHORIZATION_RECORD_SIZE);
    memcpy(out + L3_SAVED_AUTHORIZATION_MAGIC_OFFSET, magic4, 4u);
    out[L3_SAVED_AUTHORIZATION_MODE_OFFSET] = 0x02u;
    wrbe32(out + L3_SAVED_AUTHORIZATION_KIND_OFFSET, 0u);
    wrbe32(out + L3_SAVED_AUTHORIZATION_BIT_LENGTH_OFFSET, 0x10u);
    wrbe32(out + L3_SAVED_AUTHORIZATION_MANAGER_INDEX_OFFSET, 0u);
    memcpy(out + L3_SAVED_AUTHORIZATION_MANAGER_ID_OFFSET, manager_id16, 16u);
    memcpy(out + L3_SAVED_AUTHORIZATION_PUBLIC_DATA_OFFSET, public16, 16u);
    memcpy(out + L3_SAVED_AUTHORIZATION_NONCE_OFFSET, nonce16, 16u);
    wrbe32(out + L3_SAVED_AUTHORIZATION_COUNT_OFFSET, 1u);

    int rc = l3_saved_authorization_record_make_header_mac(nonce16, out, out + L3_SAVED_AUTHORIZATION_HEADER_MAC_OFFSET);
    if (rc) return rc;

    memcpy(out + L3_SAVED_AUTHORIZATION_PAYLOAD_TAG_OFFSET, payload_tag4, 4u);
    wrbe32(out + L3_SAVED_AUTHORIZATION_PAYLOAD_LENGTH_OFFSET, 0x10u);
    memcpy(out + L3_SAVED_AUTHORIZATION_ROOT_OFFSET, payload16, 16u);
    memcpy(out + L3_SAVED_AUTHORIZATION_TRAILER_OFFSET, trailer20, 20u);
    return L3_SAVED_AUTHORIZATION_OK;
}

int l3_saved_authorization_record_build(const uint8_t magic4[4],
                                    const uint8_t manager_id16[16],
                                    const uint8_t public16[16],
                                    const uint8_t nonce16[16],
                                    const uint8_t payload16[16],
                                    const uint8_t trailer20[20],
                                    uint8_t out[L3_SAVED_AUTHORIZATION_RECORD_SIZE]) {
    return l3_saved_authorization_record_build_with_options(magic4, NULL, manager_id16, public16,
                                              nonce16, payload16, trailer20, out);
}

int l3_saved_authorization_record_is_plausible(const uint8_t *bytes, size_t len) {
    if (!bytes) return L3_SAVED_AUTHORIZATION_ERR_ARGUMENT;
    if (len != L3_SAVED_AUTHORIZATION_RECORD_SIZE) return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    if (bytes[L3_SAVED_AUTHORIZATION_MODE_OFFSET] != 0x02u) return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_KIND_OFFSET, "\0\0\0\0", 4u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_BIT_LENGTH_OFFSET, "\0\0\0\x10", 4u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_MANAGER_INDEX_OFFSET, "\0\0\0\0", 4u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    /* Manager id and public constant are fixed in the captures so far, but they
     * are descriptor material rather than frame syntax.  Plausibility stays
     * structural; callers can compare these fields explicitly when needed.
     */
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_COUNT_OFFSET, k_export_fixed_65_68, 4u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    /* Payload tag is the second FUN_f3f604b0 output and changes with native
     * entropy, so do not require the v76 fixed-urandom value here.
     */
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_PAYLOAD_LENGTH_OFFSET, k_export_fixed_93_96, 4u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    if (memcmp(bytes + L3_SAVED_AUTHORIZATION_ZERO_BLOCK_OFFSET, k_export_zero_113_128, 16u) != 0)
        return L3_SAVED_AUTHORIZATION_ERR_FORMAT;
    return L3_SAVED_AUTHORIZATION_OK;
}

int l3_saved_authorization_record_verify_header_mac(const uint8_t *bytes, size_t len) {
    if (!bytes) return L3_SAVED_AUTHORIZATION_ERR_ARGUMENT;
    int rc = l3_saved_authorization_record_is_plausible(bytes, len);
    if (rc) return rc;
    uint8_t mac[20];
    rc = l3_saved_authorization_record_make_header_mac(bytes + L3_SAVED_AUTHORIZATION_NONCE_OFFSET, bytes, mac);
    if (rc) return rc;
    return memcmp(mac, bytes + L3_SAVED_AUTHORIZATION_HEADER_MAC_OFFSET, 20u) == 0
        ? L3_SAVED_AUTHORIZATION_OK : L3_SAVED_AUTHORIZATION_ERR_FORMAT;
}



