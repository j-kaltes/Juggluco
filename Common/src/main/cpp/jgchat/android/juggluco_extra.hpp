// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "jgchat/tools.hpp"
#include <cstdint>
#include <optional>

namespace jgchatdata {
std::string meal_store_path();
bool glucose_data_ready();
jgchat::Json glucose_display_preferences();
jgchat::Json activity_records(uint32_t start, uint32_t end, uint32_t offset, uint32_t limit,
    const std::string& revision, const std::atomic_bool* cancel);
jgchat::Json activity_samples(uint32_t record_id, const std::string& kind, uint32_t start, uint32_t end,
    std::optional<uint32_t> source_id, uint32_t offset, uint32_t limit,
    const std::string& revision, const std::atomic_bool* cancel);
jgchat::Json activity_settings(const jgchat::Json& labels, bool mirrored_amounts, const std::atomic_bool* cancel);
jgchat::Json nutrition_context();
jgchat::Json meal_details(jgchat::Json records, const std::atomic_bool* cancel);
jgchat::Json ingredients(std::string_view query, uint32_t offset, uint32_t limit,
                         const std::atomic_bool* cancel);
jgchat::Json food_search(std::string_view query, uint32_t offset, uint32_t limit,
                        const std::atomic_bool* cancel);
jgchat::Json food_details(uint32_t id, const std::atomic_bool* cancel);
jgchat::Json statistics(uint32_t days, uint32_t end, int unit, bool history,
                        bool calibrated, bool pastvalues, const std::atomic_bool* cancel);
}
