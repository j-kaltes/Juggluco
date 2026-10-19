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
/*      Thu Oct 15 12:53:38 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "jgchat/http_response.hpp"
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <unordered_set>

namespace jgchat {
namespace {
constexpr std::size_t max_headers = 32768;
bool token(std::string_view value) {
    if (value.empty()) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || std::string_view("!#$%&'*+-.^_`|~").find(c) != std::string_view::npos;
    });
}
std::string lower(std::string_view value) {
    std::string result(value);
    for (auto& c : result) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return result;
}
std::string_view trim(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
    return value;
}
bool field_value(std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](unsigned char c) { return c == '\t' || (c >= 32 && c != 127); });
}
std::pair<std::string, std::string_view> field(std::string_view line) {
    const auto colon = line.find(':');
    if (colon == std::string_view::npos || !token(line.substr(0, colon)) || !field_value(line.substr(colon+1)))
        throw jgchat::UiError(jgchat::UiCode::malformed_https_header);
    return {lower(line.substr(0, colon)), trim(line.substr(colon+1))};
}
std::size_t number(std::string_view value, int base = 10) {
    std::size_t result = 0;
    auto parsed = std::from_chars(value.data(), value.data()+value.size(), result, base);
    if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size())
        throw jgchat::UiError(jgchat::UiCode::invalid_https_body_framing);
    return result;
}
}

void validate_http_request(const HttpRequest& r) {
    if (r.host.empty() || r.host.size() > 253 || !std::all_of(r.host.begin(), r.host.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '-';
    }) || !r.port || r.timeout.count() < 0 || !r.max_response_bytes)
        throw jgchat::UiError(jgchat::UiCode::invalid_https_request_configuration);
    if (r.path.empty() || r.path.front() != '/' || r.path.size() > 8192 ||
        r.path.find('#') != std::string::npos || !std::all_of(r.path.begin(), r.path.end(), [](unsigned char c) {
            return c >= 33 && c <= 126 && c != '\\';
        })) throw jgchat::UiError(jgchat::UiCode::invalid_https_request_path);
    if (r.method != "GET" && r.method != "POST" && r.method != "PUT" &&
        r.method != "DELETE" && r.method != "PATCH" && r.method != "OPTIONS")
        throw jgchat::UiError(jgchat::UiCode::unsupported_https_request_method);
    std::size_t size = 0;
    std::unordered_set<std::string> names;
    for (const auto& [key, value] : r.headers) {
        if (!token(key) || !field_value(value)) throw jgchat::UiError(jgchat::UiCode::invalid_https_request_header);
        const auto name = lower(key);
        if (!names.insert(name).second) throw jgchat::UiError(jgchat::UiCode::duplicate_https_request_header);
        if (name == "host" || name == "content-length" || name == "transfer-encoding" ||
            name == "connection" || name == "upgrade" || name == "trailer" || name == "te" ||
            name == "expect" || name == "proxy-authorization" || name == "proxy-connection")
            throw jgchat::UiError(jgchat::UiCode::reserved_https_request_header);
        if (name == "accept-encoding" && lower(trim(value)) != "identity")
            throw jgchat::UiError(jgchat::UiCode::compressed_https_responses_are_unsupported);
        size += key.size() + value.size() + 4;
        if (size > max_headers) throw jgchat::UiError(jgchat::UiCode::https_request_headers_too_large);
    }
}
std::string serialize_http_request(const HttpRequest& r) {
    validate_http_request(r);
    std::string result = r.method + " " + r.path + " HTTP/1.1\r\nHost: " + r.host + ":" +
        std::to_string(r.port) + "\r\nConnection: close\r\nContent-Length: " + std::to_string(r.body.size()) + "\r\n";
    bool encoding = false;
    for (const auto& [key, value] : r.headers) {
        result += key + ": " + value + "\r\n";
        if (lower(key) == "accept-encoding") encoding = true;
    }
    if (!encoding) result += "Accept-Encoding: identity\r\n";
    result += "\r\n";
    result += r.body;
    return result;
}
HttpResponseParser::HttpResponseParser(std::size_t maximum_body, HttpBodySink on_body)
    : maximum_(maximum_body), on_body_(std::move(on_body)) {
    if (!maximum_) throw jgchat::UiError(jgchat::UiCode::invalid_https_response_limit);
}
void HttpResponseParser::body(std::string_view bytes) {
    response_.body.append(bytes);
    if (!bytes.empty() && on_body_ && response_.status >= 200 && response_.status < 300) {
        // A UI/progress observer must not break authentication or framing.
        try { on_body_(bytes); } catch (...) {}
    }
}
bool HttpResponseParser::complete() const { return state_ == State::done; }
void HttpResponseParser::append(std::string_view bytes) {
    if (bytes.empty()) return;
    if (complete()) throw jgchat::UiError(jgchat::UiCode::unexpected_data_after_https_response);
    pending_.append(bytes);
    process();
}
void HttpResponseParser::process() {
    for (;;) {
        if (state_ == State::headers) {
            const auto end = pending_.find("\r\n\r\n");
            if (end == std::string::npos) {
                if (pending_.size() > max_headers) throw jgchat::UiError(jgchat::UiCode::https_response_headers_too_large);
                return;
            }
            if (end > max_headers) throw jgchat::UiError(jgchat::UiCode::https_response_headers_too_large);
            std::string_view head(pending_.data(), end + 2);
            const auto first_end = head.find("\r\n");
            const auto status = head.substr(0, first_end);
            if (status.size() < 12 || (status.substr(0, 9) != "HTTP/1.1 " && status.substr(0, 9) != "HTTP/1.0 ") ||
                (status.size() > 12 && status[12] != ' ') || !field_value(status))
                throw jgchat::UiError(jgchat::UiCode::malformed_https_status);
            const auto code = number(status.substr(9, 3));
            if (code < 100 || code > 599 || code == 101) throw jgchat::UiError(jgchat::UiCode::unsupported_https_status);
            bool has_length = false, chunked = false, encoding = false;
            std::size_t length = 0;
            for (auto pos = first_end + 2; pos < head.size();) {
                const auto next = head.find("\r\n", pos);
                if (next == std::string_view::npos) throw jgchat::UiError(jgchat::UiCode::malformed_https_headers);
                const auto [name, value] = field(head.substr(pos, next-pos));
                if (name == "content-length") {
                    if (has_length) throw jgchat::UiError(jgchat::UiCode::duplicate_https_content_length);
                    has_length = true; length = number(value);
                    if (length > maximum_) throw jgchat::UiError(jgchat::UiCode::https_response_too_large);
                } else if (name == "transfer-encoding") {
                    if (chunked || lower(value) != "chunked") throw jgchat::UiError(jgchat::UiCode::unsupported_https_transfer_encoding);
                    chunked = true;
                } else if (name == "content-encoding") {
                    if (encoding || lower(value) != "identity") throw jgchat::UiError(jgchat::UiCode::unsupported_https_content_encoding);
                    encoding = true;
                }
                pos = next + 2;
            }
            if (has_length && chunked) throw jgchat::UiError(jgchat::UiCode::ambiguous_https_body_framing);
            pending_.erase(0, end + 4);
            if (code < 200) {
                if (chunked || (has_length && length != 0) || ++interim_ > 8)
                    throw jgchat::UiError(jgchat::UiCode::invalid_interim_https_response);
                continue;
            }
            response_.status = static_cast<long>(code);
            if (code == 204 || code == 304) {
                if (chunked || (has_length && length != 0)) throw jgchat::UiError(jgchat::UiCode::unexpected_https_body_framing);
                state_ = State::done;
            } else if (chunked) state_ = State::chunk_size;
            else if (has_length) { remaining_ = length; state_ = State::fixed; }
            else state_ = State::eof_body;
        } else if (state_ == State::fixed || state_ == State::chunk_data) {
            const bool chunk = state_ == State::chunk_data;
            if (remaining_) {
                const auto count = std::min(remaining_, pending_.size());
                if (count > maximum_ - response_.body.size()) throw jgchat::UiError(jgchat::UiCode::https_response_too_large);
                body(std::string_view(pending_.data(), count)); pending_.erase(0, count); remaining_ -= count;
                if (remaining_) return;
            }
            if (chunk) {
                if (pending_.size() < 2) return;
                if (!pending_.starts_with("\r\n")) throw jgchat::UiError(jgchat::UiCode::malformed_https_chunk_terminator);
                pending_.erase(0, 2); state_ = State::chunk_size;
            } else state_ = State::done;
        } else if (state_ == State::chunk_size || state_ == State::trailers) {
            const auto end = pending_.find("\r\n");
            if (end == std::string::npos) {
                if (pending_.size() > 8192) throw jgchat::UiError(jgchat::UiCode::https_chunk_line_too_large);
                return;
            }
            if (end > 8192) throw jgchat::UiError(jgchat::UiCode::https_chunk_line_too_large);
            const std::string line = pending_.substr(0, end); pending_.erase(0, end + 2);
            if (state_ == State::trailers) {
                trailer_bytes_ += line.size()+2;
                if (trailer_bytes_ > max_headers) throw jgchat::UiError(jgchat::UiCode::https_trailers_too_large);
                if (line.empty()) state_ = State::done;
                else {
                    const auto [name, value] = field(line); (void)value;
                    if (name == "content-length" || name == "transfer-encoding" || name == "content-encoding")
                        throw jgchat::UiError(jgchat::UiCode::framing_field_in_https_trailer);
                }
            } else {
                if (!field_value(line)) throw jgchat::UiError(jgchat::UiCode::invalid_https_chunk_extension);
                remaining_ = number(std::string_view(line).substr(0, line.find(';')), 16);
                if (remaining_ > maximum_ - response_.body.size()) throw jgchat::UiError(jgchat::UiCode::https_response_too_large);
                state_ = remaining_ ? State::chunk_data : State::trailers;
            }
        } else if (state_ == State::eof_body) {
            if (pending_.size() > maximum_ - response_.body.size()) throw jgchat::UiError(jgchat::UiCode::https_response_too_large);
            body(pending_); pending_.clear(); return;
        } else {
            if (!pending_.empty()) throw jgchat::UiError(jgchat::UiCode::unexpected_data_after_https_body);
            return;
        }
    }
}
HttpResponse HttpResponseParser::finish() {
    process();
    if (state_ == State::eof_body) state_ = State::done;
    if (!complete()) throw jgchat::UiError(jgchat::UiCode::incomplete_https_response);
    return response_;
}
HttpResponse HttpResponseParser::result() const {
    if (!complete()) throw jgchat::UiError(jgchat::UiCode::incomplete_https_response);
    return response_;
}
std::optional<HttpResponse> parse_http_response(std::string_view raw, bool eof, std::size_t maximum_body) {
    HttpResponseParser parser(maximum_body); parser.append(raw);
    if (eof) return parser.finish();
    if (parser.complete()) return parser.result();
    return std::nullopt;
}
}
