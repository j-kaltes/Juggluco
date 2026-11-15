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
/*      Sun Nov 15 13:57:27 CET 2026                                                 */

#pragma once
#include <cstdint>
#include <span>

namespace libre2_processing {
inline constexpr unsigned minutes_per_day = 24 * 60;
inline constexpr std::uint16_t libre_wear_minutes = 14 * minutes_per_day;
inline constexpr std::uint16_t libre_plus_wear_minutes = 15 * minutes_per_day;

// These are the generation ranges Juggluco uses to select its NFC and BLE
// exchanges. The region and manufacturer bytes may vary within a generation.
inline constexpr int libre_security_generation(std::uint8_t type) {
    switch (type >> 4) {
        case 3: return (type & 15) < 9 ? 1 : 2;
        case 7: return (type & 15) < 4 ? 1 : 2;
        default: return 0;
    }
}

inline constexpr bool encrypted_scan_only(std::uint8_t type) {
    return type >= 3 && type <= 15;
}

inline constexpr bool classic_payload_cipher(std::uint8_t type) {
    return (type >= 0x30 && type <= 0x38) || (type >= 0x70 && type <= 0x78);
}

inline constexpr std::uint16_t nominal_wear_minutes(std::uint8_t type) {
    switch (type) {
        case 4: case 0x31: case 0x36: case 0x3a: case 0x71: case 0x7a:
            return libre_plus_wear_minutes;
        case 6: case 0x32: case 0x37: case 0x3b: case 0x72: case 0x7b:
            return 21 * minutes_per_day;
        case 0x33:
            return 30 * minutes_per_day;
        case 5: case 0x34: case 0x38: case 0x3c: case 0x74: case 0x7c:
            return 10 * minutes_per_day;
        default:
            return libre_wear_minutes;
    }
}

struct PatchMetadata {
    unsigned family;
    std::uint8_t activation_command;
    unsigned warmup_minutes, wear_minutes;
};

inline constexpr PatchMetadata patch_metadata(std::span<const std::uint8_t, 6> info) {
    const auto type = info[2];
    const unsigned family = type >> 4;
    const bool signed_activation = encrypted_scan_only(type) || classic_payload_cipher(type);
    return {(family == 3 || family == 7) ? family : 0,
            std::uint8_t(signed_activation ? 0xa1 : 0xa0), 60, nominal_wear_minutes(type)};
}
} // namespace libre2_processing
