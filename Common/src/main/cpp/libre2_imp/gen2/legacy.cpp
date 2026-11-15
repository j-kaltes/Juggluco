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

#include "legacy.hpp"
#include <algorithm>
#include <string_view>
#include <utility>

namespace libre2_gen2 {

namespace {
int next_handle(int current) {
    return current >= 0x7ffffffe ? 1 : current + 1;
}
}

LegacyProtocol::LegacyProtocol(RandomBytes random, void *context)
    : random_(random), random_context_(context) {
    keys_[0].handle = 1; // The legacy key pool reserves this entry.
}

LegacyProtocol::KeyEntry *LegacyProtocol::find_key(int handle) {
    if (!handle) return nullptr;
    for (auto &entry : keys_) {
        if (entry.handle == handle) return &entry;
    }
    return nullptr;
}

LegacyProtocol::Session *LegacyProtocol::find_session(int handle) {
    for (auto &session : sessions_) {
        if (session.handle == handle) return &session;
    }
    return nullptr;
}

int LegacyProtocol::remember_key(KeyEntry entry) {
    last_key_ = next_handle(last_key_);
    auto free_entry = std::find_if(keys_.begin(), keys_.end(),
                                   [](const auto &key) { return key.handle == 0; });
    if (free_entry == keys_.end()) {
        // Abbott evicts the highest handle when all ten entries are occupied.
        free_entry = std::max_element(keys_.begin(), keys_.end(),
                [](const auto &a, const auto &b) { return a.handle < b.handle; });
        free_entry->handle = 0;
    }
    if (!find_key(last_key_)) {
        entry.handle = last_key_;
        *free_entry = std::move(entry);
    }
    return last_key_;
}

void LegacyProtocol::forget_key(int handle) {
    if (auto *entry = find_key(handle)) {
        entry->handle = 0;
        entry->identity.clear();
    }
}

int LegacyProtocol::open_session(int requested_root, View uid) {
    if (uid.size() < 2) return L2G2_INVALID_UID;
    const int root_slot = requested_root < 0 ? 1 : requested_root;
    const std::string_view identity(reinterpret_cast<const char *>(uid.data()), uid.size() - 2);
    int sensor_key_handle = 0;
    // The legacy cache matches UID context alone, even when the root changes.
    for (const auto &entry : keys_) {
        if (entry.handle && entry.sensor_key && entry.identity == identity) {
            sensor_key_handle = entry.handle;
            break;
        }
    }
    if (!sensor_key_handle) {
        KeyEntry derived;
        const int status = l2g2_derive_sensor_key(root_slot, uid.data(), uid.size(), derived.key.data());
        if (status != L2G2_OK) return status;
        derived.identity.assign(identity);
        derived.sensor_key = true;
        sensor_key_handle = remember_key(std::move(derived));
    }

    last_session_ = next_handle(last_session_);
    if (!find_session(last_session_)) {
        Session session;
        session.handle = last_session_;
        session.root_slot = root_slot;
        session.sensor_key_handle = sensor_key_handle;
        sessions_.push_back(session);
    }
    return last_session_;
}

int LegacyProtocol::accept_authentication(int handle, View parameters, View response) {
    if (parameters.empty()) return L2G2_AUTHENTICATION_FAILED;
    auto *session = find_session(handle);
    const auto *sensor_key = session ? find_key(session->sensor_key_handle) : nullptr;
    KeyEntry derived;
    // The response CRC is checked even for an unknown or evicted session.
    const int status = l2g2_accept_auth_response(
            session ? &session->exchange : nullptr, sensor_key ? sensor_key->key.data() : nullptr,
            session ? session->root_slot : 0, parameters[0], response.data(), response.size(),
            derived.key.data());
    if (status != L2G2_OK) return status;
    session->data_key_handle = remember_key(std::move(derived));
    l2g2_make_data_nonce(&session->exchange, parameters[0], session->data_nonce.data());
    return L2G2_OK;
}

int LegacyProtocol::close_session(int handle) {
    if (auto *session = find_session(handle)) {
        forget_key(session->data_key_handle);
        *session = sessions_.back();
        sessions_.pop_back();
    }
    return L2G2_OK;
}

int LegacyProtocol::cipher_self_test() {
    // The legacy self test temporarily occupies two key handles.
    forget_key(remember_key({}));
    forget_key(remember_key({}));
    return l2g2_cipher_self_test();
}

int LegacyProtocol::control(P1Operation operation, int argument, View first, View second) {
    switch (operation) {
    case P1Operation::initialize:
        return L2G2_OK;
    case P1Operation::cipher_self_test:
        return cipher_self_test();
    case P1Operation::open_session:
        return open_session(argument, first);
    case P1Operation::accept_authentication:
        return accept_authentication(argument, first, second);
    case P1Operation::close_session:
        return close_session(argument);
    }
    return 2; // Legacy status for an unknown operation.
}

size_t LegacyProtocol::write_result(P2Operation operation, int handle, View first,
                                     View second, Output output) {
    auto *session = find_session(handle);
    if (!session) return 0;

    switch (operation) {
    case P2Operation::build_nfc_auth_request:
    case P2Operation::build_ble_auth_request: {
        const bool ble = operation == P2Operation::build_ble_auth_request;
        const size_t challenge_size = ble ? L2G2_BLE_PROVISIONING_BYTES + L2G2_BLE_CHALLENGE_BYTES
                                          : L2G2_NFC_CHALLENGE_BYTES;
        if (first.size() < 2 || second.size() != challenge_size ||
            output.size() < L2G2_AUTH_REQUEST_BYTES) {
            return 0;
        }
        const auto *sensor_key = find_key(session->sensor_key_handle);
        uint8_t random[L2G2_RANDOM_BYTES];
        if (!sensor_key || !random_(random_context_, random)) return 0;
        if (ble) {
            l2g2_build_ble_auth_request(&session->exchange, sensor_key->key.data(), first[1],
                    second.data(), second.data() + L2G2_BLE_PROVISIONING_BYTES, random, output.data());
        } else if (!l2g2_build_nfc_auth_request(&session->exchange, sensor_key->key.data(),
                    first[1], second.data(), random, output.data())) {
            return 0;
        }
        return L2G2_AUTH_REQUEST_BYTES;
    }
    case P2Operation::read_command_reply: {
        if (first.size() < 2) return 0;
        const size_t result_size = std::max<size_t>(1, first[1]);
        const auto *sensor_key = find_key(session->sensor_key_handle);
        if (!sensor_key || output.size() < result_size ||
            !l2g2_read_command_reply(&session->exchange, sensor_key->key.data(), first[0],
                                    second.data(), second.size(), output.data(), first[1])) {
            return 0;
        }
        if (first[1] == 0) output[0] = 0;
        return result_size;
    }
    case P2Operation::export_metadata: {
        if (first.empty()) return 0;
        const size_t result_size = first[0];
        if (result_size < L2G2_METADATA_BYTES || output.size() < result_size) return 0;
        l2g2_write_metadata(&session->exchange, session->root_slot, output.data());
        std::fill(output.begin() + L2G2_METADATA_BYTES, output.begin() + result_size, 0);
        return result_size;
    }
    case P2Operation::decrypt_nfc_blocks:
    case P2Operation::decrypt_framed_nfc_blocks: {
        if (first.size() < 2 || first[1] == 0) return 0;
        const size_t header_size = operation == P2Operation::decrypt_framed_nfc_blocks ? 8 : 0;
        const size_t result_size = first[1] * L2G2_BLOCK_BYTES;
        const auto *data_key = find_key(session->data_key_handle);
        if (!data_key || output.size() < result_size || second.size() < header_size + result_size) {
            return 0;
        }
        l2g2_decrypt_nfc_blocks(data_key->key.data(), session->data_nonce.data(),
                                first[0], first[1], second.data() + header_size, output.data());
        return result_size;
    }
    case P2Operation::decrypt_ble_frame: {
        const auto *data_key = find_key(session->data_key_handle);
        if (!data_key || output.size() < first.size() ||
            !l2g2_decrypt_ble_frame(data_key->key.data(), session->exchange.stream_prefix,
                                    first.data(), first.size(), output.data())) {
            return 0;
        }
        return first.size();
    }
    }
    return 0;
}

} // namespace libre2_gen2
