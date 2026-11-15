// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "tools.hpp"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace jgchat {

// Match URLSearchParams for query fields, and use the existing browser URL
// encoding for the optional secret path segment.
inline std::string web_encode(std::string_view text, bool query) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == (query ? '*' : '~')) out += char(c);
        else if (query && c == ' ') out += '+';
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}

inline Json web_probe_error(std::string_view code, std::string_view message) {
    return {{"status", "error"}, {"error", {{"code", code}, {"message", message}}}};
}

// This is a complete in-process reply from the ordinary watchserver GET
// handler, not a general HTTP client. Only known textual bodies are returned.
// Never include the authenticated URL, arbitrary headers or an error body.
inline Json web_probe_sample(std::string_view wire) {
    const auto split = wire.find("\r\n\r\n"), first = wire.find("\r\n");
    if (split == wire.npos || first == wire.npos || first > split || first < 12 ||
        (!wire.starts_with("HTTP/1.1 ") && !wire.starts_with("HTTP/1.0 ")))
        return web_probe_error("invalid_web_response", "Juggluco did not produce a complete HTTP response.");
    unsigned code = 0;
    const auto [ptr, error] = std::from_chars(wire.data() + 9, wire.data() + 12, code);
    if (error != std::errc{} || ptr != wire.data() + 12 || code < 200 || code > 599)
        return web_probe_error("invalid_web_response", "Juggluco did not produce a valid HTTP status.");
    if (code >= 300) {
        auto result = web_probe_error("web_http_error", "Juggluco returned HTTP " + std::to_string(code) + ".");
        result["http_status"] = code;
        return result;
    }
    std::string_view media;
    for (auto start = first + 2; start < split;) {
        const auto end = wire.find("\r\n", start);
        auto field = wire.substr(start, end - start);
        constexpr std::string_view name = "content-type:";
        if (field.size() >= name.size() && std::equal(name.begin(), name.end(), field.begin(), [](char a, char b) {
                return a == (b >= 'A' && b <= 'Z' ? b + ('a' - 'A') : b);
            })) {
            field.remove_prefix(name.size());
            const auto non_space = field.find_first_not_of(" \t");
            if (non_space != field.npos) field.remove_prefix(non_space);
            media = field.substr(0, field.find_first_of("; \t"));
        }
        start = end + 2;
    }
    const auto body = wire.substr(split + 4);
    if (media != "text/plain" && media != "text/tab-separated-values" &&
        media != "application/json" && media != "text/html")
        return {{"status", "error"}, {"http_status", code}, {"body_bytes", body.size()},
                {"error", {{"code", "unexpected_web_type"}, {"message", "The endpoint did not return a supported text response."}}}};
    constexpr std::size_t preview_limit = 16 * 1024;
    auto preview = body.substr(0, preview_limit);
    if (preview.size() < body.size() && media != "application/json") {
        const auto newline = preview.rfind('\n');
        if (newline != preview.npos) preview = preview.substr(0, newline + 1);
    }
    // Legacy labels and a cut UTF-8 character must not break JSON serialization.
    auto text = Json::parse(Json(preview).dump(-1, ' ', false, Json::error_handler_t::replace));
    return {{"status", "ok"}, {"http_status", code}, {"content_type", media},
            {"body_bytes", body.size()}, {"preview_truncated", preview.size() < body.size()},
            {"body_preview", std::move(text)}};
}

} // namespace jgchat

namespace jgchatdata {
// The caller supplies only an allowlisted read path and encoded query.
jgchat::Json probe_web_get(std::string_view target);
}
