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
/*      Thu Oct 15 12:54:42 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <string_view>

namespace jgchat {
// Browser URLs percent-encode secrets used as a path segment. Authenticate the
// complete decoded segment before stripping it; never decode the API/file path.
inline bool consume_encoded_secret(std::string_view& target, std::string_view secret) {
    if (secret.empty()) return false;
    const auto slash = target.find('/');
    if (slash == std::string_view::npos || slash > secret.size() * 3 || slash + 1 >= target.size() ||
        target[slash + 1] <= ' ' || target.substr(0, slash).find('%') == std::string_view::npos) return false;
    auto hex = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; };
    std::string decoded;
    for (std::size_t i = 0; i < slash; ++i) {
        char c = target[i];
        if (c == '%') {
            if (i + 2 >= slash) return false;
            const int a = hex(target[i + 1]), b = hex(target[i + 2]);
            if (a < 0 || b < 0) return false;
            c = static_cast<char>((a << 4) | b); i += 2;
        }
        decoded += c;
        if (decoded.size() > secret.size()) return false;
    }
    if (decoded != secret) return false;
    target.remove_prefix(slash + 1);
    return true;
}
}
