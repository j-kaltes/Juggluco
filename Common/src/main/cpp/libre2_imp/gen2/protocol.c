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
/*      Sun Nov 15 13:58:54 CET 2026                                                 */

#include "protocol.h"
#include <string.h>

enum { SPECK_ROUNDS = 27 };

static const uint8_t sensor_roots[2][L2G2_KEY_BYTES] = {
    {0xa1, 0xaa, 0x73, 0x8f, 0x0c, 0xf7, 0x58, 0x01, 0xa1, 0x6d, 0xd4, 0xf7, 0x90, 0x6e, 0x97, 0x2a},
    {0x3c, 0xe8, 0xbd, 0xbb, 0x8c, 0xb3, 0x0a, 0x48, 0xa7, 0xd6, 0x93, 0xf7, 0xb7, 0x2e, 0xec, 0x6c}
};
static const uint8_t session_roots[2][L2G2_KEY_BYTES] = {
    {0xe8, 0x2f, 0x8f, 0xc3, 0x21, 0x99, 0x56, 0x3c, 0xfd, 0xa8, 0x0c, 0x7f, 0x6c, 0x2a, 0x67, 0x21},
    {0x4e, 0x28, 0x73, 0xbd, 0x16, 0xc5, 0x4a, 0xdc, 0xe0, 0x38, 0xea, 0xd5, 0xb2, 0x5b, 0x68, 0xa3}
};

// The terminating zero is the separator in the CMAC KDF.
static const uint8_t sensor_label[17] = "ADC_*SpeckPatch#";
static const uint8_t session_label[17] = "ADC_*SpecKpatch#";

static uint32_t load32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void store32(uint8_t *bytes, uint32_t word) {
    for (unsigned i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t)(word >> (8 * i));
    }
}

static uint32_t rotate_right(uint32_t word, unsigned bits) {
    return (word >> bits) | (word << (32 - bits));
}

static uint32_t rotate_left(uint32_t word, unsigned bits) {
    return (word << bits) | (word >> (32 - bits));
}

static void expand_key(const uint8_t key[L2G2_KEY_BYTES],
                       uint32_t round_keys[SPECK_ROUNDS]) {
    uint32_t current = load32(key);
    uint32_t oldest = load32(key + 4);
    uint32_t middle = load32(key + 8);
    uint32_t newest = load32(key + 12);
    for (unsigned round = 0; round < SPECK_ROUNDS; ++round) {
        round_keys[round] = current;
        const uint32_t next = (rotate_right(oldest, 8) + current) ^ round;
        current = rotate_left(current, 3) ^ next;
        oldest = middle;
        middle = newest;
        newest = next;
    }
}

static void encrypt_block(const uint32_t round_keys[SPECK_ROUNDS],
                          const uint8_t input[8], uint8_t output[8]) {
    uint32_t left = load32(input + 4);
    uint32_t right = load32(input);
    for (unsigned round = 0; round < SPECK_ROUNDS; ++round) {
        left = (rotate_right(left, 8) + right) ^ round_keys[round];
        right = rotate_left(right, 3) ^ left;
    }
    store32(output, right);
    store32(output + 4, left);
}

static void xor_keystream(const uint32_t round_keys[SPECK_ROUNDS],
                          const uint8_t nonce[8], const uint8_t *input,
                          uint8_t *output, size_t count) {
    uint8_t keystream[8];
    encrypt_block(round_keys, nonce, keystream);
    for (size_t i = 0; i < count; ++i) {
        output[i] = input[i] ^ keystream[i];
    }
}

static void crypt_ctr(const uint8_t key[L2G2_KEY_BYTES], uint8_t nonce[8],
                      const uint8_t *input, uint8_t *output, size_t size) {
    uint32_t round_keys[SPECK_ROUNDS];
    expand_key(key, round_keys);
    while (size) {
        const size_t count = size < 8 ? size : 8;
        xor_keystream(round_keys, nonce, input, output, count);
        // The counter wraps within byte zero without carry.
        ++nonce[0];
        input += count;
        output += count;
        size -= count;
    }
}

