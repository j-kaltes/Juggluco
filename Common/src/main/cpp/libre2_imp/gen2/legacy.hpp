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

#pragma once

#include "protocol.h"
#include <array>
#include <span>
#include <string>
#include <vector>

namespace libre2_gen2 {

enum class P1Operation : int {
    initialize = 0,
    cipher_self_test = 1,
    open_session = 0x7120,
    accept_authentication = 0x7319,
    close_session = 0x9218
};

enum class P2Operation : int {
    build_nfc_auth_request = 0x1928,
    build_ble_auth_request = 0x1969,
    read_command_reply = 0x5731,
    export_metadata = 0x1801,
    decrypt_nfc_blocks = 0x1978,
    decrypt_framed_nfc_blocks = 0x3101,
    decrypt_ble_frame = 0x0305
};

// Android and replay use the legacy handles and key eviction rules. A BLE
// client using protocol.h owns its exchange and key directly.
class LegacyProtocol {
public:
    using View = std::span<const uint8_t>;
    using Output = std::span<uint8_t>;
    using RandomBytes = bool (*)(void *context, uint8_t out[L2G2_RANDOM_BYTES]);

    explicit LegacyProtocol(RandomBytes random, void *context = nullptr);
    int control(P1Operation operation, int argument, View first, View second);
    // Return the byte count written, or zero for a legacy null result.
    size_t write_result(P2Operation operation, int handle, View first, View second,
                        Output output);

private:
    struct KeyEntry {
        std::array<uint8_t, L2G2_KEY_BYTES> key{};
        std::string identity;
        int handle{};
        bool sensor_key{};
    };

    struct Session {
        L2Gen2Challenge exchange{};
        std::array<uint8_t, L2G2_BLOCK_BYTES> data_nonce{};
        int handle{};
        int root_slot{};
        int sensor_key_handle{};
        int data_key_handle{};
    };

    RandomBytes random_;
    void *random_context_;
    std::array<KeyEntry, 10> keys_{};
    std::vector<Session> sessions_;
    int last_key_ = 1;
    int last_session_ = 0;

    KeyEntry *find_key(int handle);
    Session *find_session(int handle);
    int remember_key(KeyEntry entry);
    void forget_key(int handle);
    int open_session(int requested_root, View uid);
    int accept_authentication(int handle, View parameters, View response);
    int close_session(int handle);
    int cipher_self_test();
};

} // namespace libre2_gen2
