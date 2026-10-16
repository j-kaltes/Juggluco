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
#include "jgchat/chat_export.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <stdexcept>
#include <unistd.h>
#include <vector>

namespace jgchat {
namespace {
constexpr std::size_t max_export = 8 * 1024 * 1024;
std::string field(const Json& object, const char* key) {
    auto at = object.find(key);
    return at != object.end() && at->is_string() ? at->get<std::string>() : "";
}
const Json& array(const Json& object, const char* key) {
    static const Json empty = Json::array();
    auto at = object.find(key);
    return at != object.end() && at->is_array() ? *at : empty;
}
std::string escape(std::string_view value) {
    std::string out;
    for (char c : value) switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c;
    }
    return out;
}
bool web_url(std::string_view value) {
    return (value.starts_with("https://") || value.starts_with("http://")) &&
        value.find_first_of("\r\n\t ") == std::string_view::npos &&
        std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
std::string base64(std::string_view value) {
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((value.size() + 2) / 3 * 4);
    for (std::size_t i = 0; i < value.size(); i += 3) {
        unsigned bits = static_cast<unsigned char>(value[i]) << 16;
        if (i + 1 < value.size()) bits |= static_cast<unsigned char>(value[i + 1]) << 8;
        if (i + 2 < value.size()) bits |= static_cast<unsigned char>(value[i + 2]);
        out += alphabet[bits >> 18]; out += alphabet[(bits >> 12) & 63];
        out += i + 1 < value.size() ? alphabet[(bits >> 6) & 63] : '=';
        out += i + 2 < value.size() ? alphabet[bits & 63] : '=';
    }
    return out;
}
std::string_view trim(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
    return text;
}
std::vector<std::string_view> split(std::string_view text, char separator) {
    std::vector<std::string_view> out;
    for (;;) {
        auto at = text.find(separator);
        out.push_back(text.substr(0, at));
        if (at == std::string_view::npos) return out;
        text.remove_prefix(at + 1);
    }
}
// A deliberately small formatting subset, with literal fallback. All text is
// escaped, including model-authored HTML. Nothing here evaluates model code.
std::string inline_html(std::string_view text, unsigned depth = 0) {
    if (depth > 8) return escape(text);
    std::string out;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '\\' && i + 1 < text.size() && std::ispunct(static_cast<unsigned char>(text[i + 1]))) {
            out += escape(text.substr(i + 1, 1)); i += 2; continue;
        }
        if (text[i] == '`') {
            auto end = text.find('`', i + 1);
            if (end != std::string_view::npos) {
                out += "<code>" + escape(text.substr(i + 1, end - i - 1)) + "</code>";
                i = end + 1; continue;
            }
        }
        if (text[i] == '[') {
            auto label = text.find("](", i + 1);
            auto end = label == std::string_view::npos ? label : text.find(')', label + 2);
            if (end != std::string_view::npos && web_url(text.substr(label + 2, end - label - 2))) {
                out += "<a rel=\"noreferrer noopener\" href=\"" + escape(text.substr(label + 2, end - label - 2)) + "\">";
                out += escape(text.substr(i + 1, label - i - 1)) + "</a>";
                i = end + 1; continue;
            }
        }
        bool marked = false;
        for (const std::string_view marker : {"***", "**", "__", "*", "_"}) {
            if (!text.substr(i).starts_with(marker)) continue;
            if (marker.front() == '_' && i && std::isalnum(static_cast<unsigned char>(text[i - 1]))) continue;
            auto begin = i + marker.size(), end = text.find(marker, begin);
            if (end == std::string_view::npos || end == begin || text[begin] == ' ' || text[end - 1] == ' ') continue;
            const char* open = marker.size() == 3 ? "<strong><em>" : marker.size() == 2 ? "<strong>" : "<em>";
            const char* close = marker.size() == 3 ? "</em></strong>" : marker.size() == 2 ? "</strong>" : "</em>";
            out += open; out += inline_html(text.substr(begin, end - begin), depth + 1); out += close;
            i = end + marker.size(); marked = true; break;
        }
        if (!marked) out += escape(text.substr(i++, 1));
    }
    return out;
}
std::vector<std::string_view> cells(std::string_view line) {
    line = trim(line);
    if (line.starts_with('|')) line.remove_prefix(1);
    if (line.ends_with('|')) line.remove_suffix(1);
    auto result = split(line, '|');
    for (auto& cell : result) cell = trim(cell);
    return result;
}
bool table_rule(std::string_view line, std::size_t columns) {
    if (line.find('|') == std::string_view::npos) return false;
    const auto parts = cells(line);
    if (parts.size() != columns) return false;
    for (auto part : parts) {
        if (part.starts_with(':')) part.remove_prefix(1);
        if (part.ends_with(':')) part.remove_suffix(1);
        if (part.size() < 3 || part.find_first_not_of('-') != std::string_view::npos) return false;
    }
    return true;
}
std::string markdown_html(std::string_view source) {
    const auto lines = split(source, '\n');
    std::string out;
    bool code = false;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const auto line = trim(lines[i]);
        if (line.starts_with("```")) { out += code ? "</code></pre>\n" : "<pre><code>"; code = !code; continue; }
        if (code) { out += escape(lines[i]) + "\n"; continue; }
        if (line.empty()) { out += "<br>\n"; continue; }
        if (i + 1 < lines.size() && line.find('|') != std::string_view::npos && table_rule(lines[i + 1], cells(line).size())) {
            auto row = [&](std::string_view value, const char* tag) {
                out += "<tr>";
                for (auto cell : cells(value)) out += "<" + std::string(tag) + ">" + inline_html(cell) + "</" + tag + ">";
                out += "</tr>\n";
            };
            out += "<div class=\"table\"><table><thead>"; row(line, "th");
            out += "</thead><tbody>"; ++i;
            while (i + 1 < lines.size() && lines[i + 1].find('|') != std::string_view::npos && !trim(lines[i + 1]).empty()) row(lines[++i], "td");
            out += "</tbody></table></div>\n"; continue;
        }
        std::size_t level = 0;
        while (level < line.size() && line[level] == '#') ++level;
        if (level && level <= 6 && level < line.size() && line[level] == ' ') {
            auto tag = "h" + std::to_string(std::min<std::size_t>(6, level + 2));
            out += "<" + tag + ">" + inline_html(trim(line.substr(level))) + "</" + tag + ">\n";
        } else if (line.starts_with("> ")) out += "<blockquote>" + inline_html(line.substr(2)) + "</blockquote>\n";
        else if (line.starts_with("- ") || line.starts_with("* ") || line.starts_with("+ "))
            out += "<div class=\"bullet\">&#8226; " + inline_html(line.substr(2)) + "</div>\n";
        else out += "<div>" + inline_html(lines[i]) + "</div>\n";
    }
    if (code) out += "</code></pre>\n";
    return out;
}
std::string translated(const Json& labels, const std::string& key, const char* fallback) {
    auto value = field(labels, key.c_str());
    return value.empty() ? fallback : value;
}
std::string heading(const Json& message, const Json& labels) {
    if (field(message, "role") == "user") return translated(labels, "you", "You");
    auto name = field(message, "model_name");
    if (trim(name).empty()) name = field(message, "model_id");
    name = trim(name).empty() ? translated(labels, "assistant", "ChatGPT") : std::string(trim(name));
    const auto effort = message.find("reasoning_effort");
    if (effort == message.end() || !effort->is_string()) return name;
    auto level = effort->get<std::string>();
    if (level.empty()) level = "default";
    const char* suffix = level == "none" ? "N" : level == "minimal" ? "Min" : level == "low" ? "L" :
        level == "medium" ? "M" : level == "high" ? "H" : level == "xhigh" ? "XH" :
        level == "max" ? "Max" : level == "default" ? "D" : nullptr;
    if (!suffix) return name;
    auto pattern = translated(labels, "answer_heading", "%1$s · %2$s");
    // Substitute only the template, never recursively reinterpret model names.
    std::string result;
    for (std::size_t i = 0; i < pattern.size();) {
        if (pattern.compare(i, 4, "%1$s") == 0) { result += name; i += 4; }
        else if (pattern.compare(i, 4, "%2$s") == 0) { result += translated(labels, "effort_" + level, suffix); i += 4; }
        else result += pattern[i++];
    }
    return result;
}
}

