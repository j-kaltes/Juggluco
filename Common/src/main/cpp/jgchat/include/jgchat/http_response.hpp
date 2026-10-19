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
#include "jgchat/http.hpp"
#include <optional>
#include <string_view>

namespace jgchat {
// Shared by both HTTPS transports. Throws only generic errors, never request
// header values, credentials, URL queries, or request/response bodies.
void validate_http_request(const HttpRequest& request);
std::string serialize_http_request(const HttpRequest& request);

class HttpResponseParser {
public:
    explicit HttpResponseParser(std::size_t maximum_body = std::string{}.max_size(),
                                HttpBodySink on_body = {});
    void append(std::string_view bytes);
    bool complete() const;
    HttpResponse finish(); // EOF: rejects incomplete framed responses.
    HttpResponse result() const; // Requires complete().
private:
    enum class State { headers, fixed, chunk_size, chunk_data, trailers, eof_body, done };
    State state_ = State::headers;
    std::size_t maximum_, remaining_ = 0, trailer_bytes_ = 0;
    unsigned interim_ = 0;
    std::string pending_;
    HttpResponse response_;
    HttpBodySink on_body_;
    void body(std::string_view bytes);
    void process();
};

std::optional<HttpResponse> parse_http_response(std::string_view raw, bool eof = false,
                                               std::size_t maximum_body = std::string{}.max_size());
}
