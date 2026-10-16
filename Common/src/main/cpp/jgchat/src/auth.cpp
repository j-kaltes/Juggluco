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
// Device authorization and refresh flow adapted from OpenAI Codex, Copyright
// 2025 OpenAI, Apache-2.0, commit e72da2b53805894878023d01949a25a082e0a5cb:
// codex-rs/login/src/{device_code_auth.rs,server.rs,oauth/client.rs,
// auth/manager.rs,token_data.rs}. See licenses/Codex-{Apache-2.0,NOTICE}.txt.
#include "jgchat/ui_messages.hpp"
#include "jgchat/auth.hpp"
#include "jgchat/diagnostics.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace jgchat {
namespace {
constexpr std::string_view client_id = "app_EMoamEEZ73f0CkXaXp7hrann";
constexpr std::string_view issuer_host = "auth.openai.com";
constexpr std::string_view callback = "https://auth.openai.com/deviceauth/callback";
constexpr std::string_view verification = "https://auth.openai.com/codex/device";
constexpr std::size_t response_limit = 1024 * 1024;
constexpr std::size_t token_limit = 64 * 1024;
using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;

[[noreturn]] void invalid() {
    throw jgchat::UiError(jgchat::UiCode::invalid_authentication_response);
}
void cancelled(const std::atomic_bool& cancel) {
    if (cancel.load()) throw jgchat::UiError(jgchat::UiCode::authentication_cancelled);
}
bool success(long status) { return status >= 200 && status < 300; }
bool safe_string(std::string_view value, std::size_t limit) {
    return !value.empty() && value.size() <= limit &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) { return c > 32 && c < 127; });
}
std::string string_field(const Json& object, const char* name, std::size_t limit,
                         bool required = true) {
    const auto it = object.find(name);
    if (it == object.end() || it->is_null()) {
        if (required) invalid();
        return {};
    }
    if (!it->is_string()) invalid();
    auto value = it->get<std::string>();
    if (!safe_string(value, limit)) invalid();
    return value;
}

// Bound nesting before building a DOM, including for attacker-controlled JWT
// metadata. JSON syntax/escaping is subsequently checked by the JSON parser.
Json parse_object(std::string_view text, std::size_t limit = response_limit) {
    if (text.size() > limit) invalid();
    unsigned depth = 0;
    bool quoted = false, escape = false;
    for (const char c : text) {
        if (quoted) {
            if (escape) escape = false;
            else if (c == '\\') escape = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') {
            if (++depth > 32) invalid();
        } else if (c == '}' || c == ']') {
            if (!depth) invalid();
            --depth;
        }
    }
    auto result = Json::parse(text.begin(), text.end(), nullptr, false);
    if (result.is_discarded() || !result.is_object()) invalid();
    return result;
}

