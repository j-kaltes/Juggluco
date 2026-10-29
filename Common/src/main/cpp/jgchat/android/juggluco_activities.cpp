// SPDX-License-Identifier: GPL-3.0-or-later
// Read the same store, mutex, visibility rules and summaries as x/activities.
#include "jgchat/ui_messages.hpp"
#include "juggluco_extra.hpp"
#include "activity/global.hpp"
#include "activity/export.hpp"
#include <set>

namespace jgchatdata {
namespace {
using jgchat::Json;
namespace a = juggluco::activity;

void cancelled(const std::atomic_bool* cancel) {
    if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
}
Json unavailable() {
    return {{"status", "unavailable"}, {"reason", "The local activity store is not initialized."}};
}

std::string revision(const a::Store& store) {
    const auto& h = store.header();
    return std::to_string(h.mirrorEpoch) + ":" + std::to_string(h.generation) + ":" +
        std::to_string(h.mirrorRevision) + ":" + std::to_string(h.count);
}
Json changed(const std::string& current) {
    return {{"status", "changed"}, {"revision", current},
        {"message", "The activity store changed between pages. Restart with offset=0 and revision=null; do not combine the old and new pages."}};
}
Json finite(double value) { return std::isfinite(value) ? Json(value) : Json(nullptr); }
Json time_or_null(uint32_t value) { return value ? Json(value) : Json(nullptr); }
Json source(const a::Store& store, uint32_t id) {
    const auto value = store.source(id);
    return {{"source_id", id}, {"origin", a::tsv::textCell(store.text(value.origin))},
        {"device", a::tsv::textCell(store.text(value.device))}};
}
bool numeric_column(std::size_t column) {
    using namespace a::tsv;
    return column == Group || column == RecordID || column == Type || column == DistanceValue ||
        column == Energy || column == SpeedValue || column == MaximumSpeed || column == StepsValue ||
        column == ElevationValue || column == PowerValue || column == CadenceValue || column == SummaryHeart ||
        column == RestingHeart || column == MeanHeart || column == MaximumHeart || column == HeartSD ||
        column == ExcessHeart || column == Modified;
}
Json cell(std::string_view value, std::size_t column) {
    if (value.empty()) return nullptr;
    if (numeric_column(column)) {
        auto parsed = Json::parse(value, nullptr, false);
        if (!parsed.is_discarded() && parsed.is_number()) return parsed;
    }
    return std::string(value);
}
const char* metric_name(int metric) {
    const char* names[] = {"duration", "distance", "mean_minus_resting_heart_rate", "mean_heart_rate",
        "heart_rate_standard_deviation", "active_energy", "mean_speed", "steps", "elevation_gain",
        "mean_power", "mean_cycling_cadence", "distance_miles", "mean_speed_miles", "maximum_heart_rate"};
    return metric >= 0 && std::size_t(metric) < std::size(names) ? names[metric] : "unknown";
}
const char* metric_unit(int metric) {
    const char* units[] = {"min", "km", "bpm", "bpm", "bpm", "kcal", "km/h", "steps", "m", "W", "rpm", "mi", "mi/h", "bpm"};
    return metric >= 0 && std::size_t(metric) < std::size(units) ? units[metric] : "unknown";
}
Json label(int id, const Json& labels) {
    Json name = nullptr;
    for (const auto& item : labels) if (item.value("id", -1) == id) { name = item.value("name", Json(nullptr)); break; }
    return {{"id", id}, {"name", std::move(name)}};
}
}

Json activity_records(uint32_t start, uint32_t end, uint32_t offset, uint32_t limit,
        const std::string& expected_revision, const std::atomic_bool* cancel) {
    cancelled(cancel);
    if (globalbasedir.empty()) return unavailable();
    auto& store = a::globalStore();
    std::lock_guard guard(store.mutex);
    cancelled(cancel);
    const auto current = revision(store);
    if (!expected_revision.empty() && expected_revision != current) return changed(current);
    std::vector<uint32_t> order;
    for (uint32_t i = 1; i <= store.header().count; ++i) {
        cancelled(cancel);
        const auto& record = *store.get(i);
        if (record.start < end && record.end > start && store.sourceVisible(i) && store.groupOf(i)) order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](auto left, auto right) {
        const auto l = store.get(left)->start, r = store.get(right)->start;
        return l != r ? l < r : left < right;
    });
    const auto begin = std::min<std::size_t>(offset, order.size());
    const auto finish = begin + (limit ? std::min<std::size_t>(limit, order.size() - begin) : order.size() - begin);
    const bool miles = store.header().options & a::UseMiles;
    const auto headings = a::tsv::headings(miles);
    a::tsv::TextLocale locale;
    Json rows = Json::array(), sources = Json::array();
    std::set<uint32_t> source_ids;
    for (auto pos = begin; pos < finish; ++pos) {
        cancelled(cancel);
        const auto index = order[pos];
        const auto& record = *store.get(index);
        source_ids.insert(record.source);
        for (const auto& heart : store.heartSummaries(record)) source_ids.insert(heart.source);
        a::tsv::rows(store, std::span<const uint32_t>(&index, 1), miles, true,
            [&](const a::tsv::Row& row) {
                cancelled(cancel);
                Json out = Json::object();
                for (std::size_t i = 0; i < row.size(); ++i) out[headings[i]] = cell(row[i], i);
                // The exact Unix times make glucose/IOB joins independent of
                // rounded display strings, locale and daylight-saving changes.
                out["start_unix_seconds"] = record.start;
                out["end_unix_seconds"] = record.end;
                out["duration_seconds"] = finite(record.values[a::Seconds]);
                out["activity_source_id"] = record.source;
                rows.push_back(std::move(out));
                return true;
            });
    }
    for (auto id : source_ids) if (id) sources.push_back(source(store, id));
    Json result{{"status", "ok"}, {"source", "Juggluco saved activities; native extended x/activities exporter"},
        {"generated_at", std::time(nullptr)}, {"start", start}, {"end", end}, {"revision", current},
        {"offset", offset}, {"limit", limit}, {"total_records", order.size()}, {"returned_records", finish - begin},
        {"returned_rows", rows.size()}, {"next_offset", finish < order.size() ? Json(finish) : Json(nullptr)},
        {"last_successful_retrieval_at", time_or_null(store.header().lastSuccess)},
        {"distance_unit", miles ? "mi" : "km"}, {"speed_unit", miles ? "mi/h" : "km/h"},
        {"sources", std::move(sources)}, {"records", std::move(rows)},
        {"interpretation", "Intervals overlap [start,end); complete session summaries are returned, not clipped to the query. Each Record is one source recording. Each Group is one merged activity. HR rows repeat session fields: do not sum their durations, distances or energy, or count source recordings as independent activities. Summary HR=1 selects that recording's HR source; never average different watches/apps. Missing fields are null, not zero. Numeric fields retain the export's rounding. Text fields use its backslash escapes. Not-returned/details-pending records are explicitly marked. Cache absence does not prove inactivity; retrieval can lag."}};
    return result;
}

