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
#include "http.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>

namespace jgchat {
using Json = nlohmann::json;
struct Tokens {
    std::string access_token, refresh_token, id_token, account_id;
    std::int64_t expires_at = 0;
};
struct DeviceCode {
    std::string verification_url, user_code, device_auth_id;
    unsigned interval_seconds = 5;
    std::chrono::steady_clock::time_point deadline;
};
DeviceCode request_device_code(const HttpClient&, const std::atomic_bool&);
Tokens complete_device_login(const HttpClient&, const DeviceCode&, const std::atomic_bool&);
Tokens refresh_tokens(const HttpClient&, const Tokens&, const std::atomic_bool&);
Json tokens_to_json(const Tokens&);
Tokens tokens_from_json(const Json&);
}
