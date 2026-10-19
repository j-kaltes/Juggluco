// SPDX-License-Identifier: GPL-3.0-or-later
// Source adapters for the same stores used by Juggluco's settings/sensor UI.
#include "jgchat/ui_messages.hpp"
#include "juggluco_system.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <mutex>
#include <span>
#include <unordered_set>
#include "config.h"
#include "sensoren.hpp"
#include "datbackup.hpp"

extern std::array<std::atomic_bool, maxallhosts> mirrordirectbluetoothactive;
extern std::mutex caliMutex;

namespace jgchatdata {
namespace {
using jgchat::Json;
void check(const std::atomic_bool* cancel) {
    if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
}
Json timestamp(uint32_t at) { return at ? Json(at) : Json(nullptr); }
template<std::size_t N> std::string text(const char (&value)[N]) { return {value, strnlen(value, N)}; }
Json ringing(const ring& r, bool loss = false) {
    return {{"sound", !r.nosound}, {"vibration", !r.novibration}, {"flash", bool(r.flash)},
        {"override_android_volume", bool(r.disturb)}, {"duration_seconds", r.duration},
        {loss ? "no_reading_delay_minutes" : "suspension_minutes", unsigned(r.wait)},
        {"custom_sound_selected", r.uri[0] != '\0'}};
}
template<class T> Json profile(const T& p, int id, const Tings& base) {
    Json thresholds = Json::array();
    const char* names[] = {"low", "high", "very_low", "very_high", "predicted_low", "predicted_high"};
    const bool enabled[] = {p.lowalarm, p.highalarm, p.verylowalarm, p.veryhighalarm, p.prelowalarm, p.prehighalarm};
    const uint32_t values[] = {p.alow, p.ahigh, p.averylow, p.averyhigh, p.aprelow, p.aprehigh};
    const int kinds[] = {0, 1, 5, 6, 7, 8};
    for (int i = 0; i < 6; ++i) {
        const auto& r = id ? p.alarms[kinds[i]] : kinds[i] < maxalarms ? base.alarms[kinds[i]] : base.extraAlarms[kinds[i] - maxalarms];
        thresholds.push_back({{"kind", names[i]}, {"enabled", enabled[i]}, {"threshold_mg_dL", values[i] / 10.0}, {"ring", ringing(r)}});
    }
    const auto& loss = id ? p.alarms[4] : base.alarms[4];
    const auto& available = id ? p.alarms[2] : base.alarms[2];
    return {{"profile_id", id}, {"threshold_unit", "mg/dL"}, {"glucose_alarms", thresholds},
        {"loss_of_signal", {{"enabled", p.lossalarm}, {"ring", ringing(loss, true)}}},
        {"available_glucose", {{"enabled", p.availablealarm}, {"ring", ringing(available)}}},
        {"speak_alarms", bool(p.speakalarms)}, {"sound_stream_setting", p.alarmSoundType}};
}
struct ReadingBounds { uint32_t first{}, last{}; };
template <typename TimeAt>
ReadingBounds reading_bounds(int begin, int end, const TimeAt& time_at, const std::atomic_bool* cancel) {
    ReadingBounds bounds;
    for (int pos = begin; pos < end; ++pos) {
        if ((pos & 255) == 0) check(cancel);
        if (const auto t = time_at(pos)) {
            if (!bounds.first || t < bounds.first) bounds.first = t;
            bounds.last = std::max(bounds.last, t);
        }
    }
    return bounds;
}
ReadingBounds scan_bounds(std::span<const ScanData> data, const std::atomic_bool* cancel) {
    return reading_bounds(0, static_cast<int>(data.size()), [&](int pos) -> uint32_t {
        // valid(0) uses native glucose validation without valid(pos)'s optional
        // timestamp repair, which would modify the mapped data during a query.
        return data[pos].valid(0) ? data[pos].gettime() : 0;
    }, cancel);
}
SensorSnapshot read_sensor(int index, const std::atomic_bool* cancel) {
    check(cancel);
    SensorSnapshot s; s.index = index;
    const auto record = sensors->sensorlist()[index];
    const auto* id = record.shortsensorname();
    s.id.assign(id->data(), strnlen(id->data(), id->size()));
    s.finished = record.finished;
    const auto* hist = sensors->getSensorData(index);
    if (!hist) { s.available = false; return s; }
    s.display_name = hist->showsensorname(); s.hidden = hist->hide; s.unused = hist->unused();
    s.start = hist->getstarttime(); s.wear_seconds = hist->getweardurationSEC();
    const auto* info = hist->getinfo();
    s.stored_wear_seconds = 60U * ((hist->isLibre2() || hist->isDexcom() || hist->isAidexX() ||
        hist->isAccuChek() || hist->isAir()) ? info->wearduration : info->wearduration2);
    s.stored_history_interval = info->interval;
    s.expected_seconds = hist->expectedWearDuration(); s.warmup_seconds = hist->getWarmupSEC();
    const auto stream = scan_bounds(hist->getPolldata(), cancel);
    const auto scans = scan_bounds(hist->getScandata(), cancel);
    const auto history = hist->hasRealHistory() ?
        reading_bounds(std::max(0, hist->getstarthistory()), std::min(hist->getAllendhistory(), hist->maxpos()),
            [&](int pos) -> uint32_t {
                const auto* value = hist->getglucose(pos);
                return value->valid() ? value->gettime() : 0;
            }, cancel) : ReadingBounds{};
    s.last_stream = stream.last; s.last_scan = scans.last; s.last_history = history.last;
    for (const auto first : {stream.first, scans.first, history.first})
        if (first && (!s.first_glucose || first < s.first_glucose)) s.first_glucose = first;
    s.stream_interval = hist->getsecstreaminterval(); s.history_interval = hist->getinterval();
    if (hist->isDexcom()) s.family = SensorFamily::dexcom;
    else if (hist->isSibionics()) { s.family = SensorFamily::sibionics; s.new_si = hist->newSI(); s.si_subtype = hist->siSubtype(); }
    else if (hist->isAccuChek()) s.family = SensorFamily::accu_chek;
    else if (hist->isAidexX()) { s.family = SensorFamily::aidex_x; s.restart = hist->getRestartTime(); }
    else if (hist->isAir()) s.family = SensorFamily::caresens_air;
    else if (hist->isLibre3()) s.family = SensorFamily::libre3;
    else if (hist->isLibre()) { s.family = SensorFamily::libre; s.libre_protocol_generation = hist->getsensorgen(); }
    return s;
}
// Compare the exact bounded export ID across the entire index. Only matched
// entries open their sensor data files; older/unrelated sensors are not paged
// through model calls. Preserve multiple exact matches instead of guessing.
std::vector<int> matching_sensor_indices(std::span<const sensor> entries,
        const std::vector<std::string>& ids, const std::atomic_bool* cancel) {
    const std::unordered_set<std::string_view> wanted(ids.begin(), ids.end());
    std::vector<int> found;
    for (std::size_t end = entries.size(); end && !wanted.empty(); --end) {
        check(cancel);
        const auto* id = entries[end - 1].shortsensorname();
        if (wanted.contains(std::string_view(id->data(), strnlen(id->data(), id->size()))))
            found.push_back(static_cast<int>(end - 1));
    }
    return found;
}
uint32_t sensor_count() {
    const int last = sensors->last();
    if (last < -1) throw jgchat::UiError(jgchat::UiCode::invalid_sensor_count);
    return uint32_t(last) + 1;
}
CalibrationSeries copy_calibrations(const Calibraties& stored) {
    CalibrationSeries result;
    result.stored_count = stored.caliNr;
    result.valid_count = stored.caliNr <= std::size(stored.caliPara);
    if (result.valid_count) {
        result.rows.reserve(stored.caliNr);
        for (uint32_t i = 0; i < stored.caliNr; ++i) {
            const auto& c = stored.caliPara[i];
            result.rows.push_back({c.time, c.a, c.b});
        }
    }
    return result;
}
}
SensorPage read_sensor_page(uint32_t offset, uint32_t limit, const std::atomic_bool* cancel) {
    SensorPage out;
    if (!sensors) return out;
    out.available = true; out.total = sensor_count();
    for (uint64_t pos = offset; pos < out.total && out.rows.size() < limit; ++pos)
        out.rows.push_back(read_sensor(static_cast<int>(out.total - pos - 1), cancel));
    return out;
}
SensorPage read_sensors_by_id(const std::vector<std::string>& ids, const std::atomic_bool* cancel) {
    SensorPage out;
    if (!sensors) return out;
    out.available = true; out.total = sensor_count();
    const auto indices = matching_sensor_indices({sensors->sensorlist(), out.total}, ids, cancel);
    for (const auto index : indices) out.rows.push_back(read_sensor(index, cancel));
    return out;
}
SensorCalibrations read_calibrations(const std::string& id, std::optional<uint32_t> index,
        const std::atomic_bool* cancel) {
    check(cancel);
    SensorCalibrations out;
    if (!sensors) { out.reason = "Sensor store is not initialized."; return out; }
    const auto matches = matching_sensor_indices({sensors->sensorlist(), sensor_count()}, {id}, cancel);
    out.matching_indices.assign(matches.begin(), matches.end());
    if (matches.empty()) { out.status = "not_found"; out.reason = "Exact sensor ID is absent from the full local index."; return out; }
    if (index && std::find(matches.begin(), matches.end(), static_cast<int>(*index)) == matches.end()) {
        out.status = "sensor_index_mismatch";
        out.reason = "The requested index does not match this sensor ID."; return out;
    }
    if (!index && matches.size() != 1) {
        out.status = "ambiguous";
        out.reason = "Multiple records share this ID; repeat with one of matching_sensor_indices."; return out;
    }
    out.sensor_index = index ? *index : static_cast<uint32_t>(matches.front());
    // Use the same mutex as native calibration calculation/synchronization.
    // Copy only the stored arrays; do not recalculate or alter calibration.
    {
        std::lock_guard lock(caliMutex);
        check(cancel);
        const auto* hist = sensors->getSensorData(*out.sensor_index);
        if (!hist) { out.reason = "Sensor calibration metadata could not be opened."; return out; }
        out.has_real_history = hist->hasRealHistory();
        const auto* info = hist->getinfo();
        out.sources[0] = copy_calibrations(info->calis[0]);
        out.sources[1] = copy_calibrations(info->calis[1]);
    }
    if (settings) {
        out.settings_available = true;
        out.enabled = settings->data()->DoCalibrate;
        out.past_values = settings->data()->CalibratePast;
    }
    out.status = "ok";
    return out;
}
Json mirror_configuration(const std::atomic_bool* cancel) {
    check(cancel);
    if (!backup || !settings) return {{"status", "unavailable"}, {"reason", "Mirror/settings store is not initialized."}};
    std::lock_guard lock(change_host_mutex);
    const auto* data = backup->getupdatedata();
    if (data->hostnr < 0 || data->hostnr > maxallhosts || data->sendnr > maxallhosts)
        throw jgchat::UiError(jgchat::UiCode::invalid_mirror_count);
    Json hosts = Json::array();
    for (int i = 0; i < data->hostnr; ++i) {
        check(cancel);
        const auto& h = data->allhosts[i];
        const bool sending = h.index >= 0 && h.index < data->sendnr;
        const auto* send = sending ? data->tosend + h.index : nullptr;
        const char* names[] = {"automatic", "tcp_ip", "wear_os_messages", "direct_ble"};
        Json host{{"index", i}, {"label", h.hasname ? Json(std::string(h.getname(), strnlen(h.getname(), passhost_t::maxnamelen))) : Json(nullptr)},
            {"enabled", !h.deactivated}, {"wear_os", bool(h.wearos)}, {"transport", h.ICE ? "ice" : names[h.gettransport()]},
            {"receive", h.receivedatafrom()}, {"send", {{"amounts", send && send->sendnums}, {"stream", send && send->sendstream}, {"scans", send && send->sendscans}}},
            {"active_only", h.getActive()}, {"passive_only", h.getPassive()}, {"detect_address", bool(h.detect)},
            {"password_configured", h.haspass()}, {"last_sync_at", timestamp(lastuptodate[i])}};
        // ICE rendezvous labels/passwords and raw failure text can contain secrets.
        if (!h.ICE) {
            host["port"] = h.getport();
            host["hostname"] = h.hashostname() ? Json(std::string(h.gethostname(), strnlen(h.gethostname(), passhost_t::hostnamedata::maxhostname))) : Json(nullptr);
            Json addresses = Json::array();
            if (!h.hashostname()) for (int n = 0; n < std::clamp(h.nr, 0, passhost_t::maxip - int(h.hasname)); ++n) {
                char address[INET6_ADDRSTRLEN]{};
                if (inet_ntop(AF_INET6, &h.ips[n].sin6_addr, address, sizeof(address))) addresses.push_back(address);
            }
            host["addresses"] = std::move(addresses);
        }
#ifdef WEAROS_MESSAGES
        host["observed_message_bridge"] = wearmessages[i].load();
        host["observed_direct_ble"] = mirrordirectbluetoothactive[i].load();
#endif
        hosts.push_back(std::move(host));
    }
    return {{"status", "ok"}, {"generated_at", std::time(nullptr)}, {"mirror_listen_port", text(data->port)},
        {"phone_sensor_bluetooth_enabled", !settings->data()->nobluetooth}, {"wear_os_enabled", bool(settings->data()->useWearos)},
        {"mirrors", std::move(hosts)}, {"credentials", "Passwords, keys, ICE rendezvous labels and raw diagnostics are omitted."}};
}
Json alarm_configuration(const std::atomic_bool* cancel) {
    check(cancel);
    if (!settings) return {{"status", "unavailable"}, {"reason", "Settings store is not initialized."}};
    const auto& s = *settings->data();
    if (s.nrProfile < 0 || s.nrProfile > maxprofiles || s.nrProfileMins < 0 || s.nrProfileMins > maxprofileMins ||
        s.alarmnr < 0 || s.alarmnr > maxnumalarms)
        throw jgchat::UiError(jgchat::UiCode::invalid_alarm_settings_counts);
    Json profiles = Json::array({profile(s, 0, s)}), schedule = Json::array(), amounts = Json::array();
    for (int i = 0; i < s.nrProfile; ++i) { check(cancel); profiles.push_back(profile(s.profiles[i], i + 1, s)); }
    for (int i = 0; i < s.nrProfileMins; ++i) schedule.push_back({{"minute_of_local_day", s.profileMins[i].min}, {"profile_id", s.profileMins[i].profile}});
    for (int i = 0; i < s.alarmnr; ++i) {
        const auto& a = s.numalarm[i];
        amounts.push_back({{"label_id", a.type}, {"value", a.value}, {"start_minute_of_day", a.start},
            {"alarm_minute_of_day", a.alarm}, {"end_minute_of_day", a.end}});
    }
    return {{"status", "ok"}, {"generated_at", std::time(nullptr)}, {"active_profile_id", s.currentProfile},
        {"active_profile_valid", s.currentProfile >= 0 && s.currentProfile <= s.nrProfile},
        {"profiles", std::move(profiles)}, {"daily_profile_schedule", std::move(schedule)},
        {"amount_reminders", std::move(amounts)}, {"amount_reminder_ring", ringing(s.alarms[3])},
        {"scope", "Current local phone settings, not historical settings or a log of alarms that actually sounded. Android permissions, DND, volume and remote-watch alarms are not established by this snapshot. Profiles use local phone time; amount reminders keep original label units."},
        {"prediction", "Predicted high/low uses Juggluco's own projection rule; these thresholds are alarm settings, not recommendations."},
        {"sound_stream_values", "Raw Juggluco alarmSoundType enum; ringtone URI is not exported."}};
}
}