std::uint64_t unsigned_integer(const Json& value) {
    if (value.is_number_unsigned()) return value.get<std::uint64_t>();
    if (!value.is_number_integer()) invalid();
    const auto result = value.get<std::int64_t>();
    if (result < 0) invalid();
    return static_cast<std::uint64_t>(result);
}
std::int64_t epoch_seconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
std::int64_t epoch_field(const Json& value) {
    const auto result = unsigned_integer(value);
    if (result > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) invalid();
    return static_cast<std::int64_t>(result);
}
unsigned interval_field(const Json& object) {
    const auto it = object.find("interval");
    if (it == object.end()) return 5;
    std::uint64_t value = 0;
    if (it->is_string()) {
        auto raw = it->get<std::string>();
        const auto first = raw.find_first_not_of(" \t\r\n");
        const auto last = raw.find_last_not_of(" \t\r\n");
        if (first == std::string::npos || last - first > 20) invalid();
        for (std::size_t i = first; i <= last; ++i) {
            if (raw[i] < '0' || raw[i] > '9') invalid();
            const auto digit = static_cast<unsigned>(raw[i] - '0');
            if (value > (std::numeric_limits<unsigned>::max() - digit) / 10U) invalid();
            value = value * 10U + digit;
        }
    } else value = unsigned_integer(*it);
    if (value > std::numeric_limits<unsigned>::max()) invalid();
    return std::max(1U, static_cast<unsigned>(value));
}
Milliseconds remaining(Clock::time_point deadline, const std::atomic_bool& cancel) {
    cancelled(cancel);
    const auto now = Clock::now();
    if (now >= deadline) throw jgchat::UiError(jgchat::UiCode::device_authorization_expired);
    return std::max(Milliseconds(1), std::min(Milliseconds(60000),
        std::chrono::duration_cast<Milliseconds>(deadline - now)));
}
HttpRequest request(std::string path, std::string body, bool form = false) {
    HttpRequest result;
    result.host = issuer_host;
    result.path = std::move(path);
    result.body = std::move(body);
    result.headers.emplace_back("Content-Type", form ? "application/x-www-form-urlencoded" : "application/json");
    result.headers.emplace_back("Accept", "application/json");
    result.max_response_bytes = response_limit;
    return result;
}
HttpResponse send_request(const HttpClient& http, const HttpRequest& req,
                    const std::atomic_bool& cancel, bool retain_success = false) {
    DiagnosticOperation trace("authentication_http");
    const char* endpoint = req.path == "/api/accounts/deviceauth/usercode" ? "device_code" :
        req.path == "/api/accounts/deviceauth/token" ? "device_poll" : req.path == "/oauth/token" ? "token" : "other";
    diagnostic("auth endpoint=%s request_bytes=%zu timeout_ms=%lld", endpoint, req.body.size(),
        static_cast<long long>(req.timeout.count()));
    cancelled(cancel);
    HttpResponse result;
    trace.stage("transport");
    try { result = http(req, cancel); }
    catch (...) {
        diagnostic("auth transport exception cancel=%d", cancel.load() ? 1 : 0);
        if (cancel.load()) trace.cancelled();
        cancelled(cancel);
        // A transport exception can contain request bodies and bearer tokens.
        throw jgchat::UiError(jgchat::UiCode::authentication_network_request_failed);
    }
    // A refresh may already have rotated its token. Do not throw away a received
    // success just because cancellation arrived concurrently; the caller must
    // persist the returned credentials before honoring cancellation.
    diagnostic("auth received status=%ld response_bytes=%zu transport_error=%d cancel=%d",
        result.status, result.body.size(), result.error.empty() ? 0 : 1, cancel.load() ? 1 : 0);
    if (cancel.load() && (!retain_success || !success(result.status))) trace.cancelled();
    if (!retain_success || !success(result.status)) cancelled(cancel);
    if (!result.error.empty()) throw jgchat::UiError(jgchat::UiCode::authentication_network_request_failed);
    if (result.body.size() > response_limit) invalid();
    trace.success();
    return result;
}
void require_success(const HttpResponse& response, bool refresh = false) {
    if (success(response.status)) return;
    if (refresh && (response.status == 401 || response.status == 403))
        throw jgchat::UiError(jgchat::UiCode::chatgpt_sign_in_expired_sign_in_again);
    // Do not include the response body, transport error, or OAuth error details.
    throw UiError({UiCode::authentication_http, {std::to_string(response.status)}});
}
std::string form_escape(std::string_view value) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : value) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~')
            result.push_back(static_cast<char>(c));
        else {
            result.push_back('%');
            result.push_back(hex[c >> 4]);
            result.push_back(hex[c & 15]);
        }
    }
    return result;
}
int base64_digit(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}
std::string decode_base64url(std::string_view value) {
    if (value.empty() || value.size() % 4 == 1) invalid();
    std::string result;
    result.reserve(value.size() * 3 / 4);
    unsigned bits = 0, bit_count = 0;
    for (unsigned char c : value) {
        const int digit = base64_digit(c);
        if (digit < 0) invalid();
        bits = (bits << 6U) | static_cast<unsigned>(digit);
        bit_count += 6;
        if (bit_count >= 8) {
            bit_count -= 8;
            result.push_back(static_cast<char>((bits >> bit_count) & 255U));
        }
    }
    if (bit_count && (bits & ((1U << bit_count) - 1U))) invalid();
    return result;
}
struct Metadata { std::string account_id; std::int64_t expires_at = 0; };
Metadata metadata(std::string_view token, bool must_be_jwt) {
    Metadata result;
    const auto first = token.find('.');
    if (first == std::string_view::npos && !must_be_jwt) return result;
    const auto second = first == std::string_view::npos ? first : token.find('.', first + 1);
    if (first == std::string_view::npos || first == 0 || second == std::string_view::npos ||
        second == first + 1 || second + 1 == token.size() || token.find('.', second + 1) != std::string_view::npos)
        invalid();
    // Metadata decoding only: this is NOT JWT signature verification. Tokens
    // originate from a verified HTTPS response (or app-private saved state).
    const auto claims = parse_object(decode_base64url(token.substr(first + 1, second - first - 1)), token_limit);
    const auto auth = claims.find("https://api.openai.com/auth");
    if (auth != claims.end() && !auth->is_null()) {
        if (!auth->is_object()) invalid();
        result.account_id = string_field(*auth, "chatgpt_account_id", 1024, false);
    }
    const auto exp = claims.find("exp");
    if (exp != claims.end() && !exp->is_null()) result.expires_at = epoch_field(*exp);
    return result;
}
void merge_account(std::string& account, const std::string& candidate) {
    if (candidate.empty()) return;
    if (!account.empty() && account != candidate) invalid();
    account = candidate;
}
void validate_tokens(const Tokens& tokens) {
    if (!safe_string(tokens.access_token, token_limit) ||
        !safe_string(tokens.refresh_token, token_limit) ||
        (!tokens.id_token.empty() && !safe_string(tokens.id_token, token_limit)) ||
        !safe_string(tokens.account_id, 1024) || tokens.expires_at < 0) invalid();
}
Tokens token_response(const Json& response, const Tokens* previous = nullptr) {
    Tokens tokens;
    tokens.access_token = string_field(response, "access_token", token_limit);
    tokens.refresh_token = string_field(response, "refresh_token", token_limit, !previous);
    tokens.id_token = string_field(response, "id_token", token_limit, false);
    if (previous) {
        if (tokens.refresh_token.empty()) tokens.refresh_token = previous->refresh_token;
        if (tokens.id_token.empty()) tokens.id_token = previous->id_token;
        tokens.account_id = previous->account_id;
    }
    merge_account(tokens.account_id, string_field(response, "account_id", 1024, false));
    const auto access = metadata(tokens.access_token, false);
    merge_account(tokens.account_id, access.account_id);
    if (!tokens.id_token.empty()) merge_account(tokens.account_id, metadata(tokens.id_token, true).account_id);
    tokens.expires_at = access.expires_at;
    const auto lifetime = response.find("expires_in");
    if (lifetime != response.end() && !lifetime->is_null()) {
        const auto seconds = epoch_field(*lifetime);
        const auto now = epoch_seconds();
        if (now < 0 || seconds > std::numeric_limits<std::int64_t>::max() - now) invalid();
        const auto expiry = now + seconds;
        tokens.expires_at = tokens.expires_at ? std::min(tokens.expires_at, expiry) : expiry;
    }
    validate_tokens(tokens);
    return tokens;
}
void poll_wait(unsigned seconds, Clock::time_point deadline, const std::atomic_bool& cancel) {
    const auto next = std::min(deadline, Clock::now() + std::chrono::seconds(seconds));
    for (;;) {
        cancelled(cancel);
        const auto now = Clock::now();
        if (now >= deadline) throw jgchat::UiError(jgchat::UiCode::device_authorization_expired);
        if (now >= next) return;
        std::this_thread::sleep_for(std::min(Milliseconds(100),
            std::max(Milliseconds(1), std::chrono::duration_cast<Milliseconds>(next - now))));
    }
}
} // namespace