static void double_cmac_block(uint8_t block[8]) {
    const bool carry = (block[0] & 0x80) != 0;
    for (unsigned i = 0; i < 7; ++i) {
        block[i] = (uint8_t)((block[i] << 1) | (block[i + 1] >> 7));
    }
    block[7] = (uint8_t)((block[7] << 1) ^ (carry ? 0x1b : 0));
}

typedef struct {
    uint8_t block[8];
    unsigned used;
} Cmac;

// Keep the last block pending so finalization can choose the correct subkey.
static void cmac_feed(Cmac *cmac, const uint32_t round_keys[SPECK_ROUNDS],
                      const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (cmac->used == 8) {
            encrypt_block(round_keys, cmac->block, cmac->block);
            cmac->used = 0;
        }
        cmac->block[cmac->used++] ^= data[i];
    }
}

static void derive_key(const uint8_t root[L2G2_KEY_BYTES], const uint8_t label[17],
                       const uint8_t *context, size_t context_size,
                       uint8_t key[L2G2_KEY_BYTES]) {
    static const uint8_t output_bits[] = {128, 0};
    uint32_t round_keys[SPECK_ROUNDS];
    uint8_t subkey[8] = {0};
    expand_key(root, round_keys);
    encrypt_block(round_keys, subkey, subkey);
    double_cmac_block(subkey);
    if ((1 + 17 + context_size + sizeof(output_bits)) % 8) {
        double_cmac_block(subkey);
    }

    for (uint8_t counter = 1; counter <= 2; ++counter) {
        Cmac cmac = {{0}, 0};
        cmac_feed(&cmac, round_keys, &counter, 1);
        cmac_feed(&cmac, round_keys, label, 17);
        cmac_feed(&cmac, round_keys, context, context_size);
        cmac_feed(&cmac, round_keys, output_bits, sizeof(output_bits));
        if (cmac.used < 8) {
            cmac.block[cmac.used] ^= 0x80;
        }
        for (unsigned i = 0; i < 8; ++i) {
            cmac.block[i] ^= subkey[i];
        }
        encrypt_block(round_keys, cmac.block, key + 8 * (counter - 1));
    }
}

static uint16_t crc_byte(uint16_t crc, uint8_t byte) {
    crc ^= byte;
    for (unsigned bit = 0; bit < 8; ++bit) {
        crc = (uint16_t)((crc >> 1) ^ ((crc & 1) ? 0x8408 : 0));
    }
    return crc;
}

uint16_t l2g2_crc16(const uint8_t *data, size_t size) {
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < size; ++i) {
        crc = crc_byte(crc, data[i]);
    }
    if (size & 1) {
        crc = crc_byte(crc, 0);
    }
    uint16_t reflected = 0;
    for (unsigned bit = 0; bit < 16; ++bit) {
        reflected = (uint16_t)((reflected << 1) | (crc & 1));
        crc >>= 1;
    }
    return reflected;
}

static bool matches_crc(const uint8_t *frame, size_t payload_size) {
    const uint16_t stored = frame[payload_size] | ((uint16_t)frame[payload_size + 1] << 8);
    return l2g2_crc16(frame, payload_size) == stored;
}

int l2g2_derive_sensor_key(int root_slot, const uint8_t *uid, size_t uid_size,
                          uint8_t key[L2G2_KEY_BYTES]) {
    if (uid_size < 2) return L2G2_INVALID_UID;
    if (root_slot < 1 || root_slot > 2) return L2G2_UNSUPPORTED_ROOT;
    derive_key(sensor_roots[root_slot - 1], sensor_label, uid, uid_size - 2, key);
    return L2G2_OK;
}

static void make_auth_nonce(const L2Gen2Challenge *exchange, uint8_t transport,
                            unsigned counter_step, uint8_t nonce[8]) {
    nonce[0] = 0;
    nonce[1] = exchange->salt[0];
    nonce[2] = exchange->salt[1];
    nonce[3] = (uint8_t)((exchange->salt[2] & 0x7f) | (transport == L2G2_BLE ? 0x80 : 0));
    store32(nonce + 4, load32(exchange->counter) + counter_step);
}

