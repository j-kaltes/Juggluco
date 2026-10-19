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
#include "jgchat/plot.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <locale>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace jgchat {
namespace {
struct Point { uint32_t time; double value; };
Json error(const char* code, const char* message) {
    return {{"status", "error"}, {"error", {{"code", code}, {"message", message}}}};
}
std::vector<std::string_view> split(std::string_view text, char delimiter) {
    std::vector<std::string_view> out;
    for (;;) {
        const auto pos = text.find(delimiter);
        out.push_back(text.substr(0, pos));
        if (pos == std::string_view::npos) return out;
        text.remove_prefix(pos + 1);
    }
}
std::string number(double n, unsigned decimals = 1) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(decimals) << n;
    return out.str();
}
std::string local_time(uint32_t seconds, const char* format) {
    const time_t at = seconds;
    tm time{};
    char out[80]{};
    if (!localtime_r(&at, &time) || !std::strftime(out, sizeof(out), format, &time))
        throw jgchat::UiError(jgchat::UiCode::cannot_format_plot_time);
    return out;
}
std::string escape(std::string_view value) {
    std::string out;
    for (char c : value) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&apos;";
        else out += c;
    }
    return out;
}
}

Json glucose_plot(Json glucose) {
    if (glucose.value("status", "error") != "ok") return glucose;
    if (glucose.value("truncated", true))
        return error("plot_truncated", "The readings are truncated. Request a shorter interval for a complete plot.");
    try {
        const auto start = glucose.at("start").get<uint32_t>(), end = glucose.at("end").get<uint32_t>();
        const auto unit = glucose.at("unit").get<std::string>();
        const auto source = glucose.at("source").get<std::string>();
        if (end <= start || (unit != "mmol/L" && unit != "mg/dL") ||
            (source != "stream" && source != "history" && source != "scans"))
            throw jgchat::UiError(jgchat::UiCode::invalid_plot_metadata);
        const auto& tsv = glucose.at("data").get_ref<const std::string&>();
        auto lines = split(tsv, '\n');
        if (lines.empty()) throw jgchat::UiError(jgchat::UiCode::missing_plot_columns);
        auto columns = split(lines.front(), '\t');
        auto column = [&](std::string_view name) {
            const auto found = std::find(columns.begin(), columns.end(), name);
            return found == columns.end() ? columns.size() : static_cast<std::size_t>(found - columns.begin());
        };
        const auto time_column = column("UnixTime"), sensor_column = column("Sensorid");
        auto value_column = column(unit);
        if (value_column == columns.size()) value_column = column("Glucose");
        if (time_column == columns.size() || sensor_column == columns.size() || value_column == columns.size())
            throw jgchat::UiError(jgchat::UiCode::missing_plot_columns);
        std::map<std::string, std::vector<Point>> series;
        std::size_t count = 0;
        double minimum = 1e10, maximum = 0;
        for (std::size_t i = 1; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            const auto fields = split(lines[i], '\t');
            if (fields.size() != columns.size()) throw jgchat::UiError(jgchat::UiCode::invalid_plot_row);
            ++count;
            uint32_t at{};
            const auto time = fields[time_column];
            const auto parsed = std::from_chars(time.data(), time.data() + time.size(), at);
            if (parsed.ec != std::errc{} || parsed.ptr != time.data() + time.size() || at < start || at >= end)
                throw jgchat::UiError(jgchat::UiCode::invalid_plot_timestamp);
            std::istringstream input{std::string(fields[value_column])};
            input.imbue(std::locale::classic());
            double value{};
            if (!(input >> value) || !(input >> std::ws).eof() || !std::isfinite(value) || value < 0 || value > 10000)
                throw jgchat::UiError(jgchat::UiCode::invalid_plot_value);
            const auto sensor = fields[sensor_column];
            if (sensor.empty()) throw jgchat::UiError(jgchat::UiCode::invalid_plot_sensor);
            series[std::string(sensor)].push_back({at, value});
            minimum = std::min(minimum, value); maximum = std::max(maximum, value);
        }
        if (count != glucose.at("returned").get<std::size_t>()) throw jgchat::UiError(jgchat::UiCode::plot_count_mismatch);
        if (!count) return error("plot_no_data", "No glucose readings were found in this interval; no plot was created.");
        double step = unit == "mmol/L" ? 2 : 40;
        while ((maximum - minimum) / step > 7) step *= 2;
        const double low = std::max(0.0, std::floor((minimum - step * .25) / step) * step);
        const double high = std::max(low + step * 2, std::ceil((maximum + step * .25) / step) * step);
        const auto x = [&](uint32_t t) { return 85.0 + 860.0 * (double(t) - start) / (double(end) - start); };
        const auto y = [&](double v) { return 405.0 - 290.0 * (v - low) / (high - low); };
        const std::string caption = "Glucose " + local_time(start, "%Y-%m-%d %H:%M %z") + " to " +
                                    local_time(end, "%Y-%m-%d %H:%M %z");
        const double extra_legend = 25 * std::max(0.0, std::ceil(series.size() / 4.0) - 2);
        std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"jg-plot\" viewBox=\"0 0 1000 " + number(560 + extra_legend) + "\" role=\"img\">";
        svg += "<title>" + escape(caption) + "</title><style>"
            ".bg{fill:#ffffff}.grid{stroke:#d5dce3}.ink{fill:#162434}.axis{stroke:#162434}"
            ".dark .bg{fill:#17212b}.dark .grid{stroke:#435361}.dark .ink{fill:#f0f4f8}.dark .axis{stroke:#f0f4f8}"
            "text{font-family:sans-serif;font-size:17px}.small{font-size:15px}</style>"
            "<rect class=\"bg\" width=\"1000\" height=\"" + number(560 + extra_legend) + "\"/>";
        auto text = [&](double px, double py, const std::string& content, const char* anchor = "start", const char* cls = "ink") {
            svg += "<text x=\"" + number(px) + "\" y=\"" + number(py) + "\" text-anchor=\"" + anchor +
                "\" class=\"" + cls + "\">" + escape(content) + "</text>";
        };
        text(32, 32, "Glucose curve - " + unit);
        text(32, 60, caption, "start", "ink small");
        const bool calibrated = glucose.at("calibrated").get<bool>();
        const std::string calibration = calibrated ?
            (glucose.at("pastvalues").get<bool>() ? "calibrated, past values" : "calibrated") : "raw, uncalibrated";
        text(32, 85, source + "; " + calibration + "; " + std::to_string(count) + " readings", "start", "ink small");
        for (double tick = low; tick <= high + step * .01; tick += step) {
            const auto yy = number(y(tick));
            svg += "<path class=\"grid\" d=\"M85 " + yy + "H945\"/>";
            text(74, y(tick) + 6, number(tick, 0), "end");
        }
        for (unsigned i = 0; i <= 4; ++i) {
            const uint32_t t = start + static_cast<uint32_t>((uint64_t(end) - start) * i / 4);
            const auto xx = number(x(t));
            svg += "<path class=\"grid\" d=\"M" + xx + " 115V405\"/>";
            text(x(t), 433, local_time(t, end - start > 86400 ? "%m-%d %H:%M" : "%H:%M"),
                 i == 0 ? "start" : i == 4 ? "end" : "middle", "ink small");
        }
        svg += "<path class=\"axis\" fill=\"none\" d=\"M85 115V405H945\"/>";
        constexpr const char* colors[] = {"#168bce", "#d88700", "#c74fbd", "#249c78", "#dd6060", "#9380eb", "#a57c45", "#5b9ca6"};
        const uint32_t gap = source == "history" ? 1200 : 300;
        std::size_t sensor_number = 0;
        for (auto& [sensor, points] : series) {
            (void)sensor;
            std::stable_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.time < b.time; });
            const char* color = colors[sensor_number % std::size(colors)];
            std::string path;
            uint32_t previous = 0;
            bool first = true;
            for (const auto& p : points) {
                if (source != "scans") {
                    const bool joined = !first && p.time > previous && p.time - previous <= gap;
                    path += std::string(joined ? "L" : "M") + number(x(p.time)) + " " + number(y(p.value));
                }
                svg += "<circle cx=\"" + number(x(p.time)) + "\" cy=\"" + number(y(p.value)) +
                    "\" r=\"2.1\" fill=\"" + color + "\"/>";
                previous = p.time; first = false;
            }
            if (!path.empty()) svg += "<path fill=\"none\" stroke=\"" + std::string(color) + "\" stroke-width=\"2\" d=\"" + path + "\"/>";
            const double legend_x = 40 + (sensor_number % 4) * 238;
            const double legend_y = 465 + (sensor_number / 4) * 25;
            svg += "<circle cx=\"" + number(legend_x) + "\" cy=\"" + number(legend_y - 5) + "\" r=\"5\" fill=\"" + color + "\"/>";
            text(legend_x + 12, legend_y, "Sensor " + std::to_string(++sensor_number), "start", "ink small");
        }
        text(32, 527 + extra_legend, source == "scans" ? "Scan readings shown as points; no interpolation." :
            "Gaps > " + std::to_string(gap / 60) + " min and different sensors are not connected.", "start", "ink small");
        svg += "</svg>";

        glucose["plot"] = {{"svg", std::move(svg)}, {"caption", caption}, {"points", count}};
        glucose["plot_summary"] = {{"minimum", minimum}, {"maximum", maximum}, {"sensors", series.size()},
            {"gap_seconds", source == "scans" ? 0 : gap}, {"time_axis", "phone local time; UTC offsets in caption"}};
        return glucose;
    } catch (...) {
        return error("plot_invalid_data", "The exported readings could not be plotted reliably. No image was created.");
    }
}