DeviceCode request_device_code(const HttpClient& http, const std::atomic_bool& cancel) {
    const auto deadline = Clock::now() + std::chrono::minutes(15);
    auto req = request("/api/accounts/deviceauth/usercode", Json{{"client_id", client_id}}.dump());
    req.timeout = remaining(deadline, cancel);
    const auto response = send_request(http, req, cancel);
    require_success(response);
    const auto body = parse_object(response.body);
    DeviceCode result;
    result.verification_url = verification;
    result.device_auth_id = string_field(body, "device_auth_id", 4096);
    result.user_code = string_field(body, "user_code", 256, false);
    const auto alias = string_field(body, "usercode", 256, false);
    if (result.user_code.empty()) result.user_code = alias;
    else if (!alias.empty() && alias != result.user_code) invalid();
    if (result.user_code.empty()) invalid();
    result.interval_seconds = interval_field(body);
    result.deadline = deadline;
    return result;
}

Tokens complete_device_login(const HttpClient& http, const DeviceCode& device, const std::atomic_bool& cancel) {
    if (!safe_string(device.device_auth_id, 4096) || !safe_string(device.user_code, 256)) invalid();
    const auto deadline = std::min(device.deadline, Clock::now() + std::chrono::minutes(15));
    for (;;) {
        auto req = request("/api/accounts/deviceauth/token",
            Json{{"device_auth_id", device.device_auth_id}, {"user_code", device.user_code}}.dump());
        req.timeout = remaining(deadline, cancel);
        const auto response = send_request(http, req, cancel);
        if (response.status == 403 || response.status == 404) {
            poll_wait(std::max(1U, device.interval_seconds), deadline, cancel);
            continue;
        }
        require_success(response);
        const auto body = parse_object(response.body);
        const auto code = string_field(body, "authorization_code", token_limit);
        const auto verifier = string_field(body, "code_verifier", 4096);
        (void)string_field(body, "code_challenge", 4096);
        auto exchange = request("/oauth/token", "grant_type=authorization_code&client_id=" +
            form_escape(client_id) + "&code=" + form_escape(code) + "&redirect_uri=" +
            form_escape(callback) + "&code_verifier=" + form_escape(verifier), true);
        exchange.timeout = remaining(deadline, cancel);
        // One-time authorization codes must not be retried after an ambiguous
        // network failure; the server may already have consumed the code.
        const auto exchanged = send_request(http, exchange, cancel, true);
        require_success(exchanged);
        return token_response(parse_object(exchanged.body));
    }
}

