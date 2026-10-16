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
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jgchat {
enum class UiCode : std::uint16_t {
    none = 0,
#define JGCHAT_UI_MESSAGE(id, name, resource, diagnostic) name = id,
#include "ui_messages.inc"
#undef JGCHAT_UI_MESSAGE
};
enum class UiFormat : unsigned { plain = 0, detail = 1, range = 2 };

// IDs select Android resources directly. Only variable content is text.
struct UiMessage {
    UiCode code = UiCode::none;
    UiFormat format = UiFormat::plain;
    std::vector<std::string> args;
    UiMessage() = default;
    UiMessage(UiCode id, std::vector<std::string> values = {}) : code(id), args(std::move(values)) {}
    static UiMessage detail(UiCode code, std::string detail) {
        UiMessage out(code);
        if (!detail.empty()) { out.format = UiFormat::detail; out.args.push_back(std::move(detail)); }
        return out;
    }
    static UiMessage range(UiCode code, std::string start, std::string end) {
        UiMessage out(code, {std::move(start), std::move(end)}); out.format = UiFormat::range; return out;
    }
    bool empty() const { return code == UiCode::none; }
    void clear() { *this = UiMessage{}; }
    bool operator==(const UiMessage&) const = default;
};
inline bool is_cancellation(const UiMessage& value) {
    return value.code == UiCode::request_cancelled || value.code == UiCode::authentication_cancelled;
}

// Native diagnostic/tool-error wording only; never used to select an ID.
inline std::string_view ui_diagnostic_template(UiCode code) {
    switch (code) {
        case UiCode::none: return {};
#define JGCHAT_UI_MESSAGE(id, name, resource, diagnostic) case UiCode::name: return diagnostic;
#include "ui_messages.inc"
#undef JGCHAT_UI_MESSAGE
    }
    return "Unknown native message";
}
inline std::string ui_diagnostic(const UiMessage& value) {
    const auto pattern = ui_diagnostic_template(value.code);
    if (value.format == UiFormat::detail && value.args.size() == 1)
        return std::string(pattern) + ": " + value.args[0];
    if (value.format == UiFormat::range && value.args.size() == 2)
        return std::string(pattern) + ": " + value.args[0] + " to " + value.args[1];
    std::string out;
    for (std::size_t i = 0; i < pattern.size();) {
        if (i + 3 < pattern.size() && pattern[i] == '%' && pattern[i+1] >= '1' && pattern[i+1] <= '9'
                && pattern[i+2] == '$' && pattern[i+3] == 's') {
            const auto index = static_cast<std::size_t>(pattern[i+1] - '1');
            if (index < value.args.size()) out += value.args[index];
            i += 4;
        } else out += pattern[i++];
    }
    return out;
}
struct UiException { UiMessage message; };
template<class Base> class MessageException final : public Base, public UiException {
public:
    explicit MessageException(UiMessage message) : Base(ui_diagnostic(message)), UiException{std::move(message)} {}
};
using UiError = MessageException<std::runtime_error>;
using UiArgumentError = MessageException<std::invalid_argument>;

// Call only inside a catch handler. Exception matching works with Juggluco's
// -fno-rtti build; do not dynamic_cast or classify exceptions by what().
inline UiMessage current_ui_error() {
    try { throw; }
    catch (const UiException& e) { return e.message; }
    catch (const std::exception& e) { return {UiCode::external_detail, {e.what()}}; }
    catch (...) { return UiCode::chat_operation_failed; }
}
}