static void write_auth_request(L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t transport,
        uint8_t command, const uint8_t *ble_extra,
        const uint8_t host_random[L2G2_RANDOM_BYTES], uint8_t request[19]) {
    const unsigned header_size = transport == L2G2_BLE ? 1 : 4;
    uint8_t *payload = request + header_size;
    uint8_t nonce[8];
    make_auth_nonce(exchange, transport, 0, nonce);
    memcpy(exchange->host_random, host_random, L2G2_RANDOM_BYTES);
    memset(request, 0, header_size - 1);
    request[header_size - 1] = command;
    memcpy(payload, host_random, L2G2_RANDOM_BYTES);
    payload[7] = (uint8_t)~command;
    if (ble_extra) {
        memcpy(payload + 8, ble_extra, 3);
    }
    memcpy(payload + 8 + (ble_extra ? 3 : 0), exchange->sensor_random, L2G2_RANDOM_BYTES);
    crypt_ctr(sensor_key, nonce, payload, payload, L2G2_AUTH_REQUEST_BYTES - header_size);
}

bool l2g2_build_nfc_auth_request(L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t command,
        const uint8_t challenge[L2G2_NFC_CHALLENGE_BYTES],
        const uint8_t host_random[L2G2_RANDOM_BYTES], uint8_t request[19]) {
    if (!matches_crc(challenge, 23)) return false;
    memcpy(exchange->sensor_random, challenge, 7);
    memcpy(exchange->counter, challenge + 7, 4);
    memcpy(exchange->salt, challenge + 14, 3);
    memcpy(exchange->stream_prefix, challenge + 17, 4);
    memcpy(exchange->nfc_epoch, challenge + 21, 2);
    write_auth_request(exchange, sensor_key, L2G2_NFC, command, NULL, host_random, request);
    return true;
}

void l2g2_build_ble_auth_request(L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t command,
        const uint8_t provisioning[L2G2_BLE_PROVISIONING_BYTES],
        const uint8_t challenge[L2G2_BLE_CHALLENGE_BYTES],
        const uint8_t host_random[L2G2_RANDOM_BYTES], uint8_t request[19]) {
    memcpy(exchange->stream_prefix, provisioning, 4);
    memcpy(exchange->salt, provisioning + 4, 3);
    memcpy(exchange->sensor_random, challenge, 7);
    memcpy(exchange->counter, challenge + 7, 4);
    write_auth_request(exchange, sensor_key, L2G2_BLE, command, provisioning + 7,
                       host_random, request);
}

int l2g2_accept_auth_response(const L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], int root_slot, uint8_t transport,
        const uint8_t *response, size_t response_size, uint8_t session_key[L2G2_KEY_BYTES]) {
    const unsigned header_size = transport == L2G2_NFC ? 8 : 0;
    uint8_t plaintext[23];
    uint8_t nonce[8];
    if (response_size != header_size + sizeof(plaintext) + 2) {
        return L2G2_AUTHENTICATION_FAILED;
    }
    const uint8_t *payload = response + header_size;
    if (transport == L2G2_NFC && !matches_crc(payload, sizeof(plaintext))) {
        return L2G2_BAD_CRC;
    }
    if (!sensor_key) return L2G2_AUTHENTICATION_FAILED;
    make_auth_nonce(exchange, transport, 1, nonce);
    crypt_ctr(sensor_key, nonce, payload, plaintext, sizeof(plaintext));
    if (memcmp(plaintext, exchange->host_random, L2G2_RANDOM_BYTES) != 0) {
        return L2G2_AUTHENTICATION_FAILED;
    }
    if (root_slot < 1 || root_slot > 2) return L2G2_UNSUPPORTED_ROOT;
    derive_key(session_roots[root_slot - 1], session_label, plaintext + 7, 16, session_key);
    return L2G2_OK;
}

