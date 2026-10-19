// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only local data dispatcher, reused by the native model client.
#include "jgchat/ui_messages.hpp"
#include "juggluco_data.hpp"
#include "juggluco_extra.hpp"
#include "jgchat/meal_layout.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef JGICE_DATA_TEST_BACKEND
#include "data_test_backend.hpp"
#else
#include "settings/settings.hpp"
#include "nums/numdata.hpp"
static_assert(sizeof(jgchatdata::MealHeader) == offsetof(mealdata, themeals));
static_assert(sizeof(jgchatdata::MealIngredient) == sizeof(ingredient_t));
static_assert(offsetof(jgchatdata::MealHeader, ingredients) == offsetof(mealdata, ingredients));
static_assert(offsetof(jgchatdata::MealHeader, units) == offsetof(mealdata, units));
static_assert(sizeof(jgchatdata::MealElement) == sizeof(mealdata::mealel));
class Sensoren;
extern std::vector<Numdata*> numdatas;
extern Sensoren *sensors;
extern double getiob(uint32_t now);
extern std::span<char> getstream(int, int, uint32_t, uint32_t, bool, int, bool, int, bool, bool);
extern std::span<char> gethistory(int, int, uint32_t, uint32_t, bool, int, bool, int, bool, bool);
extern std::span<char> getscans(int, int, uint32_t, uint32_t, bool, int, bool, int, bool, bool);
#endif

namespace jgchatdata {
namespace {
constexpr uint32_t iob_lookback = 16U * 60U * 60U;
using Args = std::vector<std::pair<std::string, std::string>>;

std::string quote(std::string_view in) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char c : in) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 32) {
            out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15];
        } else out += char(c);
    }
    return out + '"';
}

std::string number(double value) {
    if (!std::isfinite(value)) return "null";
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(10) << value;
    return out.str();
}

std::string response(unsigned status, std::string_view body,
                     std::string_view type = "application/json; charset=utf-8",
                     std::string_view extra = {}) {
    const char *reason = status == 200 ? "OK" : status == 400 ? "Bad Request" :
        status == 404 ? "Not Found" : status == 413 ? "Content Too Large" :
        status == 503 ? "Service Unavailable" : "Internal Server Error";
    std::string out = "HTTP/1.1 " + std::to_string(status) + " " + reason +
        "\r\nContent-Type: " + std::string(type) +
        "\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: " +
        std::to_string(body.size()) + "\r\n" + std::string(extra) + "\r\n";
    out.append(body);
    return out;
}

std::string error(unsigned status, std::string_view code, std::string_view message) {
    return response(status, "{\"error\":{\"code\":" + quote(code) +
                    ",\"message\":" + quote(message) + "}}");
}

int unhex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool decode(std::string_view in, std::string &out) {
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = in[i];
        if (c == '%') {
            if (i + 2 >= in.size()) return false;
            const int a = unhex(in[i + 1]), b = unhex(in[i + 2]);
            if (a < 0 || b < 0) return false;
            c = static_cast<unsigned char>((a << 4) | b); i += 2;
        }
        if (c <= 32 || c >= 127 || c == '+' || c == '#' || c == '&' || c == '=') return false;
        out += char(c);
    }
    return true;
}

bool parse(std::string_view target, std::string_view &path, Args &args) {
    if (target.empty() || target.size() > 2048) return false;
    for (unsigned char c : target) if (c <= 32 || c >= 127 || c == '#') return false;
    const size_t q = target.find('?');
    path = target.substr(0, q);
    if (q == std::string_view::npos) return true;
    auto query = target.substr(q + 1);
    if (query.empty()) return false;
    while (!query.empty()) {
        const size_t end = query.find('&');
        const auto part = query.substr(0, end);
        const size_t eq = part.find('=');
        if (eq == std::string_view::npos || !eq || eq + 1 == part.size()) return false;
        std::string key, value;
        if (!decode(part.substr(0, eq), key) || !decode(part.substr(eq + 1), value)) return false;
        for (const auto &arg : args) if (arg.first == key) return false;
        args.emplace_back(std::move(key), std::move(value));
        if (args.size() > 8) return false;
        if (end == std::string_view::npos) break;
        query.remove_prefix(end + 1);
        if (query.empty()) return false;
    }
    return true;
}

