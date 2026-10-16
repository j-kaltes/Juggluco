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
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string_view>
#ifdef __ANDROID__
#include <android/log.h>
#include "logs.hpp"
#endif

namespace jgchat {
// Call sites pass fixed protocol names and numeric metadata only. Never pass
// request/response bodies, headers, OAuth codes, credentials or patient data.
using DiagnosticSink = void (*)(std::string_view);
inline std::atomic<DiagnosticSink> diagnostic_sink{nullptr};
inline std::atomic<unsigned long long> diagnostic_sequence{0};
inline thread_local unsigned long long diagnostic_operation = 0;

inline void diagnostic(const char* format, ...) noexcept __attribute__((format(printf, 1, 2)));
inline void diagnostic(const char* format, ...) noexcept {
    char message[1024];
    int prefix = std::snprintf(message, sizeof(message), "op=%llu ", diagnostic_operation);
    if (prefix < 0 || static_cast<std::size_t>(prefix) >= sizeof(message)) return;
    message[prefix] = '\0';
    va_list args;
    va_start(args, format);
    std::vsnprintf(message + prefix, sizeof(message) - static_cast<std::size_t>(prefix), format, args);
    va_end(args);
    message[sizeof(message) - 1] = '\0';
    for (char* p = message; *p; ++p) if (static_cast<unsigned char>(*p) < 0x20 || *p == 0x7f) *p = ' ';
    if (const auto sink = diagnostic_sink.load()) {
        try { sink(message); } catch (...) {} // Diagnostics must not break a request.
        return;
    }
#ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, "JugglucoChat", message);
    // Also include these events in Juggluco's normal native log when enabled.
#if !defined(NOLOG) && !defined(LOGCAT)
    LOGGER("JugglucoChat: %s\n", message);
#endif
#else
    std::fprintf(stderr, "JugglucoChat: %s\n", message);
#endif
}

class DiagnosticOperation {
    const char* name_;
    const char* stage_ = "start";
    const char* outcome_ = "failed";
    unsigned long long previous_ = diagnostic_operation;
    std::chrono::steady_clock::time_point started_ = std::chrono::steady_clock::now();
public:
    explicit DiagnosticOperation(const char* name) noexcept : name_(name) {
        if (!diagnostic_operation) diagnostic_operation = ++diagnostic_sequence;
        diagnostic("begin operation=%s", name_);
    }
    ~DiagnosticOperation() {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_).count();
        diagnostic("end operation=%s stage=%s outcome=%s elapsed_ms=%lld", name_, stage_, outcome_,
                   static_cast<long long>(elapsed));
        diagnostic_operation = previous_;
    }
    DiagnosticOperation(const DiagnosticOperation&) = delete;
    DiagnosticOperation& operator=(const DiagnosticOperation&) = delete;
    void stage(const char* name) noexcept {
        if (std::strcmp(stage_, name) != 0) {
            stage_ = name;
            diagnostic("operation=%s stage=%s", name_, stage_);
        }
    }
    void success() noexcept { outcome_ = "ok"; }
    void cancelled() noexcept { outcome_ = "cancelled"; }
};
} // namespace jgchat
