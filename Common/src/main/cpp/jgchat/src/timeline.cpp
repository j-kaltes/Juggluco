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
#include "jgchat/timeline.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <ctime>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace jgchat {
namespace {
struct Reading { uint32_t at; double value; };
std::vector<std::string_view> split(std::string_view text, char delimiter) {
    std::vector<std::string_view> out;
    while (true) {
        auto end = text.find(delimiter); out.push_back(text.substr(0, end));
        if (end == std::string_view::npos) break;
        text.remove_prefix(end + 1);
    }
    return out;
}
std::string local_time(uint32_t at) {
    const time_t stamp = at; tm time{}; char result[64]{};
    if (!localtime_r(&stamp, &time) || !strftime(result, sizeof(result), "%Y-%m-%dT%H:%M:%S%z", &time))
        throw jgchat::UiError(jgchat::UiCode::cannot_format_timeline_time);
    return result;
}
void check(const std::atomic_bool* cancel) { if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled); }
struct Summary {
    unsigned count{}, max_gap{};
    Reading first{}, last{}, min{}, max{};
    double sum{};
    void add(Reading r) {
        if (!count) { first = min = max = r; }
        else max_gap = std::max(max_gap, r.at - last.at);
        if (r.value < min.value) min = r;
        if (r.value > max.value) max = r;
        last = r; sum += r.value; ++count;
    }
    Json json(const std::string& sensor) const {
        return {{"sensor_id", sensor}, {"count", count}, {"first_at", first.at}, {"first", first.value},
            {"last_at", last.at}, {"last", last.value}, {"minimum", min.value}, {"minimum_at", min.at},
            {"maximum", max.value}, {"maximum_at", max.at}, {"mean", sum / count},
            {"largest_between_readings_gap_seconds", max_gap}};
    }
};
}
Json make_stream_gaps(const Json& glucose, uint32_t minimum, const std::atomic_bool* cancel) {
    check(cancel);
    if (glucose.value("status", "error") != "ok") return glucose;
    if (glucose.value("truncated", true))
        return {{"status", "error"}, {"error", {{"code", "stream_analysis_truncated"},
            {"message", "Stream export was truncated. Shorten the interval; no partial gap analysis was returned."}}}};
    if (glucose.at("source") != "stream" || minimum < 60 || minimum > 86400)
        throw jgchat::UiError(jgchat::UiCode::stream_analysis_requires_stream_data_and_a_1_1440_minute_threshold);
    const auto start = glucose.at("start").get<uint32_t>(), end = glucose.at("end").get<uint32_t>();
    if (end <= start || end - start > 7U * 86400) throw jgchat::UiError(jgchat::UiCode::invalid_stream_analysis_bounds);
    const auto& tsv = glucose.at("data").get_ref<const std::string&>();
    auto lines = split(tsv, '\n'), columns = split(lines.front(), '\t');
    auto column = [&](std::string_view name) { return std::find(columns.begin(), columns.end(), name) - columns.begin(); };
    const auto time_col = column("UnixTime"), sensor_col = column("Sensorid"), rate_col = column("Rate");
    if (time_col == int(columns.size()) || sensor_col == int(columns.size()) || rate_col == int(columns.size()))
        return {{"status", "error"}, {"error", {{"code", "stream_columns_missing"},
            {"message", "Stream analysis needs UnixTime, Sensorid and Rate columns; history cannot substitute for stream."}}}};
    struct Sample { uint32_t at; bool rate; };
    std::map<std::string, std::vector<Sample>> sensors;
    unsigned count = 0;
    for (std::size_t row = 1; row < lines.size(); ++row) {
        check(cancel); if (lines[row].empty()) continue;
        const auto fields = split(lines[row], '\t');
        if (fields.size() != columns.size() || ++count > 20000) throw jgchat::UiError(jgchat::UiCode::invalid_stream_row_count);
        uint32_t at{}; const auto field = fields[time_col];
        auto parsed = std::from_chars(field.data(), field.data() + field.size(), at);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size() || at < start || at >= end)
            throw jgchat::UiError(jgchat::UiCode::invalid_stream_timestamp);
        const auto rate = fields[rate_col];
        const bool missing = rate == "nan" || rate == "+nan" || rate == "-nan" ||
            rate == "NaN" || rate == "+NaN" || rate == "-NaN";
        if (!missing) {
            std::istringstream input{std::string(rate)}; input.imbue(std::locale::classic()); double value{};
            if (!(input >> value) || !(input >> std::ws).eof() || !std::isfinite(value))
                throw jgchat::UiError(jgchat::UiCode::invalid_stream_rate_unavailable_rate_must_be_a_native_nan_token);
        }
        const auto id = fields[sensor_col];
        if (id.empty() || id.size() > 128) throw jgchat::UiError(jgchat::UiCode::invalid_stream_sensor_id);
        sensors[std::string(id)].push_back({at, !missing});
    }
    if (count != glucose.at("returned").get<unsigned>() || sensors.size() > 8)
        throw jgchat::UiError(jgchat::UiCode::invalid_stream_count_or_more_than_eight_sensors_shorten_the_interval);
    Json intervals = Json::array(), summaries = Json::array();
    auto add = [&](Json value) {
        if (intervals.size() >= 200) throw jgchat::UiError(jgchat::UiCode::more_than_200_stream_intervals_shorten_the_window_no_partial_analysis_returned);
        intervals.push_back(std::move(value));
    };
    for (auto& [id, samples] : sensors) {
        std::sort(samples.begin(), samples.end(), [](auto a, auto b) { return a.at < b.at; });
        std::vector<Sample> values;
        for (auto sample : samples) {
            if (!values.empty() && values.back().at == sample.at) values.back().rate |= sample.rate;
            else values.push_back(sample);
        }
        // Never infer cadence from the largest observed spacing: that would
        // turn an outage into a supposed normal sampling interval. Use the
        // exact sensor's native metadata, and leave it unknown on ambiguity.
        const Json* metadata = nullptr;
        unsigned matches = 0;
        if (glucose.contains("sensor_metadata")) for (const auto& item : glucose.at("sensor_metadata")) {
            if (item.value("sensor_id", "") == id) { metadata = &item; ++matches; }
        }
        if (matches != 1 || metadata->value("status", "unavailable") != "ok") metadata = nullptr;
        uint32_t cadence = 0;
        if (metadata && metadata->contains("stream_interval_seconds") &&
            (*metadata)["stream_interval_seconds"].is_number_integer()) {
            const auto seconds = (*metadata)["stream_interval_seconds"].get<int64_t>();
            if (seconds > 0 && seconds <= 86400) cadence = static_cast<uint32_t>(seconds);
        }
        const bool libre3 = metadata && metadata->contains("libre_model") && (*metadata)["libre_model"] == 3;
        // A half-cadence allowance avoids labelling ordinary timestamp jitter
        // as missing glucose. It is an analysis heuristic, not a device event.
        auto breaks_cadence = [&](uint32_t elapsed) {
            return cadence ? elapsed > cadence + cadence / 2 : elapsed >= minimum;
        };
        auto event_id = [&](const char* kind, uint32_t first, uint32_t last) {
            return std::string(kind) + ":" + id + ":" + std::to_string(first) + ":" + std::to_string(last);
        };
        auto between = [&](std::size_t i) {
            const auto previous = values[i - 1].at, next = values[i].at, elapsed = next - previous;
            const bool gap = breaks_cadence(elapsed);
            const char* kind = cadence ? (gap ? "no_stream_records" : "normal_sampling_interval") :
                "sampling_interval_cadence_unknown";
            return Json{{"sensor_id", id}, {"kind", kind}, {"event_id", event_id(kind, previous, next)},
                {"previous_at", previous}, {"next_at", next}, {"elapsed_seconds", elapsed},
                {"local_previous", local_time(previous)}, {"local_next", local_time(next)},
                {"glucose_records_between", 0}, {"meets_threshold", gap && elapsed >= minimum},
                {"estimated_missing_samples", cadence ? Json(gap ? (elapsed + cadence / 2) / cadence - 1 : 0) : Json(nullptr)},
                {"sensor_error_assessment", cadence && !gap ? "no_missing_glucose_evidence" : "missing_glucose_cause_undetermined"}};
        };
        unsigned missing = 0; uint32_t largest_interval = 0;
        Json largest_spacing = nullptr, largest_glucose_gap = nullptr;
        for (std::size_t i = 0; i < values.size(); ++i) {
            check(cancel);
            if (!values[i].rate) ++missing;
            if (i) {
                const auto elapsed = values[i].at - values[i - 1].at;
                if (elapsed >= largest_interval) { largest_interval = elapsed; largest_spacing = between(i); }
                if (breaks_cadence(elapsed)) {
                    auto gap = between(i);
                    if (cadence && (largest_glucose_gap.is_null() || elapsed >= largest_glucose_gap["elapsed_seconds"].get<uint32_t>()))
                        largest_glucose_gap = gap;
                    if (elapsed >= minimum) add(std::move(gap));
                }
            }
        }
        for (std::size_t i = 0; i < values.size();) {
            if (values[i].rate) { ++i; continue; }
            const auto first = i++;
            // Two NaN samples separated by missing glucose are not one
            // continuous backfill run. Keep the absence as a separate event.
            while (i < values.size() && !values[i].rate && !breaks_cadence(values[i].at - values[i - 1].at)) ++i;
            const auto last = i - 1;
            const bool gap_before = first && breaks_cadence(values[first].at - values[first - 1].at);
            const bool gap_after = i < values.size() && breaks_cadence(values[i].at - values[last].at);
            const bool has_previous = first && !gap_before && values[first - 1].rate;
            const bool has_next = i < values.size() && !gap_after && values[i].rate;
            const uint32_t previous = has_previous ? values[first - 1].at : 0, next = has_next ? values[i].at : 0;
            const auto span = values[last].at - values[first].at;
            const bool bracket_meets = has_previous && has_next && next - previous >= minimum;
            if (span >= minimum || bracket_meets || first == 0 || i == values.size()) add({
                {"sensor_id", id}, {"kind", "no_finite_rate"},
                {"event_id", event_id("no_finite_rate", values[first].at, values[last].at)},
                {"observation", "glucose_present_rate_unavailable"}, {"glucose_records_present", i - first},
                {"sensor_error_assessment", "not_supported_by_rate_absence"},
                {"connection_assessment", libre3 ? "consistent_with_backfill_not_an_exact_connection_log" : "not_classified_for_this_sensor_family"},
                {"first_no_rate_at", values[first].at}, {"last_no_rate_at", values[last].at},
                {"local_first_no_rate", local_time(values[first].at)}, {"local_last_no_rate", local_time(values[last].at)},
                {"no_rate_records", i - first}, {"observed_no_rate_span_seconds", span},
                {"previous_finite_rate_at", has_previous ? Json(previous) : Json(nullptr)},
                {"next_finite_rate_at", has_next ? Json(next) : Json(nullptr)},
                {"bracket_elapsed_seconds", has_previous && has_next ? Json(next - previous) : Json(nullptr)},
                {"meets_threshold", span >= minimum}, {"bracket_meets_threshold", bracket_meets},
                {"threshold_assessment", span >= minimum ? "observed_span_meets_minimum" : bracket_meets ?
                    "only_bracket_reaches_minimum_duration_uncertain" : "below_minimum_open_edge"},
                {"open_before", first == 0}, {"open_after", i == values.size()},
                {"glucose_gap_before", gap_before}, {"glucose_gap_after", gap_after}});
        }
        summaries.push_back({{"sensor_id", id}, {"exported_records", samples.size()}, {"unique_timestamps", values.size()},
            {"finite_rate_records", values.size() - missing}, {"no_rate_records", missing},
            {"expected_stream_interval_seconds", cadence ? Json(cadence) : Json(nullptr)},
            {"cadence_source", cadence ? "native_sensor_metadata" : "unknown"},
            {"first_at", values.front().at}, {"last_at", values.back().at},
            {"unobserved_before_first_seconds", values.front().at - start},
            {"unobserved_after_last_seconds", end - values.back().at},
            {"largest_between_records_interval_seconds", largest_interval},
            {"largest_between_records_interval", std::move(largest_spacing)},
            {"largest_glucose_gap", std::move(largest_glucose_gap)}});
    }
    Json result{{"status", "ok"}, {"schema_version", 2}, {"source", "stream"}, {"start", start}, {"end", end},
        {"local_start", local_time(start)}, {"local_end", local_time(end)}, {"duration_seconds", end - start}, {"end_exclusive", true},
        {"minimum_seconds", minimum}, {"exported_records", count}, {"truncated", false},
        {"coverage", count ? "complete_requested_export" : "no_records"},
        {"sensor_summaries", std::move(summaries)}, {"intervals", std::move(intervals)},
        {"historical_sensor_error_codes_available", false},
        {"search_scope", "Only the requested time window and exported sensors were analyzed. Sensor metadata lookup across the entire index does not mean all historical glucose was analyzed. Seven days is a per-call limit; continue backward if needed. No records in this window does not establish absence of events elsewhere."},
        {"method", "Complete valid-glucose export, per sensor, sorted by timestamp; finite rate wins duplicate timestamps and zero rate is valid. A glucose gap exceeds 1.5 times the native expected stream interval (jitter heuristic). Unknown/ambiguous metadata leaves cadence unknown. Largest glucose gaps include timestamps even below the requested threshold. No-rate runs split at glucose gaps. Open window edges are not confirmed gaps; query adjacent windows."},
        {"interpretation", "no_finite_rate means glucose IS PRESENT; unavailable Rate is not unavailable glucose or evidence of a sensor error. In Libre 3 a sustained run is consistent with backfilled readings after interrupted live reception. Isolated unavailable rates are insufficient evidence. no_stream_records means no valid glucose BETWEEN the given endpoints; sensor unavailability is possible but connection loss without recovered data or incomplete local storage can also explain it. The valid-glucose export cannot confirm historical sensor-error codes or exact Bluetooth event times. Normal sampling intervals, including 60-62 seconds at a 60-second cadence, are NOT missing minutes. For 'when was that gap', use that event's ID and endpoints, never another no-rate run. Duration is the observed no-rate span or spacing between glucose endpoints; the finite-rate bracket is separately labelled and cannot turn a shorter observed span into a confirmed minimum-duration event."}};
    for (const auto* key : {"sensor_metadata", "missing_sensor_metadata_ids", "unavailable_sensor_metadata_ids",
                            "ambiguous_sensor_metadata_ids", "sensor_metadata_lookup"})
        if (glucose.contains(key)) result[key] = glucose.at(key);
    return result;
}