Json activity_samples(uint32_t record_id, const std::string& kind, uint32_t start, uint32_t end,
        std::optional<uint32_t> source_id, uint32_t offset, uint32_t limit,
        const std::string& expected_revision, const std::atomic_bool* cancel) {
    cancelled(cancel);
    if (globalbasedir.empty()) return unavailable();
    auto& store = a::globalStore();
    std::lock_guard guard(store.mutex);
    cancelled(cancel);
    const auto current = revision(store);
    if (!expected_revision.empty() && expected_revision != current) return changed(current);
    const auto* record = store.get(record_id);
    if (!record || !store.sourceVisible(record_id))
        return {{"status", "unavailable"}, {"reason", "The source recording is absent, deleted or filtered. Retrieve current activities again."}};
    a::tsv::TextLocale locale;
    a::Slice slice;
    const char* unit;
    if (kind == "heart_rate") { slice = record->heart; unit = "bpm"; }
    else if (kind == "resting_heart_rate") { slice = record->rest; unit = "bpm"; }
    else if (kind == "speed") { slice = record->speed; unit = "m/s"; }
    else if (kind == "power") { slice = record->power; unit = "W"; }
    else if (kind == "cadence") { slice = record->cadence; unit = "rpm"; }
    else throw jgchat::UiArgumentError(jgchat::UiMessage::detail(jgchat::UiCode::invalid_tool_arguments, "Unknown activity sample kind."));
    const auto samples = store.view<a::Sample>(slice);
    Json rows = Json::array(), sources = Json::array();
    uint64_t matched = 0;
    std::set<uint32_t> source_ids;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        cancelled(cancel);
        const auto& sample = samples[index];
        if (sample.time < int64_t(start) * 1000 || sample.time >= int64_t(end) * 1000 ||
            (source_id && sample.source != *source_id) || store.sourceRemoved(record->group, sample.source)) continue;
        if (matched++ < offset || (limit && rows.size() >= limit)) continue;
        Json row{{"time_unix_ms", sample.time}, {"source_id", sample.source},
            {"value", finite(sample.value)}, {"stored_sample_index", index}};
        rows.push_back(std::move(row)); source_ids.insert(sample.source);
    }
    for (auto id : source_ids) if (id) sources.push_back(source(store, id));
    const uint64_t next = uint64_t(offset) + rows.size();
    Json result{{"status", "ok"}, {"record_id", record_id}, {"group_id", store.groupOf(record_id)},
        {"kind", kind}, {"unit", unit}, {"start", start}, {"end", end}, {"revision", current},
        {"offset", offset}, {"limit", limit}, {"total_samples", matched}, {"returned", rows.size()},
        {"next_offset", next < matched ? Json(next) : Json(nullptr)},
        {"sources", std::move(sources)}, {"records", std::move(rows)},
        {"interpretation", "Stored original samples for this source recording, ordered by source then time. Query start/end use Unix seconds; sample times use Unix milliseconds. Raw speed uses m/s even with miles display; it retains spikes excluded from the export's filtered maximum. Missing values are null. Keep sources separate; a missing sample is not zero or inactivity. These are cached session samples, not a continuous current all-day heart-rate feed."}};
    return result;
}