bool uintarg(std::string_view text, uint32_t &value) {
    if (text.empty()) return false;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc() && end == text.data() + text.size();
}

const char *web_category(int id) {
    switch (id) {
        case 0: return "unspecified";
        case 1: return "rapid_insulin";
        case 2: return "long_insulin";
        case 3: return "carbohydrate";
        case 4: return "note";
        default: return "unknown";
    }
}

const char *insulin_name(unsigned id) {
    static constexpr const char *names[] = {"Not", "Human", "Aspart", "Lispro",
                                            "Glulisine", "Fiasp", "URli", "Afrezza"};
    return id < std::size(names) ? names[id] : "unknown";
}

std::string iob_json(uint32_t at) {
    const bool enabled = settings->data()->IOB;
    bool configured = false, valid = true;
    for (int i = 0; i < settings->getlabelcount(); ++i) {
        const unsigned type = static_cast<unsigned>(settings->getIOBtype(i));
        configured |= type != 0;
        valid &= type < insulinsNR();
    }
    const char *state = !enabled ? "disabled" : !valid ? "invalid_insulin_category" :
                        !configured ? "no_insulin_labels" : "ok";
    std::string value = "null";
    if (enabled && valid && configured) {
        // This is Juggluco's own calculation, not a reimplementation.
        const double calculated = getiob(at);
        if (std::isfinite(calculated)) value = number(calculated);
        else state = "calculation_unavailable";
    }
    return "{\"at\":" + std::to_string(at) + ",\"enabled\":" + (enabled ? "true" : "false") +
        ",\"configured\":" + (configured ? "true" : "false") + ",\"status\":" + quote(state) +
        ",\"value\":" + value + ",\"unit\":\"U\",\"computed_by\":\"Juggluco\"," +
        "\"lookback_seconds\":" + std::to_string(iob_lookback) +
        ",\"category_mapping\":\"current_settings\"}";
}

std::string metadata(uint32_t now) {
    const time_t stamp = now;
    struct tm local{};
    localtime_r(&stamp, &local);
    char zone[80]{};
    strftime(zone, sizeof(zone), "%Z", &local);
    std::string out = "{\"schema\":1,\"generated_at\":" + std::to_string(now) +
        ",\"glucose_unit\":" + quote(settings->getunitlabel()) +
        ",\"glucose_display\":" + glucose_display_preferences().dump() +
        ",\"timezone\":{\"abbreviation\":" + quote(zone) +
        ",\"utc_offset_seconds\":" + std::to_string(local.tm_gmtoff) +
        ",\"at\":" + std::to_string(now) +
        "},\"retrieval\":{\"date_range_cap\":null,\"limit_zero_returns_all\":true},\"meal_label_id\":" + std::to_string(settings->data()->mealvar) +
        ",\"iob\":" + iob_json(now) + ",\"labels\":[";
    for (int i = 0; i < settings->getlabelcount(); ++i) {
        if (i) out += ',';
        const auto &web = settings->data()->Nightnums[i];
        const unsigned insulin = static_cast<unsigned>(settings->getIOBtype(i));
        out += "{\"id\":" + std::to_string(i) + ",\"name\":" + quote(settings->getlabel(i)) +
            ",\"web_category_id\":" + std::to_string(web.kind) +
            ",\"web_category\":" + quote(web_category(web.kind)) +
            ",\"web_weight\":" + number(web.weight) +
            ",\"carbohydrate_grams_per_value\":" + (web.kind == 3 ? number(web.weight) : "null") +
            ",\"iob_insulin_id\":" + std::to_string(insulin) +
            ",\"iob_insulin\":" + quote(insulin_name(insulin)) + "}";
    }
    out += "],\"calibration\":{\"default_export\":\"raw\",\"calibrated_option\":true,";
    out += "\"pastvalues_option\":true,\"stored_coefficients_tool\":\"juggluco_calibrations\"},\"time_semantics\":\"Unix seconds; start inclusive, end exclusive\"}";
    return response(200, out);
}

