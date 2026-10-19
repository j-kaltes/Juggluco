// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "juggluco_extra.hpp"
#include <ctime>
#include <memory>
#include <stdexcept>

#ifdef JGICE_DATA_TEST_BACKEND
#include "statistics_test_backend.hpp"
#else
#include "share/logs.hpp"
#include "net/watchserver/Getopts.hpp"
#include "net/watchserver/watchserver.hpp"
#endif

namespace jgchatdata {
jgchat::Json statistics(uint32_t days, uint32_t end, int unit, bool history,
                        bool calibrated, bool pastvalues, const std::atomic_bool* cancel) {
    auto check = [&] { if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled); };
    check();
    if (!glucose_data_ready()) return {{"status", "unavailable"}, {"reason", "Glucose stores are not initialized."}};
    Getopts opts;
    opts.start = end - days * 86400U;
    opts.end = end;
    opts.unit = unit;
    opts.jsonmode = true;
    opts.historymode = history;
    opts.streammode = !history;
    opts.calibratedmode = calibrated && !history;
    opts.calibratedhistorymode = calibrated && history;
    opts.pastvaluesmode = pastvalues;
    recdata data;
    // Use Juggluco's existing numeric exporter and stats calculation directly.
    // This neither opens HTTP nor reads/changes the visible statistics screen.
    const bool ok = givestatistics(opts, {}, &data);
    std::unique_ptr<char[]> owner(data.allbuf);
    check();
    if (!ok) return {{"status", "unavailable"}, {"reason", "Juggluco could not generate statistics for this source/window (insufficient usable data or export failure)."}};
    if (!data.start || data.len <= 0)
        throw jgchat::UiError(jgchat::UiCode::invalid_native_statistics_response);
    const std::string_view http(data.start, static_cast<std::size_t>(data.len));
    const auto split = http.find("\r\n\r\n");
    if (split == std::string_view::npos || !http.starts_with("HTTP/1.1 200 "))
        throw jgchat::UiError(jgchat::UiCode::invalid_native_statistics_envelope);
    auto result = jgchat::Json::parse(http.substr(split + 4));
    if (!result.is_object() || result.value("schemaVersion", 0) != 1)
        throw jgchat::UiError(jgchat::UiCode::unsupported_native_statistics_schema);
    result["status"] = "ok";
    result["computed_by"] = "Juggluco statistics exporter";
    result["generated_at"] = std::time(nullptr);
    result["requested_days"] = days;
    result["requested_end"] = end;
    result["pastvalues"] = pastvalues;
    result["window_selection"] = "Native whole-day selection may shift/clamp to available data; startTime/endTime and durationDays are the actual returned coverage. Match source, period and calibration to compare with the statistics screen.";
    result["range_percentages"] = "Native measurement-count percentages, not a new time-weighted calculation; native timeActivePercent uses expectedMeasurementCount.";
    result["a1c_interpretation"] = "estimatedA1c and gmi are glucose-derived estimates, not laboratory HbA1c.";
    return result;
}
}
