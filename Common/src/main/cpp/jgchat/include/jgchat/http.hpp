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
#include "ui_messages.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jgchat {
// Decoded successful-response body bytes, synchronously during the request.
// Presentation only: may be partial/unvalidated; never execute tools from it.
using HttpBodySink = std::function<void(std::string_view)>;
struct HttpRequest {
    std::string host;
    std::string path;
    std::string method = "POST";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    // Zero means no overall deadline; transports still check cancellation.
    std::chrono::milliseconds timeout{0};
    std::size_t max_response_bytes = std::string{}.max_size();
    unsigned short port = 443;
    HttpBodySink on_body;
};
struct HttpResponse {
    long status = 0;
    std::string body;
    std::string error;
    UiMessage ui_error{};
};
inline HttpResponse http_failure(UiMessage message) {
    return {0, {}, ui_diagnostic(message), std::move(message)};
}
using HttpClient = std::function<HttpResponse(const HttpRequest&, const std::atomic_bool&)>;
HttpClient native_https();
HttpClient curl_https();
}