struct Period {
    uint32_t start{}, end{}, limit{};
    int unit{};
    bool calibrated{}, pastvalues{};
};

bool period(const Args &args, uint32_t now, bool glucose, Period &p, std::string &why) {
    p.end = now == UINT32_MAX ? now : now + 1;
    p.start = now > 86400 ? now - 86400 : 0;
    p.unit = settings->data()->unit == 1 ? 1 : 2;
    bool hasstart = false, hasend = false;
    for (const auto &[key, val] : args) {
        bool ok = true;
        if (key == "start") { hasstart = true; ok = uintarg(val, p.start); }
        else if (key == "end") { hasend = true; ok = uintarg(val, p.end); }
        else if (key == "limit") ok = uintarg(val, p.limit);
        else if (glucose && key == "unit") {
            if (val == "mmol/L") p.unit = 1;
            else if (val == "mg/dL") p.unit = 2;
            else ok = false;
        } else if (glucose && (key == "calibrated" || key == "pastvalues")) {
            ok = val == "0" || val == "1";
            if (key == "calibrated") p.calibrated = val == "1";
            else p.pastvalues = val == "1";
        } else { why = "Unknown query parameter: " + key; return false; }
        if (!ok) { why = "Invalid query value for: " + key; return false; }
    }
    if (hasstart != hasend) { why = "Supply both start and end, or neither."; return false; }
    if (p.end <= p.start) { why = "Require start < end."; return false; }
    if (glucose && p.limit > uint32_t(std::numeric_limits<int>::max())) {
        why = "The native exporter uses a signed int count; use limit=0 for all records."; return false;
    }
    if (p.pastvalues && !p.calibrated) { why = "pastvalues=1 requires calibrated=1."; return false; }
    return true;
}

std::string amounts(const Period &p, uint32_t now, bool meals_only = false) {
    struct Item { uint32_t at, type; float value; size_t source; int index; uint32_t meal_id; };
    struct Cursor { const Numdata *store; const Num *at, *end; size_t source; };
    // Match the app's writer lock while copying. Authentication belongs to the
    // outer transport; this function never changes the stores or categories.
    std::vector<std::unique_lock<std::mutex>> locks;
    for (auto *store : numdatas) if (store) locks.emplace_back(store->nummutex);
    std::vector<Cursor> cursors;
    for (size_t source = 0; source < numdatas.size(); ++source) {
        const auto *store = numdatas[source];
        if (!store) continue;
        const auto [begin, end] = store->getInRange(p.start, p.end);
        cursors.push_back({store, begin, end, source});
    }
    std::vector<Item> items;
    items.reserve(1024);
    while (!p.limit || items.size() <= p.limit) {
        Cursor *oldest = nullptr;
        for (auto &c : cursors) {
            while (c.at < c.end && (!c.store->valid(c.at) ||
                (meals_only && (c.at->type != settings->data()->mealvar ||
                    c.at->type == settings->data()->bloodvar || !c.at->mealptr || Num::issportref(c.at->mealptr))))) ++c.at;
            if (c.at < c.end && (!oldest || c.at->time < oldest->at->time)) oldest = &c;
        }
        if (!oldest) break;
        const Num copy = *oldest->at;
        const int index = oldest->store->getfirstpos() + int(oldest->at - oldest->store->begin());
        const uint32_t meal_id = copy.type == settings->data()->mealvar &&
            copy.type != settings->data()->bloodvar && !Num::issportref(copy.mealptr) ? copy.mealptr : 0;
        items.push_back({copy.time, copy.type, copy.value, oldest->source, index, meal_id});
        ++oldest->at;
    }
    locks.clear();
    const bool truncated = p.limit && items.size() > p.limit;
    if (truncated) items.resize(p.limit);
    std::string body = "{\"schema\":1,\"generated_at\":" + std::to_string(now) +
        ",\"start\":" + std::to_string(p.start) + ",\"end\":" + std::to_string(p.end) +
        ",\"end_exclusive\":true,\"returned\":" + std::to_string(items.size()) +
        ",\"truncated\":" + (truncated ? "true" : "false") +
        ",\"category_mapping\":\"current_settings\",\"meal_label_id\":" +
        std::to_string(settings->data()->mealvar) + ",\"records\":[";
    bool first = true;
    for (const auto &item : items) {
        if (!first) body += ',';
        first = false;
        body += "{\"time\":" + std::to_string(item.at) + ",\"source\":" + std::to_string(item.source) +
            ",\"index\":" + std::to_string(item.index) + ",\"label_id\":" + std::to_string(item.type) +
            ",\"label\":" + quote(settings->getlabel(item.type)) + ",\"value\":" + number(item.value) +
            ",\"meal_id\":" + (item.meal_id ? std::to_string(item.meal_id) : "null") + "}";
    }
    body += "]}";
    return response(200, body);
}

