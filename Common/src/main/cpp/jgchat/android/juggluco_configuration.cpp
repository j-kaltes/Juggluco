// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only, explicitly selected fields from Juggluco's existing settings.
#include "jgchat/ui_messages.hpp"
#include "juggluco_system.hpp"
#include "juggluco_extra.hpp"
#include "config.h"
#include "settings/settings.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string_view>

namespace jgchatdata {
namespace {
using jgchat::Json;
template<std::size_t N> std::string setting_text(const char (&s)[N]) { return {s, strnlen(s, N)}; }
Json finite_setting(double value) { return std::isfinite(value) ? Json(value) : Json(nullptr); }
Json setting_time(uint32_t t) { return t ? Json(t) : Json(nullptr); }
void count_ok(int64_t n, std::size_t maximum) {
    if (n < 0 || uint64_t(n) > maximum) throw jgchat::UiError(jgchat::UiCode::invalid_settings_count);
}
// URLs can carry credentials in userinfo, paths, queries or fragments. Only
// report a validated http(s) origin and whether those other components exist.
Json endpoint_summary(std::string_view url) {
    Json out{{"configured", !url.empty()}, {"origin", nullptr}};
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string_view::npos || (url.substr(0, scheme_end) != "http" && url.substr(0, scheme_end) != "https")) return out;
    if (url.find_first_of("\\\r\n\t") != std::string_view::npos) return out;
    auto authority = url.substr(scheme_end + 3);
    const auto path = authority.find_first_of("/?#");
    out["path_query_or_fragment_omitted"] = path != std::string_view::npos;
    authority = authority.substr(0, path);
    const auto at = authority.rfind('@');
    out["userinfo_omitted"] = at != std::string_view::npos;
    if (at != std::string_view::npos) authority.remove_prefix(at + 1);
    if (authority.empty() || authority.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-:[]") != std::string_view::npos) return out;
    out["origin"] = std::string(url.substr(0, scheme_end + 3)) + std::string(authority);
    return out;
}
template<int N> Json recipients(const BroadcastListeners<N>& b) {
    count_ok(b.nr, N);
    Json names = Json::array();
    for (int i = 0; i < b.nr; ++i) names.push_back(setting_text(b.name[i]));
    return {{"enabled", b.nr > 0}, {"recipient_packages", std::move(names)}};
}
template<class T> Json talk_profile(const T& p, int id) {
    return {{"profile_id", id}, {"automatic_glucose_speech", bool(p.voiceactive)},
        {"minimum_separation_seconds", unsigned(p.voicesep)}, {"speed", finite_setting(p.voicespeed)},
        {"pitch", finite_setting(p.voicepitch)}, {"voice_index", p.voicespeaker},
        {"android_audio_usage", p.soundtype}, {"touch_to_speak", bool(p.talktouch)},
        {"speak_messages", bool(p.speakmessages)}, {"speak_alarms", bool(p.speakalarms)}};
}
template<class T> Json display_profile(const T& p, int id) {
    return {{"profile_id", id}, {"theme_id", p.Theme}, {"invert_colors", bool(p.invertcolors)},
        {"round_display", bool(p.isOval)}, {"radius_setting", p.radius}};
}
const char* category_name(int n) {
    switch (n) { case 0: return "none"; case 1: return "rapid_insulin";
        case 2: return "long_acting_insulin"; case 3: return "carbohydrate";
        case 4: return "note"; default: return "unknown"; }
}
Json label_mapping(const Tings::ToLibre& m) {
    return {{"category_id", m.kind}, {"category", category_name(m.kind)}, {"weight", finite_setting(m.weight)},
        {"carbohydrate_grams_per_value", m.kind == 3 ? finite_setting(m.weight) : Json(nullptr)}};
}
}

// Pure formatter also exercised against the real on-disk types in host tests.
Json configuration_snapshot(const Tings& s, std::string_view section) {
    Json result{{"status", "ok"}, {"section", section}, {"generated_at", std::time(nullptr)},
        {"scope", "Current saved local settings, not historical settings or proof a service is running. Read only; no connection, upload, scan or setting change is triggered."}};
    if (section == "bluetooth") {
        result.update({{"use_bluetooth", !s.nobluetooth}, {"balanced_connection_priority", bool(s.balanced_priority)},
            {"stream_history", bool(s.streamHistory)}, {"alarm_clock_enabled", !s.noalarmclock},
            {"disconnect_sensor", bool(s.DisconnectSensor)},
            {"meaning", "use_bluetooth is Juggluco's preference for direct phone-to-sensor reception, not the Android Bluetooth radio state. Garmin direct sensor handoff can disable it while the phone still communicates with the Garmin. For the configured receiver, Garmin status and mirror directions read juggluco_devices."}});
    } else if (section == "garmin") {
        result.update({{"enabled", bool(s.usegarmin)}, {"has_garmin_setting", bool(s.hasgarmin)},
            {"default_dark_mode", bool(s.kerfstokblack)},
            {"meaning", "Phone-side Garmin preferences. Individual cached watch preferences and delivery observations are returned by juggluco_activity(section=garmin)."}});
    } else if (section == "glucose_meters") {
        count_ok(s.glucoseMeterNR, maxglucosemeters);
        Json rows = Json::array();
        for (uint32_t i = 0; i < s.glucoseMeterNR; ++i) {
            const auto& m = s.glucosemeters[i];
            rows.push_back({{"index", i}, {"name", setting_text(m.deviceName)}, {"active", bool(m.active)},
                {"time_offset_seconds", m.timeoffset}, {"last_recorded_time", setting_time(m.lastTime)},
                {"next_record_index", m.nextIndex}, {"address_configured", std::any_of(std::begin(m.deviceAddress), std::end(m.deviceAddress), [](auto b) { return b != 0; })}});
        }
        result["meters"] = std::move(rows);
        result["blood_glucose_label_id"] = s.bloodvar;
        result["meaning"] = "Configured Bluetooth glucose meters. Active is a preference, not a live connection. last_recorded_time and next_record_index are native import progress, not time of successful connection.";
    } else if (section == "broadcasts") {
        result["librelink"] = recipients(s.librelinkBroadcast);
        result["eversense"] = recipients(s.everSenseBroadcast);
        result["xdrip"] = recipients(s.xdripBroadcast);
        result["glucodata"] = recipients(s.glucodataBroadcast);
        result["gadgetbridge"] = s.gadgetbridge;
        result["xinfuus"] = bool(s.xinfuus);
        result["health_connect"] = bool(s.healthConnect);
        result["watchdrip"] = bool(s.watchdrip);
        result["meaning"] = "Recipient package names are configured destinations, not evidence that the app is installed, granted permissions or receiving broadcasts.";
    } else if (section == "libreview") {
        result.update({{"enabled", bool(s.uselibre)}, {"send_to_libreview", bool(s.sendtolibreview)},
            {"send_numbers", bool(s.sendnumbers)}, {"current_only", bool(s.LibreCurrentOnly)},
            {"viewed_flag", bool(s.libreIsViewed)}, {"stored_region_code", s.librecountry},
            {"effective_region_index", s.librecountry >= 1 && s.librecountry <= 5 ? s.librecountry - 1 : s.unit == 1 ? 0 : 1},
            {"email_configured", s.libreemail[0] != 0}, {"password_configured", s.librepasslen > 0},
            {"libre2_session_configured", s.tokensize > 0}, {"libre3_session_configured", s.tokensize3 > 0},
            {"libre2_initialized", bool(s.libreinit)}, {"libre3_initialized", bool(s.libreinit3)},
            {"libre2_endpoint", endpoint_summary(setting_text(s.librebaseurl))},
            {"libre3_endpoint", endpoint_summary(setting_text(s.libre3baseurl))},
            {"last_libre2_glucose_progress_at", setting_time(s.lastlibretime)},
            {"upload_from", setting_time(s.startlibretime)},
            {"number_mappings_tool", "juggluco_settings(section=numbers)"},
            {"meaning", "Session configured does not establish authentication validity. Glucose progress is the recorded sample timestamp, not a last successful HTTP response or the Libre 3 upload cursor."}});
    } else if (section == "web_server") {
        result.update({{"enabled", bool(s.usexdripwebserver)}, {"remote_access_allowed", bool(s.remotelyxdripserver)},
            {"http_port", s.httpport}, {"https_enabled", bool(s.useSSL)}, {"https_port", s.sslport},
            {"api_secret_configured", s.apisecretlength > 0}, {"glucose_interval_seconds", s.nightinterval},
            {"give_amounts_as_treatments", bool(s.GiveAmounts)},
            {"meaning", "GiveAmounts controls whether the Nightscout-compatible web commands return entered Amounts as treatments. It is independent of Talk/speech and the Nightscout uploader's send_treatments setting. Enabled and ports are configuration, not a listening/reachability test. The API secret and authenticated URL paths are omitted."}});
    } else if (section == "uploader") {
        result.update({{"enabled", s.nightuploadon}, {"nightscout_v3", bool(s.nightscoutV3)},
            {"endpoint", endpoint_summary(setting_text(s.nightuploadname))},
            {"secret_configured", s.nightuploadsecret[0] != 0}, {"send_treatments", bool(s.postTreatments)},
            {"last_completed_treatment_pass_at", setting_time(s.lastuploadtime)},
            {"meaning", "The treatment pass timestamp is not a glucose-upload receipt or evidence the server is reachable now. Credentials, URL path/query/fragment and raw HTTP responses are omitted."}});
    } else if (section == "display") {
        count_ok(s.nrProfile, maxprofiles);
        Json profiles = Json::array({display_profile(s, 0)});
        for (int i = 0; i < s.nrProfile; ++i) profiles.push_back(display_profile(s.profiles[i], i + 1));
        result.update({{"glucose_unit", s.unit == 1 ? "mmol/L" : "mg/dL"},
            {"graph_low_mg_dL", s.glow / 10.0}, {"graph_high_mg_dL", s.ghigh / 10.0},
            {"target_low_mg_dL", s.tlow / 10.0}, {"target_high_mg_dL", s.thigh / 10.0},
            {"duration_seconds", s.duration}, {"fixed_time_axis", bool(s.fixatex)}, {"fixed_glucose_axis", bool(s.fixatey)},
            {"orientation_setting", unsigned(s.orientation)}, {"system_ui", bool(s.systemUI)},
            {"keep_screen_on", !s.dontshowalways}, {"glucose_axis_left", bool(s.levelleft)}, {"hour24", bool(s.hour24)},
            {"round_to", finite_setting(s.roundto)}, {"scans", bool(s.showscans)}, {"stream", bool(s.showstream)},
            {"history", bool(s.showhistories)}, {"numbers", bool(s.shownumbers)}, {"meals", bool(s.showmeals)},
            {"calibrated_scans", bool(s.showcalibratedscans)}, {"calibrated_stream", bool(s.showcalibratedstream)},
            {"calibrated_history", bool(s.showcalibratedhistories)}, {"calibration_enabled", bool(s.DoCalibrate)},
            {"calibrate_past", bool(s.CalibratePast)}, {"iob_visible", bool(s.IOB)},
            {"floating_glucose", {{"enabled", s.floatglucose != 0}, {"font_size", s.floatingFontsize},
                {"foreground_argb", s.floatingforeground}, {"background_argb", s.floatingbackground},
                {"hide_inside_juggluco", bool(s.hidefloatinJuggluco)}, {"show_time", bool(s.floattime)},
                {"not_touchable", bool(s.floatingNotTouchable)}}},
            {"active_profile_id", s.currentProfile}, {"active_profile_valid", s.currentProfile >= 0 && s.currentProfile <= s.nrProfile},
            {"profiles", std::move(profiles)}, {"meaning", "Saved display settings; not a screenshot or the current pan/zoom position. Theme and orientation are native enum values. Ranges use mg/dL regardless of display unit."}});
    } else if (section == "numbers") {
        count_ok(s.varcount, maxvarnr); count_ok(s.shortnr, s.shorts.size());
        Json labels = Json::array(), shortcuts = Json::array();
        const char* insulin_names[] = {"none", "human", "aspart", "lispro", "glulisine", "fiasp", "ultra_rapid_lispro", "afrezza"};
        for (int i = 0; i < s.varcount; ++i) {
            const auto insulin = unsigned(s.insulintypes[i]);
            labels.push_back({{"id", i}, {"name", setting_text(s.vars[i].name)},
                {"precision", finite_setting(s.vars[i].prec)}, {"display_weight_mg_L", finite_setting(s.vars[i].weight)},
                {"web_mapping", label_mapping(s.Nightnums[i])}, {"libreview_mapping", label_mapping(s.librenums[i])},
                {"iob_insulin_id", insulin}, {"iob_insulin", insulin < std::size(insulin_names) ? insulin_names[insulin] : "unknown"}});
        }
        for (int i = 0; i < s.shortnr; ++i) shortcuts.push_back({{"name", setting_text(s.shorts[i].name)}, {"value", setting_text(s.shorts[i].value)}});
        result.update({{"labels", std::move(labels)}, {"shortcuts", std::move(shortcuts)},
            {"meal_label_id", s.mealvar}, {"blood_glucose_label_id", s.bloodvar}, {"iob_enabled", bool(s.IOB)},
            {"numbers_read_only", s.nochangenum},
            {"meaning", "Label precision and display weight control number entry/drawing; display weight is not an insulin sensitivity or dose recommendation. For a carbohydrate web/LibreView mapping, weight is grams per entered value; do not apply it as an insulin dose multiplier. Current mappings also apply when interpreting older entries."}});
    } else if (section == "talk") {
        count_ok(s.nrProfile, maxprofiles); count_ok(s.nrProfileMins, maxprofileMins);
        Json profiles = Json::array({talk_profile(s, 0)}), schedule = Json::array();
        for (int i = 0; i < s.nrProfile; ++i) profiles.push_back(talk_profile(s.profiles[i], i + 1));
        for (int i = 0; i < s.nrProfileMins; ++i) schedule.push_back({{"minute_of_local_day", s.profileMins[i].min}, {"profile_id", s.profileMins[i].profile}});
        result.update({{"active_profile_id", s.currentProfile}, {"active_profile_valid", s.currentProfile >= 0 && s.currentProfile <= s.nrProfile},
            {"profiles", std::move(profiles)}, {"daily_profile_schedule", std::move(schedule)},
            {"meaning", "Saved per-profile Talk preferences; not proof speech played or the TTS engine, selected voice or audio route is available. The voice index is local to the Android voice list. Profile schedules use local phone time."}});
    } else throw jgchat::UiArgumentError(jgchat::UiCode::unknown_settings_section);
    return result;
}
Json settings_configuration(const std::string& section, const std::atomic_bool* cancel) {
    if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
    if (!settings) return {{"status", "unavailable"}, {"reason", "Settings store is not initialized."}};
    if (section == "activities") {
        Json labels = Json::array();
        count_ok(settings->getlabelcount(), maxvarnr);
        for (int i = 0; i < settings->getlabelcount(); ++i)
            labels.push_back({{"id", i}, {"name", settings->getlabel(i)}});
        return activity_settings(labels, settings->staticnum(), cancel);
    }
    return configuration_snapshot(*settings->data(), section);
}
}
