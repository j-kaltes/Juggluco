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
#include "auth.hpp"
#include "tools.hpp"

namespace jgchat {
class Workspace;
// Events are small UI messages such as a tool name or a progress indication;
// never credentials or hidden reasoning.
using EventSink = std::function<void(const UiMessage&)>;
using TokenSink = std::function<void(const Tokens&)>;
class ChatClient {
public:
    ChatClient(HttpClient, Tokens, ToolHandler, TokenSink, Workspace* workspace = nullptr);
    // Includes normalized reasoning_efforts and recommended_reasoning_effort.
    // No hard-coded default model; effort choices come from account metadata.
    Json list_models(const std::atomic_bool& cancel);
    std::string ask(const std::string& question, const std::string& model,
                    const std::atomic_bool& cancel, const EventSink& event,
                    const std::string& reasoning_effort = {}, bool internet = true);
    void reset();
    Json history() const;
    Json plots() const; // Local attachments for the last successful answer.
    Json files() const; // Validated bundles, saved by the Android bridge on commit.
    Json citations() const;
    void restore_history(const Json&);
private:
    HttpClient http_;
    Tokens tokens_;
    ToolHandler tool_;
    TokenSink save_tokens_;
    Workspace* workspace_;
    Json history_ = Json::array();
    Json models_ = Json::array();
    Json plots_ = Json::array();
    Json files_ = Json::array(), citations_ = Json::array();
    HttpResponse authenticated(HttpRequest, const std::atomic_bool&);
};
}