std::string glucose_export(std::string_view path, const Period &p, uint32_t now) {
    if (!sensors) return error(503, "unavailable", "Sensor data are not initialized.");
    const auto exporter = path == "/v1/glucose" ? getstream : path == "/v1/history" ? gethistory : getscans;
    // Request one extra record, then report truncation rather than silently
    // suggesting that the returned prefix is a complete interval.
    const int max_count = !p.limit || p.limit == uint32_t(std::numeric_limits<int>::max())
        ? std::numeric_limits<int>::max() : int(p.limit + 1);
    const std::span<char> data = exporter(0, 4096, p.start, p.end, true, p.unit,
                                          true, max_count, p.calibrated, p.pastvalues);
    std::unique_ptr<char[]> owner(data.data());
    if (!owner || data.size() == std::numeric_limits<size_t>::max())
        return error(500, "export_failed", "Juggluco could not export this interval.");
    std::string_view body(data.data(), data.size());
    size_t returned = 0, cursor = body.find('\n');
    if (cursor == std::string_view::npos) return error(500, "export_failed", "Export header is missing.");
    ++cursor;
    size_t cut = body.size();
    while (cursor < body.size()) {
        const size_t end = body.find('\n', cursor);
        if (p.limit && returned == p.limit) { cut = cursor; ++returned; break; }
        ++returned;
        if (end == std::string_view::npos) break;
        cursor = end + 1;
    }
    const bool truncated = (p.limit && returned > p.limit) || returned == std::size_t(std::numeric_limits<int>::max());
    if (truncated && p.limit && returned > p.limit) { returned = p.limit; body = body.substr(0, cut); }
    const std::string extra = "X-Juggluco-Generated-At: " + std::to_string(now) +
        "\r\nX-Juggluco-Start: " + std::to_string(p.start) +
        "\r\nX-Juggluco-End: " + std::to_string(p.end) +
        "\r\nX-Juggluco-End-Exclusive: true\r\nX-Juggluco-Returned: " + std::to_string(returned) +
        "\r\nX-Juggluco-Truncated: " + (truncated ? "true" : "false") +
        "\r\nX-Juggluco-Unit: " + (p.unit == 1 ? "mmol/L" : "mg/dL") +
        "\r\nX-Juggluco-Calibrated: " + (p.calibrated ? "true" : "false") +
        "\r\nX-Juggluco-Pastvalues: " + (p.pastvalues ? "true" : "false") + "\r\n";
    return response(200, body, "text/tab-separated-values; charset=utf-8", extra);
}
} // namespace

std::string meal_store_path() {
    if (globalbasedir.empty()) return {};
    return std::string(globalbasedir) + "/meals.dat";
}
bool glucose_data_ready() { return settings && sensors; }