std::string render_chat_export(const Json& messages, std::string_view pending, bool busy,
        std::string_view format, int message_index, int plot_index, const Json& labels) {
    if (!messages.is_array()) throw jgchat::UiError(jgchat::UiCode::no_chat_is_available_to_export);
    if (format == "svg") {
        if (message_index < 0 || static_cast<std::size_t>(message_index) >= messages.size() || plot_index < 0)
            throw jgchat::UiError(jgchat::UiCode::the_plot_is_unavailable);
        const auto& plots = array(messages[message_index], "plots");
        if (static_cast<std::size_t>(plot_index) >= plots.size()) throw jgchat::UiError(jgchat::UiCode::the_plot_is_unavailable);
        auto svg = field(plots[plot_index], "svg");
        if (!svg.starts_with("<svg ") || svg.size() > 131072) throw jgchat::UiError(jgchat::UiCode::the_plot_is_unavailable);
        return svg;
    }
    if (format != "html" && format != "txt") throw jgchat::UiError(jgchat::UiCode::unsupported_chat_export_format);
    if (messages.empty() && pending.empty()) throw jgchat::UiError(jgchat::UiCode::there_are_no_messages_to_export);
    const bool html = format == "html";
    const auto label = [&](const char* key, const char* fallback) { return translated(labels, key, fallback); };
    const auto title = label("title", "Juggluco chat");
    std::string out = html ? "<!doctype html>\n<html lang=\"" + escape(label("language", "en")) + R"HTML("><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="referrer" content="no-referrer">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src data:; style-src 'unsafe-inline'; base-uri 'none'; form-action 'none'">
<title>)HTML" + escape(title) + R"HTML(</title><style>
body{font:17px/1.5 system-ui,sans-serif;background:#fff;color:#202124;max-width:960px;margin:auto;padding:20px;overflow-wrap:anywhere}
h1{font-size:1.5em}h2{font-size:1.05em}h3,h4,h5,h6{font-size:1em;margin:.7em 0}.message{border-top:1px solid #ccc;margin-top:1.5em;padding-top:.5em}
.user{background:#f4f4f4;padding:.5em 1em}.literal,pre{white-space:pre-wrap}.note,figcaption{color:#50545b;font-size:.9em}
figure{margin:1em 0}img{width:100%;height:auto}pre{padding:12px;background:#f4f4f4;overflow:auto}code{font-family:monospace}
blockquote{border-left:3px solid #aaa;margin:.5em 0;padding-left:1em}.bullet{padding-left:1em}.table{overflow-x:auto}
table{border-collapse:collapse;margin:.7em 0}th,td{border:1px solid #bbb;text-align:left;padding:.4em .7em}a{color:#164b9b}
@media print{body{max-width:none;padding:0}.message,figure{break-inside:avoid}}
</style></head><body><h1>)HTML" + escape(title) + "</h1>\n" : title + "\n=============\n";
    for (const auto& message : messages) {
        const auto role = field(message, "role");
        if (role != "user" && role != "assistant") continue;
        const auto text = field(message, "text");
        if (html) {
            out += "<section class=\"message " + std::string(role == "user" ? "user" : "answer") + "\"><h2>" + escape(heading(message, labels)) + "</h2>\n";
            out += role == "user" ? "<div class=\"literal\">" + escape(text) + "</div>\n" : markdown_html(text);
        } else out += "\n" + heading(message, labels) + "\n" + text + "\n";
        for (const auto& citation : array(message, "citations")) {
            const auto url = field(citation, "url");
            if (!web_url(url)) continue;
            const auto title = field(citation, "title");
            if (html) out += "<p class=\"note\">" + escape(label("source", "Source")) + ": <a rel=\"noreferrer noopener\" href=\"" + escape(url) + "\">" + escape(title.empty() ? url : title) + "</a> — " + escape(url) + "</p>\n";
            else out += label("source", "Source") + ": " + (title.empty() ? "" : title + " — ") + url + "\n";
        }
        for (const auto& plot : array(message, "plots")) {
            auto caption = field(plot, "caption");
            if (caption.empty()) caption = label("plot", "Plot");
            const auto svg = field(plot, "svg");
            if (html && svg.starts_with("<svg ") && svg.size() <= 131072)
                out += "<figure><img alt=\"" + escape(caption) + "\" src=\"data:image/svg+xml;base64," + base64(svg) + "\"><figcaption>" + escape(caption) + "</figcaption></figure>\n";
            else out += html ? "<p>" + escape(caption) + " (" + escape(label("plot_unavailable", "plot unavailable")) + ")</p>\n"
                : label("plot", "Plot") + ": " + caption + " (" + label("plot_text_note", "image included in HTML export") + ")\n";
        }
        for (const auto& bundle : array(message, "files")) {
            std::string names;
            for (const auto& file : array(bundle, "files")) {
                if (!names.empty()) names += ", ";
                names += field(file, "name");
            }
            const auto note = label("generated_files", "Generated files") + ": " + field(bundle, "title") + (names.empty() ? "" : " (" + names + ")") +
                ". " + label("separate_contents", "Contents are separate; save them using Saved files in Juggluco.");
            out += html ? "<p class=\"note\">" + escape(note) + "</p>\n" : note + "\n";
        }
        if (html) out += "</section>\n";
        if (out.size() > max_export) throw jgchat::UiError(jgchat::UiCode::the_chat_export_is_too_large);
    }
    if (!pending.empty()) {
        const auto note = busy ? label("pending", "Answer in progress when exported; no completed answer yet.")
                               : label("incomplete", "No completed answer for this question.");
        const auto you = label("you", "You");
        out += html ? "<section class=\"message user\"><h2>" + escape(you) + "</h2><div class=\"literal\">" + escape(pending) + "</div><p class=\"note\">" + escape(note) + "</p></section>\n"
                    : "\n" + you + "\n" + std::string(pending) + "\n[" + note + "]\n";
    }
    if (html) out += "</body></html>\n";
    if (out.size() > max_export) throw jgchat::UiError(jgchat::UiCode::the_chat_export_is_too_large);
    return out;
}

std::size_t write_chat_export(int output, std::string_view contents) {
    if (output < 0 || contents.size() > max_export) throw jgchat::UiError(jgchat::UiCode::cannot_write_the_chat_export);
    std::size_t done = 0;
    while (done < contents.size()) {
        const auto count = ::write(output, contents.data() + done, contents.size() - done);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) throw jgchat::UiError(jgchat::UiCode::could_not_write_the_complete_chat_export);
        done += static_cast<std::size_t>(count);
    }
    return done;
}
std::size_t copy_chat_export(int input, int output) {
    if (input < 0 || output < 0 || input == output) throw jgchat::UiError(jgchat::UiCode::invalid_chat_export_descriptor);
    std::array<char, 16384> buffer{};
    std::size_t total = 0;
    for (;;) {
        const auto count = ::read(input, buffer.data(), buffer.size());
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) throw jgchat::UiError(jgchat::UiCode::cannot_read_the_chat_export);
        if (!count) return total;
        total += static_cast<std::size_t>(count);
        if (total > max_export) throw jgchat::UiError(jgchat::UiCode::the_chat_export_is_too_large);
        write_chat_export(output, {buffer.data(), static_cast<std::size_t>(count)});
    }
}
}