Json activity_settings(const Json& labels, bool mirrored_amounts, const std::atomic_bool* cancel) {
    cancelled(cancel);
    if (globalbasedir.empty()) return unavailable();
    auto& store = a::globalStore();
    std::lock_guard guard(store.mutex);
    cancelled(cancel);
    const auto& h = store.header();
    Json mappings = Json::array(), measures = Json::array(), permissions = Json::object();
    for (uint32_t i = 0; i < h.ruleCount; ++i) {
        const auto& rule = h.rules[i];
        mappings.push_back({{"activity_type", rule.type}, {"activity", a::typeName(rule.type)},
            {"label", label(rule.label, labels)}, {"metric_id", rule.metric},
            {"metric", metric_name(rule.metric)}, {"unit", metric_unit(rule.metric)},
            {"rounding_increment", rule.roundTo > 0 ? rule.roundTo : rule.metric == a::Steps ? 1.0f : 0.1f}});
    }
    for (int metric = 0; metric < a::MetricCount; ++metric)
        measures.push_back({{"id", metric}, {"name", metric_name(metric)}, {"unit", metric_unit(metric)}});
    // Same order as ActivityHealth.types. This mask was saved during retrieval,
    // not obtained by a live permission or Health Connect availability query.
    const char* kinds[] = {"exercise_sessions", "heart_rate", "resting_heart_rate", "distance",
        "active_energy", "speed", "steps", "elevation_gain", "power", "cycling_cadence"};
    for (std::size_t i = 0; i < std::size(kinds); ++i) permissions[kinds[i]] = bool(h.readPermissions & (1U << i));
    permissions["history"] = bool(h.readPermissions & (1U << 16));
    return {{"status", "ok"}, {"section", "activities"}, {"generated_at", std::time(nullptr)},
        {"revision", revision(store)}, {"retrieve_from", time_or_null(h.from)},
        {"retrieval_interval_seconds", h.interval}, {"automatic_retrieval_configured", h.interval != 0},
        {"retrieval_disabled_by_mirrored_amounts", mirrored_amounts},
        {"last_successful_retrieval_at", time_or_null(h.lastSuccess)},
        {"create_amounts_from_activities", bool(h.options & a::CreateAmounts)},
        {"distance_unit", h.options & a::UseMiles ? "mi" : "km"},
        {"speed_unit", h.options & a::UseMiles ? "mi/h" : "km/h"},
        {"default_mapping", {{"label", label(h.defaultLabel, labels)}, {"metric", "duration"},
            {"unit", "min"}, {"rounding_increment", 0.1}}},
        {"mappings", std::move(mappings)}, {"available_measures", std::move(measures)},
        {"last_recorded_read_permissions", std::move(permissions)},
        {"background_read_permission", nullptr}, {"permission_observed_at", nullptr},
        {"store_has_received_mirror_image", h.receivedMirror != 0},
        {"meaning", "Current saved configuration. No import, permission prompt, Amount creation or settings change is performed. Zero interval means automatic retrieval is off; Android can delay scheduled retrieval. Receiving mirrored Amounts disables local Health Connect retrieval. The saved permission mask is not live permission/provider status; background grant is not stored here. Unconfigured activity types fall back to the default duration mapping when Amount creation is enabled. Display units do not change mapping units. Current mappings are not historical settings or proof that an existing Amount was generated with them. Existing/manual Amounts and their timestamps/values can be preserved independently."}};
}
}
