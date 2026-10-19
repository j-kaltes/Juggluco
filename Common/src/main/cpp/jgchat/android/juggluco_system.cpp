// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "juggluco_system.hpp"
#include "jgchat/diagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <mutex>
#include <map>
#include <stdexcept>

namespace jgchatdata {
namespace {
using jgchat::Json;
std::mutex wear_mutex;
Json wear_cache{{"status", "unavailable"}, {"nodes", Json::array()}};
std::mutex activity_mutex;
Json activity_cache = Json::object();
Json stamp(uint64_t t) { return t && t <= UINT32_MAX ? Json(t) : Json(nullptr); }
constexpr auto sensor_times = "Unix seconds. Nominal/expected ends use Juggluco's own wear durations, including its fallback defaults; expected end is not a guarantee of sensor operation. Last readings and marked-finished are separate observations, not measured removal times. Negative remaining time means nominal wear has elapsed. first_glucose_at/last_glucose_at bound valid recorded stream, scan or real-history glucose; start alone does not establish successful use. For last recorded use of a model, compare last_glucose_at only for positively identified matching sensors with has_valid_glucose=true. no_valid_glucose means no valid glucose remains in this local record, not proof of failure, non-wear or absence of data elsewhere.";
constexpr auto sensor_provenance = "Current local sensor metadata; mirrored sensor records can be present. Join sensor_id to Sensorid in glucose exports. Configuration is not historical transport provenance. Type identification uses native family flags, recognized Libre serial-family rules, history interval and valid recorded stream evidence. Serial rules apply only to canonical 11-character IDs in the non-Libre-3 Libre family; other prefixes are not guessed. Libre protocol generation is not the marketed model number.";
void check(const std::atomic_bool* cancel) {
    if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
}
}
Json sensor_record(const SensorSnapshot& s, uint32_t now) {
    Json result{{"sensor_index", s.index}, {"sensor_id", s.id}, {"display_name", s.display_name},
        {"status", s.available ? "ok" : "unavailable"}};
    if (!s.available) {
        result.update({{"glucose_data_status", "unavailable"}, {"has_valid_glucose", nullptr},
            {"first_glucose_at", nullptr}, {"last_glucose_at", nullptr}});
        return result;
    }
    std::string type = "Unknown", basis = "Juggluco sensor flags";
    const bool has_stream = s.last_stream != 0;
    unsigned libre_model = 0;
    const char* classification = "native_family";
    const char* serial_rule = nullptr;
    Json excluded = Json::array();
    switch (s.family) {
        case SensorFamily::libre: {
            excluded = Json::array({"FreeStyle Libre 3", "FreeStyle Libre 3+"});
            // share/serial.cpp encodes getProductFamily() as the first character.
            // 0M identifies Libre 1; family code 3 identifies Libre 2 even when
            // activation failed and no stream/scan/history values were stored.
            const bool canonical = s.id.size() == 11 &&
                s.id.find_first_not_of("0123456789ACDEFGHJKLMNPQRTUVWXYZ") == std::string::npos;
            if (canonical && s.id.starts_with("0M")) {
                serial_rule = "0M identifies Libre 1";
                if (has_stream) {
                    type = "FreeStyle Libre (conflicting model evidence)";
                    classification = "conflicting_evidence";
                    basis = "The 0M ID identifies Libre 1 but valid stream readings contradict that identification. Do not count this as confirmed Libre 1 or Libre 2 use without resolving the record conflict.";
                } else {
                    type = "FreeStyle Libre 1"; libre_model = 1;
                    classification = "identified_from_serial_family";
                    basis = "The canonical Libre sensor ID starts with 0M, identifying Libre 1 within Juggluco's non-Libre-3 Libre family. This identification does not depend on whether glucose was recorded.";
                    excluded.push_back("FreeStyle Libre 2"); excluded.push_back("FreeStyle Libre 2+");
                }
            } else if (canonical && s.id[0] == '3') {
                serial_rule = "Encoded product-family code 3 identifies Libre 2";
                type = "FreeStyle Libre 2"; libre_model = 2;
                classification = "identified_from_serial_family";
                basis = "The canonical sensor ID encodes product-family code 3, identifying Libre 2 within Juggluco's non-Libre-3 Libre family. Zero recorded glucose does not turn a failed or empty Libre 2 record into a possible Libre 1.";
                excluded.push_back("FreeStyle Libre 1");
            } else if (has_stream) {
                type = "FreeStyle Libre 2"; libre_model = 2;
                classification = "identified_from_recorded_data";
                basis = "Juggluco identifies the non-Libre-3 Libre family and has valid stream readings. In this native store, that identifies Libre 2. ";
                if (s.history_interval == 900)
                    basis += "Juggluco's effective 15-minute history interval also differs from Libre 3's stored 5-minute history interval.";
                else basis += "The native family classification excludes Libre 3.";
                excluded.push_back("FreeStyle Libre 1");
            } else {
                type = "FreeStyle Libre (model unknown; not Libre 3)";
                classification = "family_only";
                basis = "Juggluco identifies the non-Libre-3 Libre family, but no recognized serial-family rule or valid stream evidence identifies a model. Do not treat missing stream data as evidence of Libre 1 or use this record as a latest possible Libre 1 date.";
            }
            break;
        }
        case SensorFamily::libre3:
            type = "FreeStyle Libre 3"; libre_model = 3;
            basis = "Juggluco classifies this Libre-family record as Libre 3 using its stored 5-minute history interval, after excluding other sensor families.";
            excluded = Json::array({"FreeStyle Libre 1", "FreeStyle Libre 2", "FreeStyle Libre 2+"});
            break;
        case SensorFamily::dexcom: type = "Dexcom G7 / ONE+";
            basis = "Juggluco Dexcom family flag; G7 versus ONE+ is not identified by this field"; break;
        case SensorFamily::sibionics:
            type = !s.new_si ? "Sibionics GS1Sb" : s.si_subtype == 3 ? "Sibionics GS2" :
                (s.si_subtype == 4 || s.si_subtype == 5) ? "Sibionics GS3" : s.si_subtype < 3 ?
                "Sibionics GS1" : "Sibionics (subtype unknown)"; break;
        case SensorFamily::accu_chek: type = "Accu-Chek SmartGuide"; break;
        case SensorFamily::aidex_x: type = "AiDEX X / compatible sensor"; break;
        case SensorFamily::caresens_air: type = "CareSens Air"; break;
        default: break;
    }
    const bool libre = s.family == SensorFamily::libre || s.family == SensorFamily::libre3;
    if (libre) {
        Json variant{{"value", nullptr}, {"status", "unknown"}};
        if (libre_model == 1)
            variant.update({{"value", false}, {"status", "not_applicable"}, {"basis", "The identified model is Libre 1, not a Libre 2/3 plus variant."}});
        else if (!s.stored_wear_seconds) variant["basis"] = "Nominal wear uses a fallback default; no recorded nominal wear establishes the variant.";
        else if (s.stored_wear_seconds == 14U * 86400) {
            variant.update({{"value", false}, {"status", "not_indicated"}, {"basis", "Stored nominal wear is 14 days."}});
        } else if (s.stored_wear_seconds == 15U * 86400 && (libre_model == 2 || libre_model == 3)) {
            variant.update({{"value", true}, {"status", "inferred_from_nominal_wear"}, {"basis", "Stored nominal wear is 15 days."}});
            type += "+";
        } else variant["basis"] = "This combination of stored nominal wear and identified model does not establish a plus variant.";
        result["libre_model"] = libre_model ? Json(libre_model) : Json(nullptr);
        result["libre_plus_variant"] = std::move(variant);
        result["excluded_models"] = std::move(excluded);
    }
    const bool started = s.start && !s.unused;
    const uint32_t last_glucose = std::max({s.last_stream, s.last_scan, s.last_history});
    result.update({{"type", type}, {"type_basis", basis}, {"type_classification", classification},
        {"type_evidence", {{"has_valid_stream_readings", has_stream},
            {"libre_serial_rule", serial_rule ? Json(serial_rule) : Json(nullptr)},
            {"history_interval_seconds", s.history_interval},
            {"stored_history_interval_seconds", s.stored_history_interval ? Json(s.stored_history_interval) : Json(nullptr)},
            {"history_interval_source", s.stored_history_interval ? "stored" : "Juggluco fallback"},
            {"libre_protocol_generation", s.family == SensorFamily::libre ? Json(s.libre_protocol_generation) : Json(nullptr)},
            {"protocol_generation_semantics", "Internal Libre communication protocol value, not a Libre 1/2/3 product number. Values 1 and 2 are used by Libre2GattCallback."}}},
        {"nominal_wear_seconds", s.wear_seconds},
        {"stored_nominal_wear_seconds", s.stored_wear_seconds ? Json(s.stored_wear_seconds) : Json(nullptr)},
        {"nominal_wear_source", s.stored_wear_seconds ? "stored" : "Juggluco fallback"},
        {"expected_operating_seconds", s.expected_seconds}, {"warmup_seconds", s.warmup_seconds},
        {"start", started ? stamp(s.start) : Json(nullptr)},
        {"nominal_end", started ? stamp(uint64_t(s.start) + s.wear_seconds) : Json(nullptr)},
        {"expected_end", started ? stamp(uint64_t(s.start) + s.expected_seconds) : Json(nullptr)},
        {"age_seconds", started && now >= s.start ? Json(now - s.start) : Json(nullptr)},
        {"nominal_remaining_seconds", started ? Json(int64_t(s.start) + s.wear_seconds - now) : Json(nullptr)},
        {"glucose_data_status", last_glucose ? "recorded" : "no_valid_glucose"},
        {"has_valid_glucose", last_glucose != 0}, {"first_glucose_at", stamp(s.first_glucose)},
        {"last_glucose_at", stamp(last_glucose)}, {"last_history_at", stamp(s.last_history)},
        {"last_stream_at", stamp(s.last_stream)}, {"last_scan_at", stamp(s.last_scan)},
        {"stream_interval_seconds", s.stream_interval}, {"history_interval_seconds", s.history_interval},
        {"marked_finished", s.finished}, {"unused", s.unused}, {"hidden_on_curve", s.hidden},
        {"restart_at", s.restart > s.start ? stamp(s.restart) : Json(nullptr)},
        {"reading_transport_provenance", "not recorded per reading"}});
    if (s.family == SensorFamily::sibionics) result["sibionics_subtype_id"] = s.si_subtype;
    return result;
}
Json sensor_records(uint32_t offset, uint32_t limit, const std::atomic_bool* cancel) {
    check(cancel);
    const auto now = static_cast<uint32_t>(std::time(nullptr));
    auto page = read_sensor_page(offset, limit ? limit : UINT32_MAX, cancel);
    if (!page.available) return {{"status", "unavailable"}, {"reason", "Sensor store is not initialized."}};
    Json rows = Json::array();
    for (const auto& sensor : page.rows) { check(cancel); rows.push_back(sensor_record(sensor, now)); }
    const uint64_t next = uint64_t(offset) + rows.size();
    return {{"status", "ok"}, {"generated_at", now}, {"offset", offset}, {"total", page.total},
        {"order", "newest sensor index first"}, {"sensors", std::move(rows)},
        {"next_offset", next < page.total ? Json(next) : Json(nullptr)},
        {"time_semantics", sensor_times}, {"provenance", sensor_provenance}};
}
Json exact_sensor_records(const std::vector<std::string>& ids, const std::atomic_bool* cancel) {
    check(cancel);
    std::map<std::string, unsigned> matches;
    for (const auto& id : ids) {
        if (id.empty() || id.find_first_of(std::string_view("\0\t\r\n", 4)) != std::string::npos)
            throw jgchat::UiArgumentError(jgchat::UiCode::sensor_id_must_be_1_128_bytes_without_nul_tab_or_newline);
        matches.emplace(id, 0);
    }
    const auto now = static_cast<uint32_t>(std::time(nullptr));
    auto page = read_sensors_by_id(ids, cancel);
    Json rows = Json::array(), missing = Json::array(), unavailable = Json::array(), ambiguous = Json::array();
    for (const auto& sensor : page.rows) {
        check(cancel);
        auto match = matches.find(sensor.id);
        if (match == matches.end()) throw jgchat::UiError(jgchat::UiCode::unexpected_sensor_id_in_exact_lookup);
        ++match->second;
        rows.push_back(sensor_record(sensor, now));
        if (!sensor.available && std::find(unavailable.begin(), unavailable.end(), sensor.id) == unavailable.end())
            unavailable.push_back(sensor.id);
    }
    for (const auto& [id, count] : matches) {
        if (!page.available) unavailable.push_back(id);
        else if (!count) missing.push_back(id);
        else if (count > 1) ambiguous.push_back(id);
    }
    jgchat::diagnostic("sensor lookup requested=%zu indexed=%u matched=%zu missing=%zu unavailable=%zu ambiguous=%zu store_ready=%d",
        matches.size(), page.total, rows.size(), missing.size(), unavailable.size(), ambiguous.size(), int(page.available));
    return {{"status", page.available ? "ok" : "unavailable"}, {"generated_at", now},
        {"sensors", std::move(rows)}, {"not_found_ids", std::move(missing)},
        {"unavailable_ids", std::move(unavailable)}, {"ambiguous_ids", std::move(ambiguous)},
        {"searched_sensor_count", page.available ? Json(page.total) : Json(nullptr)},
        {"lookup", "Exact case-sensitive Sensorid match across the entire local sensor index, including old/finished sensors. No recent-page limit or prefix matching. Recognized serial-family rules are applied only to the matched native records. not_found_ids means absent from the index; unavailable_ids means unreadable metadata or an uninitialized store; ambiguous_ids means multiple matching index records."},
        {"time_semantics", sensor_times}, {"provenance", sensor_provenance}};
}
Json calibration_records(const std::string& id, std::optional<uint32_t> index,
        const std::atomic_bool* cancel) {
    check(cancel);
    if (id.empty() || id.find_first_of(std::string_view("\0\t\r\n", 4)) != std::string::npos)
        throw jgchat::UiArgumentError(jgchat::UiCode::sensor_id_must_be_1_128_bytes_without_nul_tab_or_newline);
    const auto snapshot = read_calibrations(id, index, cancel);
    Json result{{"status", snapshot.status}, {"generated_at", std::time(nullptr)},
        {"sensor_id", id}, {"sensor_index", snapshot.sensor_index ? Json(*snapshot.sensor_index) : Json(nullptr)},
        {"matching_sensor_indices", snapshot.matching_indices}, {"records", Json::array()}};
    if (!snapshot.reason.empty()) result["reason"] = snapshot.reason;
    if (snapshot.status != "ok") return result;
    Json sources = Json::object();
    bool complete = true;
    for (unsigned source = 0; source < snapshot.sources.size(); ++source) {
        const char* name = source == 0 ? "stream" : "history";
        const auto& series = snapshot.sources[source];
        sources[name] = {{"status", series.valid_count ? "ok" : "invalid_count"},
            {"stored_count", series.stored_count}, {"returned", series.rows.size()}};
        if (!series.valid_count) { complete = false; continue; }
        for (std::size_t pos = 0; pos < series.rows.size(); ++pos) {
            check(cancel);
            const auto& c = series.rows[pos];
            const bool finite = std::isfinite(c.a) && std::isfinite(c.b);
            result["records"].push_back({{"sensor_id", id}, {"sensor_index", *snapshot.sensor_index},
                {"source", name}, {"position", pos}, {"time", c.time},
                {"a", std::isfinite(c.a) ? Json(c.a) : Json(nullptr)},
                {"b", std::isfinite(c.b) ? Json(c.b) : Json(nullptr)},
                {"finite_coefficients", finite}, {"time_available", c.time != 0}});
        }
    }
    result.update({{"status", complete ? "ok" : "invalid_data"}, {"complete", complete},
        {"sources", std::move(sources)}, {"returned", result["records"].size()},
        {"has_real_history", snapshot.has_real_history},
        {"current_settings", {{"status", snapshot.settings_available ? "ok" : "unavailable"},
            {"calibration_enabled", snapshot.settings_available ? Json(snapshot.enabled) : Json(nullptr)},
            {"calibrate_past", snapshot.settings_available ? Json(snapshot.past_values) : Json(nullptr)}}},
        {"units", {{"time", "Unix seconds"}, {"a", "dimensionless"}, {"b", "mg/dL"}}},
        {"provenance", "Stored Info.calis[0] (stream) and Info.calis[1] (history) for this exact sensor record. Original array order and values; no recalculation. These are the currently retained coefficients, not an audit trail of edits or deleted calibrations. Disabled calibration does not erase stored records. Empty sources with status=ok contain no stored calibrations; invalid_count is unavailable, not empty."},
        {"interpretation", "Base correction is a*x+b with x and b in mg/dL. Juggluco's calibration kernel uses this directly when reading_time<=calibration_time. After that it uses w*(a*x+b)+(1-w)*x, w=2/(1+exp(2.3148148148148148e-6*age_seconds)); if w<=0 it returns unavailable. Coefficient selection and past-values behavior depend on the native export. To obtain actual calibrated glucose use juggluco_glucose with calibrated=true and the intended pastvalues option; do not apply a fixed a*x+b indefinitely. time is the stored calibration timestamp, not a sensor start, expiry or retrieval time. Nonfinite coefficients are null and flagged; zero timestamps remain zero and are flagged unavailable."}});
    jgchat::diagnostic("calibrations returned=%zu stream_count=%u history_count=%u complete=%d",
        result["records"].size(), snapshot.sources[0].stored_count, snapshot.sources[1].stored_count, int(complete));
    return result;
}
void update_wear_nodes(const Json& snapshot) {
    if (!snapshot.is_object() || !snapshot.contains("available") || !snapshot["available"].is_boolean() ||
        !snapshot.contains("nodes") || !snapshot["nodes"].is_array())
        throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_wear_snapshot);
    Json nodes = Json::array();
    for (const auto& node : snapshot["nodes"]) {
        if (!node.is_object() || !node.contains("id") || !node["id"].is_string() ||
            !node.contains("name") || !node["name"].is_string() || !node.contains("nearby") || !node["nearby"].is_boolean())
            throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_wear_node);
        const auto id = node["id"].get<std::string>(), name = node["name"].get<std::string>();
        if (id.empty() || id.find('\0') != std::string::npos || name.find('\0') != std::string::npos)
            throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_wear_node_text);
        // Whitelist fields: never forward arbitrary platform/Java JSON.
        nodes.push_back({{"id", id}, {"name", name}, {"nearby", node["nearby"]}});
    }
    Json safe{{"status", snapshot["available"] == true ? "ok" : "unavailable"},
        {"observed_at", std::time(nullptr)}, {"source_updated_at", nullptr}, {"nodes", std::move(nodes)},
        {"source", "Juggluco's cached reachable Wear OS nodes with the Juggluco capability; no discovery or messages triggered by this read"}};
    std::lock_guard lock(wear_mutex); wear_cache = std::move(safe);
}
Json device_configuration(const std::atomic_bool* cancel) {
    check(cancel);
    auto result = mirror_configuration(cancel);
    result["mirror_configuration_status"] = result.value("status", std::string("unavailable"));
    if (result.contains("reason")) {
        result["mirror_configuration_reason"] = result["reason"];
        result.erase("reason");
    }
    for (auto& host : result["mirrors"]) {
        check(cancel);
        const bool enabled = host.at("enabled").get<bool>();
        const bool receive = host.at("receive").get<bool>();
        const bool send_stream = host.at("send").at("stream").get<bool>();
        const bool watch = host.at("wear_os").get<bool>();
        host["incoming_data_eligible"] = enabled && receive;
        host["incoming_data_exclusion"] = !enabled ? Json("mirror_disabled") :
            !receive ? Json("receiving_disabled") : Json(nullptr);
        // Receiving is a local permission, not knowledge of the remote sender's
        // data selections. Preserve both directions when both are configured.
        host["configured_stream_direction"] = receive ?
            (send_stream ? "bidirectional_possible" : (watch ? "watch_to_phone_possible" : "mirror_to_phone_possible")) :
            (send_stream ? (watch ? "phone_to_watch" : "phone_to_mirror") : "not_configured");
    }
    { std::lock_guard lock(wear_mutex); result["wear_os_discovery"] = wear_cache; }
    result["phone_sensor_activity"] = activity_configuration("sensors", cancel);
    result["garmin"] = activity_configuration("garmin", cancel);
    // An uninitialized mirror store must not hide available Garmin evidence.
    result["status"] = result["mirror_configuration_status"] == "ok" ||
        result["phone_sensor_activity"]["status"] == "ok" || result["garmin"]["status"] == "ok" ? "ok" : "unavailable";
    result["direction_meaning"] = "All mirror directions are relative to this phone. receive=false or enabled=false excludes that mirror as an incoming data source under these settings, even with recent sync. send.stream=true means this phone sends glucose to that mirror. active_only/passive_only concern connection initiation, not data direction. Receiving allowed makes a route possible, not proven.";
    result["last_sync_meaning"] = "last_sync_at is Juggluco's up-to-date marker, updated by both the outgoing update path and incoming suptodate commands. It is not an incoming-glucose timestamp, a direction indicator, or per-reading provenance.";
    result["bluetooth_meaning"] = "phone_sensor_bluetooth_enabled is Juggluco's Use Bluetooth preference for direct sensor reception, not the Android radio state. The radio observation, when available, is phone_sensor_activity.bluetooth_enabled. Garmin direct handoff deliberately disables phone sensor reception while retaining phone-watch communication.";
    result["scope"] = "Current configuration and cached observations. For sensor connection questions inspect Garmin's selected direct sensor receiver as well as phone callbacks and mirror directions. Garmin glucose return uses its own route, independent of mirror receive flags. Report the configured sensor-to-Garmin route when selected; distinguish it from a live sensor-link observation. Configured Wear OS mirrors include disconnected watches; match cached node id to mirror label. observed_direct_ble is phone-to-mirror transport, not sensor-to-watch BLE. Remote battery, alarms and live sensor sessions are not fetched. Per-reading transport history is not recorded.";
    return result;
}

