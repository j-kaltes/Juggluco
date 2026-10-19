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
#include "jgchat/http.hpp"
#include "jgchat/http_response.hpp"
#include <curl/curl.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace jgchat {
namespace {
std::once_flag init_once;
CURLcode init_result = CURLE_FAILED_INIT;
struct Reader {
    HttpResponseParser parser;
    UiMessage error;
    Reader(std::size_t maximum, HttpBodySink sink) : parser(maximum, std::move(sink)) {}
};
std::size_t receive(char* bytes, std::size_t size, std::size_t count, void* opaque) {
    auto& reader = *static_cast<Reader*>(opaque);
    if (size && count > static_cast<std::size_t>(-1) / size) return 0;
    const auto length = size * count;
    try { reader.parser.append(std::string_view(bytes, length)); }
    catch (...) { reader.error = current_ui_error(); return 0; }
    return length;
}
int progress(void* opaque, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return static_cast<const std::atomic_bool*>(opaque)->load() ? 1 : 0;
}
HttpResponse perform(const HttpRequest& request, const std::atomic_bool& cancel) {
    validate_http_request(request);
    if (cancel.load()) return http_failure(UiCode::https_cancelled);
    std::call_once(init_once, [] { init_result = curl_global_init(CURL_GLOBAL_DEFAULT); });
    if (init_result != CURLE_OK) throw jgchat::UiError(jgchat::UiCode::https_library_initialization_failed);
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
    if (!handle) throw jgchat::UiError(jgchat::UiCode::https_request_allocation_failed);
    auto* curl = handle.get();
    Reader reader(request.max_response_bytes, request.on_body);
    const auto url = "https://" + request.host + ":" + std::to_string(request.port) + request.path;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request.method.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request.body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    // Match the curl command-line client's conventional custom CA environment.
    // Verification remains enabled; this also permits an ephemeral CA for
    // localhost tests without changing the machine's trust store.
    if (const char* ca = std::getenv("CURL_CA_BUNDLE"); ca && *ca)
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca);
    else if (const char* ca = std::getenv("SSL_CERT_FILE"); ca && *ca)
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    const auto timeout = static_cast<long>(std::min<std::int64_t>(request.timeout.count(), LONG_MAX));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
    // Keep environment proxy selection intact. CONNECT response headers belong
    // to the proxy, not to the authenticated HTTPS response being parsed.
    curl_easy_setopt(curl, CURLOPT_SUPPRESS_CONNECT_HEADERS, 1L);
    // Shared parser validates both framing headers and raw chunk boundaries.
    curl_easy_setopt(curl, CURLOPT_HTTP_TRANSFER_DECODING, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTP_CONTENT_DECODING, 0L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, receive);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &reader);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reader);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &cancel);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(nullptr, curl_slist_free_all);
    auto add = [&](const std::string& value) {
        auto* next = curl_slist_append(headers.get(), value.c_str());
        if (!next) throw jgchat::UiError(jgchat::UiCode::https_header_allocation_failed);
        headers.release(); headers.reset(next);
    };
    for (const auto& [key, value] : request.headers) add(key + ": " + value);
    add("Expect:");
    add("Connection: close");
    // An empty Content-Type value in libcurl's list suppresses its automatic
    // application/x-www-form-urlencoded header if the caller omitted one.
    bool content_type = false, encoding = false;
    for (const auto& [key, value] : request.headers) {
        (void)value;
        std::string lower = key;
        for (char& c : lower) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (lower == "content-type") content_type = true;
        if (lower == "accept-encoding") encoding = true;
    }
    if (!content_type) add("Content-Type:");
    if (!encoding) add("Accept-Encoding: identity");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get());
    const auto result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        if (cancel.load()) return http_failure(UiCode::https_cancelled);
        if (!reader.error.empty()) return http_failure(reader.error);
        // curl's detailed error buffer can contain URL/query text. Use only its
        // fixed code description so credentials cannot enter transport errors.
        return http_failure(UiMessage::detail(UiCode::https_transport_failed, curl_easy_strerror(result)));
    }
    return reader.parser.finish();
}
}
HttpClient curl_https() {
    return [](const HttpRequest& request, const std::atomic_bool& cancel) -> HttpResponse {
        try { return perform(request, cancel); }
        catch (...) { return http_failure(current_ui_error()); }
    };
}
}