Tokens refresh_tokens(const HttpClient& http, const Tokens& previous, const std::atomic_bool& cancel) {
    validate_tokens(previous);
    const auto req = request("/oauth/token", Json{{"client_id", client_id},
        {"grant_type", "refresh_token"}, {"refresh_token", previous.refresh_token}}.dump());
    // Refresh tokens may rotate. Never retry here; the caller serializes refresh
    // and persists the entire successful result immediately.
    const auto response = send_request(http, req, cancel, true);
    require_success(response, true);
    return token_response(parse_object(response.body), &previous);
}

Json tokens_to_json(const Tokens& tokens) {
    validate_tokens(tokens);
    return Json{{"access_token", tokens.access_token}, {"refresh_token", tokens.refresh_token},
        {"id_token", tokens.id_token}, {"account_id", tokens.account_id}, {"expires_at", tokens.expires_at}};
}

Tokens tokens_from_json(const Json& body) {
    if (!body.is_object()) invalid();
    Tokens tokens;
    tokens.access_token = string_field(body, "access_token", token_limit);
    tokens.refresh_token = string_field(body, "refresh_token", token_limit);
    // Empty ID token is the serialized representation when the server omits it.
    const auto id = body.find("id_token");
    if (id != body.end() && !id->is_null()) {
        if (!id->is_string()) invalid();
        tokens.id_token = id->get<std::string>();
    }
    tokens.account_id = string_field(body, "account_id", 1024);
    const auto expiry = body.find("expires_at");
    if (expiry != body.end() && !expiry->is_null()) tokens.expires_at = epoch_field(*expiry);
    validate_tokens(tokens);
    std::string account = tokens.account_id;
    merge_account(account, metadata(tokens.access_token, false).account_id);
    if (!tokens.id_token.empty()) merge_account(account, metadata(tokens.id_token, true).account_id);
    return tokens;
}
} // namespace jgchat