Json xy_plot(const Json& spec, bool from_saved_table) {
    auto bad = [] { throw jgchat::UiArgumentError(jgchat::UiCode::invalid_plot_arguments_check_keys_text_limits_finite_coordinates_and_the_8_series_100); };
    auto shape = [&](const Json& value, std::initializer_list<const char*> keys) {
        if (!value.is_object() || value.size() != keys.size()) bad();
        for (const char* key : keys) if (!value.contains(key)) bad();
    };
    auto label = [&](const Json& value, const char* key) {
        const auto& item = value.at(key);
        if (!item.is_string()) bad();
        const auto text = item.get<std::string>();
        for (unsigned char c : text) {
            if (c < 32 || c == 127) bad();
        }
        return text;
    };
    shape(spec, {"title", "x_label", "y_label", "x_type", "kind", "provenance", "series"});
    const auto title = label(spec, "title"), x_label = label(spec, "x_label"),
        y_label = label(spec, "y_label"), provenance = label(spec, "provenance"),
        x_type = label(spec, "x_type"), kind = label(spec, "kind");
    if ((x_type != "number" && x_type != "time") || (kind != "line" && kind != "scatter" && kind != "bar" && kind != "band")) bad();
    const auto& series = spec.at("series");
    if (!series.is_array() || series.empty()) bad();
    std::size_t total = 0, visible = 0;
    double min_x = std::numeric_limits<double>::max(), max_x = -min_x, min_y = min_x, max_y = -min_x;
    std::vector<double> x_values;
    auto coordinate = [&](const Json& value) {
        if (!value.is_number()) bad();
        const double n = value.get<double>();
        if (!std::isfinite(n)) bad();
        return n;
    };
    for (const auto& s : series) {
        shape(s, {"label", "points"}); (void)label(s, "label");
        const auto& points = s.at("points");
        if (!points.is_array() || points.empty()) bad();
        total += points.size();
        for (const auto& p : points) {
            shape(p, {"x", "y"});
            const auto x = coordinate(p.at("x"));
            if (x_type == "time" && (x < 0 || x > UINT32_MAX)) bad();
            min_x = std::min(min_x, x); max_x = std::max(max_x, x);
            x_values.push_back(x);
            if (p.at("y").is_null()) continue;
            const auto y = coordinate(p.at("y"));
            min_y = std::min(min_y, y); max_y = std::max(max_y, y); ++visible;
        }
    }
    if (!visible) bad();
    if (kind == "band") {
        if (series.size() < 3) bad();
        const auto& center = series[0]["points"]; const auto& lo = series[1]["points"]; const auto& hi = series[2]["points"];
        if (center.size() != lo.size() || lo.size() != hi.size()) bad();
        for (std::size_t i = 0; i < center.size(); ++i) {
            if (center[i]["x"] != lo[i]["x"] || center[i]["x"] != hi[i]["x"] ||
                (i && center[i]["x"].get<double>() <= center[i-1]["x"].get<double>()) ||
                center[i]["y"].is_null() != lo[i]["y"].is_null() ||
                lo[i]["y"].is_null() != hi[i]["y"].is_null()) bad();
            if (!lo[i]["y"].is_null() && lo[i]["y"].get<double>() > hi[i]["y"].get<double>()) bad();
        }
    }
    if (kind == "bar") { min_y = std::min(min_y, 0.0); max_y = std::max(max_y, 0.0); }
    if (min_x == max_x) {
        const double pad = x_type == "time" ? 60 : std::max(1.0, std::abs(min_x) * .1);
        min_x -= pad; max_x += pad;
    } else { const double pad = (max_x - min_x) * .04; min_x -= pad; max_x += pad; }
    if (x_type == "time") { min_x = std::max(0.0, min_x); max_x = std::min(double(UINT32_MAX), max_x); }
    if (min_y == max_y) { const double pad = std::max(1.0, std::abs(min_y) * .1); min_y -= pad; max_y += pad; }
    const double raw_step = (max_y - min_y) / 5;
    if (!std::isfinite(raw_step) || raw_step <= std::numeric_limits<double>::min()) bad();
    const double power = std::pow(10.0, std::floor(std::log10(raw_step)));
    const double fraction = raw_step / power;
    const double step = (fraction <= 1 ? 1 : fraction <= 2 ? 2 : fraction <= 5 ? 5 : 10) * power;
    min_y = std::floor(min_y / step) * step; max_y = std::ceil(max_y / step) * step;
    auto x = [&](double v) { return 95 + 850 * (v - min_x) / (max_x - min_x); };
    auto y = [&](double v) { return 405 - 290 * (v - min_y) / (max_y - min_y); };
    auto tick_number = [](double v) { std::ostringstream out; out.imbue(std::locale::classic()); out << std::setprecision(6) << v; return out.str(); };
    const double extra_legend = 25 * std::max(0.0, std::ceil(series.size() / 2.0) - 4);
    std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" class=\"jg-plot\" viewBox=\"0 0 1000 " + number(660 + extra_legend) + "\" role=\"img\"><title>" + escape(title) + "</title>";
    svg += "<style>.bg{fill:#fff}.ink{fill:#162434}.grid{stroke:#d5dce3}.axis{stroke:#162434}"
        ".dark .bg{fill:#17212b}.dark .ink{fill:#f0f4f8}.dark .grid{stroke:#435361}.dark .axis{stroke:#f0f4f8}"
        "text{font:17px sans-serif}.small{font-size:15px}</style><rect class=\"bg\" width=\"1000\" height=\"" + number(660 + extra_legend) + "\"/>";
    auto text = [&](double px, double py, const std::string& content, const char* anchor = "start", const char* cls = "ink") {
        svg += "<text x=\"" + number(px) + "\" y=\"" + number(py) + "\" text-anchor=\"" + anchor + "\" class=\"" + cls + "\">" + escape(content) + "</text>";
    };
    text(32, 32, title);
    text(32, 61, std::string(from_saved_table ? "Saved-table series; " : "Model-supplied series; ") + std::to_string(visible) + " points, " + std::to_string(total - visible) + " gaps", "start", "ink small");
    text(95, 95, y_label);
    for (unsigned i = 0; i <= 10; ++i) {
        const double tick = min_y + i * step;
        if (tick > max_y + step * .001) break;
        svg += "<path class=\"grid\" d=\"M95 " + number(y(tick)) + "H945\"/>";
        text(84, y(tick) + 5, tick_number(tick), "end", "ink small");
    }
    for (unsigned i = 0; i <= 4; ++i) {
        const double tick = min_x + (max_x - min_x) * i / 4;
        svg += "<path class=\"grid\" d=\"M" + number(x(tick)) + " 115V405\"/>";
        const auto value = x_type == "time" ? local_time(static_cast<uint32_t>(tick), max_x - min_x > 86400 ? "%m-%d %H:%M" : "%H:%M") : tick_number(tick);
        text(x(tick), 430, value, i == 0 ? "start" : i == 4 ? "end" : "middle", "ink small");
    }
    text(520, 456, x_label + (x_type == "time" ? " (phone local time)" : ""), "middle", "ink small");
    svg += "<path class=\"axis\" fill=\"none\" d=\"M95 115V405H945\"/>";
    constexpr const char* colors[] = {"#168bce", "#d88700", "#c74fbd", "#249c78", "#dd6060", "#9380eb", "#a57c45", "#5b9ca6"};
    if (kind == "band") {
        const auto& lo = series[1]["points"]; const auto& hi = series[2]["points"];
        for (std::size_t start = 0; start < lo.size();) {
            if (lo[start]["y"].is_null()) { ++start; continue; }
            auto end = start+1; while (end < lo.size() && !lo[end]["y"].is_null()) ++end;
            if (end-start > 1) {
                std::string polygon;
                for (auto i = start; i < end; ++i) polygon += number(x(lo[i]["x"].get<double>())) + "," + number(y(lo[i]["y"].get<double>())) + " ";
                for (auto i = end; i-- > start;) polygon += number(x(hi[i]["x"].get<double>())) + "," + number(y(hi[i]["y"].get<double>())) + " ";
                svg += "<polygon class=\"interval-band\" fill=\"#168bce\" fill-opacity=\"0.18\" points=\"" + polygon + "\"/>";
            }
            start = end;
        }
    }
    std::sort(x_values.begin(), x_values.end());
    double spacing = max_x - min_x;
    for (std::size_t i = 1; i < x_values.size(); ++i)
        if (x_values[i] > x_values[i-1]) spacing = std::min(spacing, x_values[i] - x_values[i-1]);
    const double bar_width = std::min(60.0, .8 * 850 * spacing / (max_x - min_x)) / series.size();
    for (std::size_t i = 0; i < series.size(); ++i) {
        const auto& s = series[i]; const auto* color = colors[i % std::size(colors)];
        bool gap = true; std::string path;
        for (const auto& p : s.at("points")) {
            if (p.at("y").is_null()) { gap = true; continue; }
            const double xx = x(p.at("x").get<double>()), yy = y(p.at("y").get<double>());
            if (kind == "bar") {
                const double left = xx + (double(i) - double(series.size()) / 2) * bar_width;
                svg += "<rect x=\"" + number(left) + "\" y=\"" + number(std::min(yy, y(0))) + "\" width=\"" + number(bar_width) +
                    "\" height=\"" + number(std::abs(yy - y(0))) + "\" fill=\"" + color + "\"/>";
            } else {
                svg += "<circle cx=\"" + number(xx) + "\" cy=\"" + number(yy) + "\" r=\"2.5\" fill=\"" + color + "\"/>";
                if (kind == "line" || kind == "band") path += std::string(gap ? "M" : "L") + number(xx) + " " + number(yy);
                gap = false;
            }
        }
        if (!path.empty()) svg += "<path fill=\"none\" stroke=\"" + std::string(color) + "\" stroke-width=\"2\" d=\"" + path + "\"/>";
        const double lx = 40 + (i % 2) * 480, ly = 490 + (i / 2) * 25;
        svg += "<circle cx=\"" + number(lx) + "\" cy=\"" + number(ly - 5) + "\" r=\"5\" fill=\"" + color + "\"/>";
        text(lx + 12, ly, s.at("label").get<std::string>(), "start", "ink small");
    }
    // Wrap the provenance at a UTF-8 boundary without accepting markup.
    auto note = provenance;
    std::size_t cut = std::min<std::size_t>(90, note.size());
    while (cut < note.size() && (static_cast<unsigned char>(note[cut]) & 0xc0) == 0x80) --cut;
    if (cut < note.size()) { const auto space = note.rfind(' ', cut); if (space != std::string::npos && space > 40) cut = space; }
    text(32, 615 + extra_legend, note.substr(0, cut), "start", "ink small");
    if (cut < note.size()) text(32, 640 + extra_legend, note.substr(cut), "start", "ink small");
    svg += "</svg>";

    return {{"status", "ok"}, {"kind", kind}, {"series", series.size()}, {"points", visible}, {"gaps", total - visible},
        {"plot", {{"svg", std::move(svg)}, {"caption", title}, {"points", visible}}}};
}
}
