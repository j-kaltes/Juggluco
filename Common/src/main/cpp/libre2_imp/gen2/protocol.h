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

#ifndef JUGGLUCO_LIBRE2_GEN2_PROTOCOL_H
#define JUGGLUCO_LIBRE2_GEN2_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L2G2_KEY_BYTES = 16,
    L2G2_RANDOM_BYTES = 7,
    L2G2_AUTH_REQUEST_BYTES = 19,
    L2G2_NFC_CHALLENGE_BYTES = 25,
    L2G2_BLE_CHALLENGE_BYTES = 14,
    L2G2_BLE_PROVISIONING_BYTES = 10,
    L2G2_BLE_RESPONSE_BYTES = 25,
    L2G2_NFC_RESPONSE_BYTES = 33,
    L2G2_BLE_FRAME_BYTES = 46,
    L2G2_METADATA_BYTES = 9,
    L2G2_BLOCK_BYTES = 8,
    L2G2_NFC = 0,
    L2G2_BLE = 1
};

enum {
    L2G2_OK = 0,
    L2G2_INVALID_UID = -9,
    L2G2_UNSUPPORTED_ROOT = -13,
    L2G2_AUTHENTICATION_FAILED = -15,
    L2G2_BAD_CRC = -17
};

// One exchange. All fields use their wire byte order.
typedef struct {
    uint8_t stream_prefix[4];
    uint8_t salt[3];
    uint8_t counter[4];
    uint8_t sensor_random[L2G2_RANDOM_BYTES];
    uint8_t host_random[L2G2_RANDOM_BYTES];
    uint8_t nfc_epoch[2];
} L2Gen2Challenge;

// The UID includes the two trailing manufacturer bytes. They are omitted
// from the key derivation context.
int l2g2_derive_sensor_key(int root_slot, const uint8_t *uid, size_t uid_size,
                          uint8_t key[L2G2_KEY_BYTES]);

bool l2g2_build_nfc_auth_request(L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t command,
        const uint8_t challenge[L2G2_NFC_CHALLENGE_BYTES],
        const uint8_t host_random[L2G2_RANDOM_BYTES],
        uint8_t request[L2G2_AUTH_REQUEST_BYTES]);

// The first ten provisioning bytes are saved when the phone enables streaming.
// challenge is the fourteen-byte notification from the BLE login characteristic.
void l2g2_build_ble_auth_request(L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t command,
        const uint8_t provisioning[L2G2_BLE_PROVISIONING_BYTES],
        const uint8_t challenge[L2G2_BLE_CHALLENGE_BYTES],
        const uint8_t host_random[L2G2_RANDOM_BYTES],
        uint8_t request[L2G2_AUTH_REQUEST_BYTES]);

// Verify the host's random echo and derive the data key. session_key may
// alias sensor_key when the caller only needs BLE streaming after login.
int l2g2_accept_auth_response(const L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], int root_slot, uint8_t transport,
        const uint8_t *response, size_t response_size,
        uint8_t session_key[L2G2_KEY_BYTES]);

// Verify both random echoes and extract an activation/stream-enable reply.
// payload has payload_size bytes (the wire length is at most 255).
bool l2g2_read_command_reply(const L2Gen2Challenge *exchange,
        const uint8_t sensor_key[L2G2_KEY_BYTES], uint8_t transport,
        const uint8_t *response, size_t response_size,
        uint8_t *payload, uint8_t payload_size);

void l2g2_write_metadata(const L2Gen2Challenge *exchange, int root_slot,
                         uint8_t metadata[L2G2_METADATA_BYTES]);
void l2g2_make_data_nonce(const L2Gen2Challenge *exchange, uint8_t transport,
                          uint8_t nonce[L2G2_BLOCK_BYTES]);

// Decryption supports input == output. The caller checks the FRAM or BLE CRC
// after decryption. A BLE frame keeps its two-byte sequence number.
void l2g2_decrypt_nfc_blocks(const uint8_t session_key[L2G2_KEY_BYTES],
        const uint8_t nonce[L2G2_BLOCK_BYTES], uint8_t first_block,
        uint8_t block_count, const uint8_t *input, uint8_t *output);
bool l2g2_decrypt_ble_frame(const uint8_t session_key[L2G2_KEY_BYTES],
        const uint8_t stream_prefix[4], const uint8_t *input,
        size_t frame_size, uint8_t *output);

uint16_t l2g2_crc16(const uint8_t *data, size_t size);
bool l2g2_cipher_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
