// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "juggluco_extra.hpp"
#include "jgchat/meal_layout.hpp"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifdef JGICE_DATA_TEST_BACKEND
#include "nutrition_test_backend.hpp"
#else
#include "foods/database.hpp"
#endif

namespace jgchatdata {
namespace {
using jgchat::Json;
void cancelled(const std::atomic_bool* cancel) {
    if (cancel && cancel->load()) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
}
Json unavailable(const char* reason) {
    return {{"status", "unavailable"}, {"reason", reason}};
}
template <std::size_t N> std::string text(const std::array<char, N>& bytes) {
    return {bytes.data(), strnlen(bytes.data(), N)};
}
std::string folded(std::string_view value) {
    std::string out(value);
    for (char& c : out) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return out;
}
bool matches(std::string_view value, const std::string& query) {
    return query.empty() || folded(value).find(query) != std::string::npos;
}
class MealFile {
    int fd_ = -1;
    uint64_t bytes_ = 0;
    bool read(void* out, std::size_t bytes, uint64_t offset) const {
        if (fd_ < 0 || offset > bytes_ || bytes > bytes_ - offset ||
            offset > uint64_t(std::numeric_limits<off_t>::max()) ||
            bytes > uint64_t(std::numeric_limits<off_t>::max()) - offset) return false;
        auto* p = static_cast<char*>(out);
        while (bytes) {
            const auto got = pread(fd_, p, bytes, static_cast<off_t>(offset));
            if (got < 0 && errno == EINTR) continue;
            if (got <= 0) return false;
            p += got; bytes -= got; offset += got;
        }
        return true;
    }
public:
    MealHeader header{};
    bool valid = false;
    MealFile() {
        const auto path = meal_store_path();
        if (path.empty()) return;
        fd_ = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
        struct stat st{};
        if (fd_ < 0 || fstat(fd_, &st) || !S_ISREG(st.st_mode) || st.st_size < 0) return;
        bytes_ = static_cast<uint64_t>(st.st_size);
        if (!read(&header, sizeof(header), 0) || header.unitnr > header.units.size() ||
            header.ingredientnr > header.ingredients.size() ||
            header.mealindex > (bytes_ - sizeof(header)) / sizeof(MealElement)) return;
        valid = unchanged();
    }
    ~MealFile() { if (fd_ >= 0) close(fd_); }
    MealFile(const MealFile&) = delete;
    MealFile& operator=(const MealFile&) = delete;
    bool unchanged() const {
        MealHeader again{};
        return read(&again, sizeof(again), 0) && !std::memcmp(&header, &again, sizeof(header));
    }
    bool meal(uint32_t id, std::vector<MealElement>& items) const {
        // A committed meal ends before mealindex; its terminal record holds
        // the item count. Match itemsinmeal's count/NaN-or-zero marker rules.
        if (!valid || !id || id >= header.mealindex) return false;
        const uint64_t pos = sizeof(MealHeader) + uint64_t(id) * sizeof(MealElement);
        MealElement tail{};
        if (!read(&tail, sizeof(tail), pos) || tail.ingr >= 40 || tail.ingr > id ||
            (!std::isnan(tail.amount) && tail.amount != 0)) return false;
        std::vector<MealElement> block(tail.ingr + 1), again(block.size());
        const uint64_t start = pos - uint64_t(tail.ingr) * sizeof(MealElement);
        const auto bytes = block.size() * sizeof(MealElement);
        if (!read(block.data(), bytes, start) || !read(again.data(), bytes, start) ||
            std::memcmp(block.data(), again.data(), bytes) ||
            std::memcmp(&block.back(), &tail, sizeof(tail))) return false;
        block.pop_back();
        items = std::move(block);
        return true;
    }
};
Json ingredient(const MealHeader& h, uint32_t id) {
    if (id >= h.ingredientnr) return {{"ingredient_id", id}, {"status", "invalid_reference"}};
    const auto& in = h.ingredients[id];
    const bool valid_unit = in.unit >= 0 && uint32_t(in.unit) < h.unitnr;
    const bool valid_carb = std::isfinite(in.carb) && in.carb >= 0;
    return {{"ingredient_id", id}, {"name", text(in.name)}, {"unit_id", in.unit},
        {"unit", valid_unit ? Json(text(h.units[in.unit])) : Json(nullptr)},
        {"carbohydrate_grams_per_unit", valid_carb ? Json(in.carb) : Json(nullptr)},
        {"usage_count", in.used}, {"food_database_id", nullptr},
        {"status", valid_unit && valid_carb ? "ok" : "invalid_definition"}};
}
const Nutrients& nutrients() { static const Nutrients value; return value; }
Json page(std::string_view source, std::string_view query, uint32_t offset,
          uint32_t limit, uint32_t total, Json records) {
    const uint64_t next = uint64_t(offset) + records.size();
    return {{"status", "ok"}, {"source", source}, {"generated_at", std::time(nullptr)},
        {"query", query}, {"match", "literal substring; ASCII case-insensitive; other UTF-8 bytes exact"},
        {"offset", offset}, {"limit", limit}, {"total_matches", total},
        {"returned", records.size()}, {"next_offset", next < total ? Json(next) : Json(nullptr)},
        {"records", std::move(records)}};
}
}

jgchat::Json nutrition_context() {
    MealFile file;
    const auto& db = nutrients();
    return {{"meal_store_available", file.valid},
        {"ingredient_count", file.valid ? Json(file.header.ingredientnr) : Json(nullptr)},
        {"unit_count", file.valid ? Json(file.header.unitnr) : Json(nullptr)},
        {"food_count", db.foodnr()}, {"food_component_count", db.compnr()},
        {"food_basis", "per 100 g"},
        {"ingredient_food_link", "Not stored. User ingredients contain name, unit and carbohydrate factor only; food matches and portion conversions are estimates, not recorded facts."}};
}

jgchat::Json ingredients(std::string_view query, uint32_t offset, uint32_t limit,
                         const std::atomic_bool* cancel) {
    cancelled(cancel);
    MealFile file;
    if (!file.valid) return unavailable("Meal/ingredient store is unavailable, changing or invalid.");
    const auto search = folded(query);
    Json records = Json::array();
    uint32_t count = 0;
    for (uint32_t id = 0; id < file.header.ingredientnr; ++id) {
        cancelled(cancel);
        if (!matches(text(file.header.ingredients[id].name), search)) continue;
        if (count++ >= offset && (!limit || records.size() < limit)) records.push_back(ingredient(file.header, id));
    }
    if (!file.unchanged()) return unavailable("Ingredient definitions changed while reading; retry.");
    auto result = page("Juggluco user ingredients", query, offset, limit, count, std::move(records));
    result["definitions"] = "Current definitions, also used to interpret saved meals; not historical snapshots.";
    return result;
}

jgchat::Json meal_details(jgchat::Json result, const std::atomic_bool* cancel) {
    cancelled(cancel);
    if (result.value("status", "error") != "ok") return result;
    MealFile file;
    if (!file.valid) return unavailable("Meal/ingredient store is unavailable, changing or invalid.");
    for (auto& record : result.at("records")) {
        cancelled(cancel);
        std::vector<MealElement> items;
        if (!file.meal(record.at("meal_id").get<uint32_t>(), items)) {
            record["meal_status"] = "invalid_deleted_or_changing_reference";
            record["items"] = nullptr;
            record["carbohydrate_grams"] = nullptr;
            continue;
        }
        Json rows = Json::array();
        bool complete = true;
        // Use the same float quantity * current factor accumulation as the
        // app's meal view. Invalid/missing items make the total unavailable.
        float total = 0;
        for (const auto& item : items) {
            auto row = ingredient(file.header, item.ingr);
            const bool quantity_valid = std::isfinite(item.amount) && item.amount >= 0;
            const bool valid = row.at("status") == "ok" && quantity_valid;
            const float carb = valid ? item.amount * file.header.ingredients[item.ingr].carb : NAN;
            row["quantity"] = quantity_valid ? Json(item.amount) : Json(nullptr);
            row["carbohydrate_grams"] = std::isfinite(carb) ? Json(carb) : Json(nullptr);
            if (std::isfinite(carb)) total += carb;
            else complete = false;
            rows.push_back(std::move(row));
        }
        complete &= std::isfinite(total);
        record["meal_status"] = complete ? "ok" : "incomplete";
        record["items"] = std::move(rows);
        record["carbohydrate_grams"] = complete ? Json(total) : Json(nullptr);
    }
    if (!file.unchanged()) return unavailable("Meal/ingredient definitions changed while reading; retry.");
    result["definitions"] = "Current user ingredient definitions; not historical nutrient snapshots. value is the entered number, carbohydrate_grams is the current ingredient sum. A timestamp is not proof of consumption time.";
    result["food_database_link"] = "Not stored; do not claim database fat/protein as recorded meal content.";
    return result;
}

jgchat::Json food_search(std::string_view query, uint32_t offset, uint32_t limit,
                        const std::atomic_bool* cancel) {
    cancelled(cancel);
    const auto& db = nutrients();
    const auto search = folded(query);
    Json records = Json::array();
    uint32_t count = 0;
    for (uint32_t id = 0; id < db.foodnr(); ++id) {
        cancelled(cancel);
        const char* name = db.foodlabel(id);
        if (!matches(name, search)) continue;
        if (count++ >= offset && (!limit || records.size() < limit))
            records.push_back({{"food_id", id}, {"name", name}});
    }
    return page("Juggluco built-in food composition database", query, offset, limit, count, std::move(records));
}

jgchat::Json food_details(uint32_t id, const std::atomic_bool* cancel) {
    cancelled(cancel);
    const auto& db = nutrients();
    if (id >= db.foodnr()) return unavailable("Food ID is outside the current database; search again.");
    const auto* values = db.getcomponents(id);
    Json components = Json::array();
    for (uint32_t i = 0; i < db.compnr(); ++i) {
        cancelled(cancel);
        const int32_t raw = std::bit_cast<int32_t>(values[i]);
        const char* status = raw >= 0 ? "known" : raw == -1 ? "not_available" :
            raw == -2 ? "unknown" : raw == -3 ? "trace" : "invalid";
        components.push_back({{"component_id", i}, {"name", db.complabel(i)},
            {"unit", db.compunit(i)}, {"status", status},
            {"value", raw >= 0 ? Json(double(raw) / 1000.0) : Json(nullptr)}});
    }
    return {{"status", "ok"}, {"source", "Juggluco built-in food composition database"},
        {"generated_at", std::time(nullptr)}, {"food_id", id}, {"name", db.foodlabel(id)},
        {"basis", {{"quantity", 100}, {"unit", "g"}}}, {"components", std::move(components)},
        {"interpretation", "All stored components are included. Trace, unknown and not_available are distinct from numeric zero. This is reference food composition, not proof of the user's meal or portion."}};
}
} // namespace jgchatdata