Json make_timeline(const Json& glucose, const Json& amounts, const Json& context,
                   uint32_t step, const IobReader& iob, const std::atomic_bool* cancel) {
    check(cancel);
    for (const auto* part : {&glucose, &amounts, &context})
        if (part->value("status", "error") != "ok") return *part;
    if (glucose.value("truncated", true) || amounts.value("truncated", true))
        return {{"status", "error"}, {"error", {{"code", "timeline_truncated"},
            {"message", "Too many readings or entered amounts. Shorten the interval; no partial timeline was returned."}}}};
    const auto start = glucose.at("start").get<uint32_t>(), end = glucose.at("end").get<uint32_t>();
    if (!step || end <= start || (uint64_t(end) - start + step - 1) / step > 672 ||
        amounts.at("start") != start || amounts.at("end") != end)
        throw jgchat::UiError(jgchat::UiCode::invalid_timeline_bounds);
    const auto unit = glucose.at("unit").get<std::string>();
    if (unit != "mmol/L" && unit != "mg/dL") throw jgchat::UiError(jgchat::UiCode::invalid_timeline_unit);
    const auto& tsv = glucose.at("data").get_ref<const std::string&>();
    auto lines = split(tsv, '\n'), columns = split(lines.front(), '\t');
    auto column = [&](std::string_view name) {
        return static_cast<std::size_t>(std::find(columns.begin(), columns.end(), name) - columns.begin());
    };
    const auto time_col = column("UnixTime"), sensor_col = column("Sensorid");
    auto value_col = column(unit); if (value_col == columns.size()) value_col = column("Glucose");
    if (time_col == columns.size() || sensor_col == columns.size() || value_col == columns.size())
        throw jgchat::UiError(jgchat::UiCode::missing_timeline_glucose_columns);
    std::map<std::string, std::vector<Reading>> sensors;
    unsigned count = 0;
    for (std::size_t i = 1; i < lines.size(); ++i) {
        check(cancel); if (lines[i].empty()) continue;
        const auto fields = split(lines[i], '\t');
        if (fields.size() != columns.size() || ++count > 20000) throw jgchat::UiError(jgchat::UiCode::invalid_timeline_row_count);
        uint32_t at{}; const auto field = fields[time_col];
        auto parsed = std::from_chars(field.data(), field.data() + field.size(), at);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size() || at < start || at >= end)
            throw jgchat::UiError(jgchat::UiCode::invalid_timeline_timestamp);
        std::istringstream value{std::string(fields[value_col])}; value.imbue(std::locale::classic());
        double v{};
        if (!(value >> v) || !(value >> std::ws).eof() || !std::isfinite(v) || v < 0 || v > 10000)
            throw jgchat::UiError(jgchat::UiCode::invalid_timeline_glucose_value);
        if (fields[sensor_col].empty() || fields[sensor_col].size() > 128) throw jgchat::UiError(jgchat::UiCode::invalid_timeline_sensor);
        sensors[std::string(fields[sensor_col])].push_back({at, v});
    }
    if (count != glucose.at("returned").get<unsigned>() || sensors.size() > 8)
        throw jgchat::UiError(jgchat::UiCode::invalid_timeline_count_or_more_than_eight_sensors_shorten_the_interval);
    const auto bins = (uint64_t(end) - start + step - 1) / step;
    std::vector<Json> glucose_bins(bins, Json::array()), amount_bins(bins, Json::array());
    Json summaries = Json::array();
    for (auto& [id, readings] : sensors) {
        std::stable_sort(readings.begin(), readings.end(), [](auto a, auto b) { return a.at < b.at; });
        std::map<std::size_t, Summary> groups; Summary all;
        for (auto r : readings) { groups[(r.at - start) / step].add(r); all.add(r); }
        for (const auto& [bin, summary] : groups) glucose_bins[bin].push_back(summary.json(id));
        auto summary = all.json(id);
        summary["unobserved_before_first_seconds"] = all.first.at - start;
        summary["unobserved_after_last_seconds"] = end - all.last.at;
        summaries.push_back(std::move(summary));
    }
    const auto& entries = amounts.at("records");
    if (!entries.is_array() || entries.size() > 1000 || entries.size() != amounts.at("returned").get<std::size_t>())
        throw jgchat::UiError(jgchat::UiCode::invalid_timeline_amount_count);
    for (auto entry : entries) {
        check(cancel);
        const auto at = entry.at("time").get<uint32_t>();
        if (at < start || at >= end) throw jgchat::UiError(jgchat::UiCode::amount_outside_timeline_bounds);
        entry["local_time"] = local_time(at);
        amount_bins[(at - start) / step].push_back(std::move(entry));
    }
    Json timeline = Json::array();
    for (uint64_t at = start, index = 0; at < end; at += step, ++index) {
        check(cancel);
        const auto sample = iob(static_cast<uint32_t>(at));
        if (sample.value("status", "error") != "ok") return sample;
        const auto& value = sample.at("iob");
        timeline.push_back({{"start", at}, {"end", std::min<uint64_t>(end, at + step)}, {"local_start", local_time(at)},
            {"glucose", glucose_bins[index]}, {"amounts", amount_bins[index]},
            {"iob_at_start", {{"value", value.at("value")}, {"status", value.at("status")}}}});
    }
    return {{"status", "ok"}, {"schema", 1}, {"format", "aligned_timeline"}, {"computed_by", "Juggluco C++ client"},
        {"generated_at", std::time(nullptr)}, {"start", start}, {"end", end}, {"end_exclusive", true},
        {"step_seconds", step}, {"unit", unit}, {"source", glucose.at("source")},
        {"calibrated", glucose.at("calibrated")}, {"pastvalues", glucose.at("pastvalues")},
        {"glucose_record_count", count}, {"amount_record_count", entries.size()}, {"truncated", false},
        {"sensor_summaries", std::move(summaries)}, {"labels", context.at("labels")}, {"iob_unit", "U"},
        {"timeline", std::move(timeline)},
        {"method", "All returned glucose observations are grouped independently per sensor into [start,end) bins; arithmetic measurement means and actual extrema, never interpolation. Empty glucose arrays mean no observations, not zero. Largest gap measures adjacent observations only; counts and edge gaps are not time-in-range or complete-coverage percentages. Use native statistics for those summaries and raw glucose to inspect individual readings."},
        {"timing", "Amounts retain exact recorded timestamps and local offsets; logging time is not proof of consumption time. IOB is Juggluco's own calculation at each bin start using current insulin-label mappings, including earlier insulin still active. Disabled/unavailable remains null. Alignment does not establish causation or record unlogged exercise."}};
}
}