bool l2g2_read_command_reply(const L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t transport,
        const uint8_t *response, size_t response_size,
        uint8_t *payload, uint8_t payload_size) {
    const unsigned header_size = transport == L2G2_NFC ? 8 : 0;
    const unsigned encrypted_size = payload_size + 2 * L2G2_RANDOM_BYTES;
    uint8_t plaintext[UINT8_MAX + 2 * L2G2_RANDOM_BYTES];
    uint8_t nonce[8];
    if (response_size != header_size + encrypted_size + 2) return false;
    const uint8_t *encrypted = response + header_size;
    if (transport == L2G2_NFC && !matches_crc(encrypted, encrypted_size)) return false;
    make_auth_nonce(exchange, transport, 1, nonce);
    crypt_ctr(sensor_key, nonce, encrypted, plaintext, encrypted_size);
    if (memcmp(plaintext, exchange->sensor_random, L2G2_RANDOM_BYTES) != 0 ||
        memcmp(plaintext + L2G2_RANDOM_BYTES + payload_size,
               exchange->host_random, L2G2_RANDOM_BYTES) != 0) {
        return false;
    }
    if (payload_size) memcpy(payload, plaintext + L2G2_RANDOM_BYTES, payload_size);
    return true;
}

void l2g2_write_metadata(const L2Gen2Challenge *exchange, int root_slot,
                         uint8_t metadata[L2G2_METADATA_BYTES]) {
    memcpy(metadata, exchange->stream_prefix, 4);
    memcpy(metadata + 4, exchange->salt, 3);
    metadata[7] = (uint8_t)root_slot;
    metadata[8] = (uint8_t)((unsigned)root_slot >> 8);
}

void l2g2_make_data_nonce(const L2Gen2Challenge *exchange, uint8_t transport,
                          uint8_t nonce[8]) {
    memset(nonce, 0, 8);
    memcpy(nonce + 2, exchange->stream_prefix, 4);
    if (transport == L2G2_NFC) {
        nonce[5] &= 0x7f;
        memcpy(nonce + 6, exchange->nfc_epoch, 2);
    } else {
        nonce[5] |= 0x80;
    }
}

void l2g2_decrypt_nfc_blocks(const uint8_t session_key[L2G2_KEY_BYTES],
        const uint8_t session_nonce[8], uint8_t first_block,
        uint8_t block_count, const uint8_t *input, uint8_t *output) {
    uint32_t round_keys[SPECK_ROUNDS];
    uint8_t nonce[8];
    memcpy(nonce, session_nonce, sizeof(nonce));
    expand_key(session_key, round_keys);
    for (unsigned block = 0; block < block_count; ++block) {
        nonce[1] = (uint8_t)(first_block + block);
        xor_keystream(round_keys, nonce, input + 8 * block, output + 8 * block, 8);
    }
}

bool l2g2_decrypt_ble_frame(const uint8_t session_key[L2G2_KEY_BYTES],
        const uint8_t stream_prefix[4], const uint8_t *input,
        size_t frame_size, uint8_t *output) {
    if (frame_size < L2G2_BLE_FRAME_BYTES) return false;
    uint8_t nonce[8] = {0};
    memcpy(nonce + 2, stream_prefix, 4);
    nonce[5] |= 0x80;
    output[0] = nonce[6] = input[0];
    output[1] = nonce[7] = input[1];
    crypt_ctr(session_key, nonce, input + 2, output + 2, frame_size - 2);
    return true;
}

bool l2g2_cipher_self_test(void) {
    static const uint8_t key[L2G2_KEY_BYTES] = {
        0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27
    };
    static const uint8_t plain[8] = {0x2d, 0x43, 0x75, 0x74, 0x74, 0x65, 0x72, 0x3b};
    static const uint8_t expected[8] = {0x8b, 0x02, 0x4e, 0x45, 0x48, 0xa5, 0x6f, 0x8c};
    uint32_t round_keys[SPECK_ROUNDS];
    uint8_t encrypted[8];
    expand_key(key, round_keys);
    encrypt_block(round_keys, plain, encrypted);
    if (memcmp(encrypted, expected, sizeof(expected)) != 0) return false;
    uint32_t left = load32(encrypted + 4);
    uint32_t right = load32(encrypted);
    for (unsigned round = SPECK_ROUNDS; round > 0; --round) {
        right = rotate_right(right ^ left, 3);
        left = rotate_left((left ^ round_keys[round - 1]) - right, 8);
    }
    return left == load32(plain + 4) && right == load32(plain);
}
