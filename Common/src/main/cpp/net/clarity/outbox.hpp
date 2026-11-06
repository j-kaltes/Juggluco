// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "records.hpp"
#include "number_inventory.hpp"
#include <map>
#include <memory>
namespace clarity {
struct Snapshot {
    std::vector<Reading> readings;
    std::vector<Number> numbers;
    // First usable reading from each chosen history/stream source, including
    // sources whose last reading predates the upload start date.
    std::vector<SensorSource> sensors;
    // Separate from eligible numbers: category/date filters must not delete
    // existing Clarity events. Only a complete retained source can do that.
    std::vector<NumberInventory> numberInventories;
};
struct GlucoseRange { int64_t count = 0, first = 0, last = 0; };
struct GlucoseProgress { GlucoseRange acknowledged, pending; };
struct SavedEvent {
    std::string key;
    EventContent content;
    uint64_t revision = 0;
    int32_t firstPosition = -1;
    bool deleted = false, updated = false;
};
struct PendingBatch {
    std::string id;
    std::string_view body; // Exact Dexcom wire bytes; never parsed for local use.
    std::vector<int64_t> times;
    std::vector<std::string> sessions;
    std::vector<SavedEvent> events;
};
class Outbox {
    struct Impl;
    std::unique_ptr<Impl> impl;
  public:
    Outbox(std::string path, const std::string &account, const std::string &installation);
    ~Outbox();
    static std::string savedInstallation(const std::string &path, const std::string &account);
    bool prepare(const Snapshot &snapshot, int64_t since, int64_t now, bool numbers);
    PendingBatch pending() const;
    std::string_view pendingBody() const;
    void acknowledge(const std::string &receipt);
    // Only after the bulk service explicitly returns DuplicatePostSequence.
    // No new receipt is invented; retain the previous signed receipt.
    void acknowledgeAlreadyStored();
    size_t sentGlucose() const;
    GlucoseProgress glucoseProgress() const;
    const std::string &account() const;
    const std::string &installation() const;
    std::vector<SensorSource> sensorSources() const;
    std::vector<SavedEvent> savedEvents() const;
};
} // namespace clarity