jgchat::Json glucose_display_preferences() {
    if (!settings) throw jgchat::UiError(jgchat::UiCode::juggluco_settings_are_not_initialized);
    const bool mmol = settings->data()->unit == 1;
    // Use the same conversion factor as Juggluco's native glucose exporter.
    return {{"unit", mmol ? "mmol/L" : "mg/dL"}, {"decimals", mmol ? 1 : 0},
        {"mg_dl_per_mmol_l", convfactordL},
        {"multiplier_from_mg_dl", mmol ? 1.0 / convfactordL : 1.0},
        {"multiplier_from_mmol_l", mmol ? 1.0 : convfactordL}};
}

WebConfig web_config() {
    if (!settings) throw jgchat::UiError(jgchat::UiCode::juggluco_settings_are_not_initialized);
    const auto* data = settings->data();
    const std::size_t length = data->apisecretlength;
    if (length > sizeof(data->apisecret)) throw jgchat::UiError(jgchat::UiCode::invalid_web_server_secret_configuration);
    return {data->usexdripwebserver, data->httpport ? data->httpport : 17580U, std::string(data->apisecret, length)};
}
std::string web_file_url(std::string_view relative) {
    if (!relative.starts_with("additions/chatgpt/") || relative.find("..") != std::string_view::npos ||
        relative.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/_.-") != std::string_view::npos)
        throw jgchat::UiError(jgchat::UiCode::invalid_saved_file_path);
    const auto config = web_config();
    if (!config.enabled) throw jgchat::UiError(jgchat::UiCode::the_file_is_saved_enable_web_server_in_settings_exchange_data_web_server_then_open_it);
    if (config.secret == "." || config.secret == "..") throw jgchat::UiError(jgchat::UiCode::a_web_server_secret_consisting_only_of_one_or_two_dots_cannot_be_used_in_a_browser_pa);
    constexpr char hex[] = "0123456789ABCDEF";
    std::string prefix;
    if (!config.secret.empty()) {
        prefix = "/";
        for (unsigned char c : config.secret) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '~' || c == '.') prefix += char(c);
            else { prefix += '%'; prefix += hex[c >> 4]; prefix += hex[c & 15]; }
        }
    }
    return "http://127.0.0.1:" + std::to_string(config.port) + prefix + "/" + std::string(relative);
}

std::string handle_request(std::string_view target) {
    std::string_view path;
    Args args;
    if (!parse(target, path, args)) return error(400, "invalid_target", "Expected a bounded path and query, not an HTTP request.");
    if (path != "/v1/metadata" && path != "/v1/iob" && path != "/v1/amounts" && path != "/v1/meals" &&
        path != "/v1/glucose" && path != "/v1/history" && path != "/v1/scans")
        return error(404, "unknown_operation", "This path is not a read operation supported by the ICE helper.");
    if (!settings) return error(503, "unavailable", "Juggluco settings are not initialized.");
    const time_t raw_now = time(nullptr);
    if (raw_now < iob_lookback || uint64_t(raw_now) >= UINT32_MAX)
        return error(503, "clock_out_of_range", "The phone clock is outside Juggluco's timestamp range.");
    const auto now = static_cast<uint32_t>(raw_now);
    if (path == "/v1/metadata") {
        if (!args.empty()) return error(400, "unknown_parameter", "metadata does not accept query parameters.");
        return metadata(now);
    }
    if (path == "/v1/iob") {
        uint32_t at = now;
        if (args.size() > 1 || (!args.empty() && (args[0].first != "at" || !uintarg(args[0].second, at))))
            return error(400, "invalid_parameter", "iob accepts only at=<Unix seconds>.");
        if (at < iob_lookback)
            return error(400, "invalid_time", "IOB time must be at least 57600.");
        return response(200, "{\"schema\":1,\"generated_at\":" + std::to_string(now) + ",\"iob\":" + iob_json(at) + "}");
    }
    Period p;
    std::string why;
    const bool glucose = path != "/v1/amounts" && path != "/v1/meals";
    if (!period(args, now, glucose, p, why)) return error(400, "invalid_parameter", why);
    if (!glucose) return amounts(p, now, path == "/v1/meals");
    return glucose_export(path, p, now);
}
} // namespace jgchatdata