namespace {
void activity_fields(Json& out, const Json& in, const std::initializer_list<const char*>& names,
                     const char type) {
    for (const auto* name : names) {
        if (!in.contains(name)) continue;
        const auto& value = in[name];
        if (value.is_null()) { out[name] = nullptr; continue; }
        if (type == 'b' ? !value.is_boolean() : type == 's' ? !value.is_string() : !value.is_number_integer())
            throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_activity_field_type);
        out[name] = value;
    }
}
}
void update_phone_activity(const Json& snapshot) {
    if (!snapshot.is_object()) throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_activity_snapshot);
    Json safe = Json::object();
    for (const auto* section : {"sensors", "garmin"}) {
        if (!snapshot.contains(section) || !snapshot[section].is_object())
            throw jgchat::UiArgumentError(jgchat::UiCode::missing_android_activity_section);
        const auto& input = snapshot[section];
        const auto status = input.value("status", std::string("unavailable"));
        if (status != "ok" && status != "unavailable") throw jgchat::UiArgumentError(jgchat::UiCode::invalid_android_activity_status);
        Json out{{"status", status}, {"observed_at", std::time(nullptr)}};
        if (std::string_view(section) == "sensors") {
            activity_fields(out, input, {"bluetooth_enabled", "truncated"}, 'b');
            activity_fields(out, input, {"scan_started_at", "scan_timeout_at", "scan_stopped_at", "total"}, 'n');
        } else {
            activity_fields(out, input, {"transport_mode", "total"}, 'n');
            activity_fields(out, input, {"has_active_watch", "truncated"}, 'b');
        }
        const char* array_name = std::string_view(section) == "sensors" ? "callbacks" : "watches";
        Json rows = Json::array();
        if (input.contains(array_name)) {
            const auto& incoming = input[array_name];
            if (!incoming.is_array()) throw jgchat::UiArgumentError(jgchat::UiCode::invalid_activity_count);
            for (const auto& row : incoming) {
                if (!row.is_object()) throw jgchat::UiArgumentError(jgchat::UiCode::invalid_activity_record);
                Json item = Json::object();
                if (std::string_view(section) == "sensors") {
                    activity_fields(item, row, {"sensor_id", "connection_status", "handshake_status"}, 's');
                    activity_fields(item, row, {"started_at", "connection_attempt_at", "found_at", "connected_at", "disconnected_at",
                        "handshake_success_at", "handshake_failure_at", "glucose_success_at", "glucose_failure_at", "rssi_dbm", "protocol_generation"}, 'n');
                    activity_fields(item, row, {"stopped", "address_known"}, 'b');
                } else {
                    activity_fields(item, row, {"id", "name", "communication_status", "last_transport_status", "last_error"}, 's');
                    activity_fields(item, row, {"connected", "direct_ble", "active", "send_glucose", "numbers_device", "libre3_direct",
                        "libre3_installed", "watch_stopped", "timestamped_glucose_ack"}, 'b');
                    activity_fields(item, row, {"last_sent_at", "last_received_at", "last_status_at", "last_acknowledged_at",
                        "last_glucose_acknowledged_at", "acknowledged_glucose_sample_at", "app_version"}, 'n');
                }
                rows.push_back(std::move(item));
            }
        }
        out[array_name] = std::move(rows);
        safe[section] = std::move(out);
    }
    std::lock_guard lock(activity_mutex);
    activity_cache = std::move(safe);
}
Json activity_configuration(const std::string& section, const std::atomic_bool* cancel) {
    check(cancel);
    if (section != "sensors" && section != "garmin") throw jgchat::UiArgumentError(jgchat::UiCode::unknown_activity_section);
    Json result;
    {
        std::lock_guard lock(activity_mutex);
        result = activity_cache.value(section, Json{{"status", "unavailable"}, {"observed_at", nullptr}});
    }
    const int64_t now = std::time(nullptr);
    result["generated_at"] = now;
    if (result["observed_at"].is_number_integer()) {
        const auto age = now - result["observed_at"].get<int64_t>();
        result["age_seconds"] = std::max<int64_t>(0, age);
        result["stale"] = age > 15 || age < 0;
    } else { result["age_seconds"] = nullptr; result["stale"] = true; }
    result["scope"] = "Phone-side cached observations, refreshed about every five seconds while a chat is visible or a request is running. Times are Unix seconds; null means unavailable/not observed. observed_at is when the fields were copied, not when every event happened or RSSI was measured. Not an event history; no scan, reconnect, read-RSSI, device probe or upload is triggered. Fields may update independently between callbacks. Configuration does not prove delivery.";
    if (section == "sensors") {
        result["bluetooth_settings"] = settings_configuration("bluetooth", cancel);
        result["source"] = "The SensorBluetooth/SuperGattCallback fields shown in bluediag, including connection, handshake, glucose and scan timestamps.";
        result["interpretation"] = "Compare success/failure timestamps: older failures do not describe the current state. A callback object or attempted connection does not establish a live link. glucose_failure_at is the callback's last invalid/failed glucose observation, not a reconstruction of historical sensor-error periods. protocol_generation is internal, not the marketed model number; use juggluco_sensor for type. Present glucose without Rate is not a sensor error.";
    } else {
        result["settings"] = settings_configuration("garmin", cancel);
        result["source"] = "AllData.getGarminDeviceInfos(), the cached phone-side view used by GarminStatus.";
        for (auto& watch : result["watches"]) {
            const auto direct = watch.value("libre3_direct", Json(nullptr));
            const auto active = watch.value("active", Json(nullptr));
            watch["sensor_role"] = direct == true ? "selected_direct_libre3_receiver" :
                direct == false ? "not_selected_for_direct_libre3" : "unknown";
            watch["direct_sensor_route_configured"] = direct == false || active == false ? Json(false) :
                direct == true && active == true ? Json(true) : Json(nullptr);
            watch["configured_sensor_data_path"] = direct == true ? Json::array({"sensor", "this_garmin_watch", "phone"}) : Json(nullptr);
        }
        result["interpretation"] = "libre3_direct=true identifies the Garmin selected for direct Libre 3 sensor reception: sensor -> that Garmin -> phone. active controls phone-watch communication; watch_stopped is a cached Kerfstok stop observation. The Garmin return path is independent of Mirrors and can supply phone glucose with phone sensor Bluetooth disabled and every mirror receive=false. send_glucose controls phone-to-watch glucose, not watch-to-phone sensor return. transport_mode: 0 automatic, 1 Garmin Connect, 2 direct BLE. direct_ble and connected describe phone-to-watch transport, not sensor-to-watch BLE. last_received_at includes any watch message; glucose acknowledgements acknowledge phone-to-watch delivery, not glucose received from the sensor. Report the selected receiver and configured path using these fields; configuration alone does not prove the watch's sensor link is live now. An unavailable/empty cache is not proof that no watches are paired. No remote battery or live sensor session is fetched.";
    }
    jgchat::diagnostic("activity section=%s status=%s stale=%d", section.c_str(),
        result.value("status", std::string("unavailable")).c_str(), int(result.value("stale", true)));
    return result;
}
}
