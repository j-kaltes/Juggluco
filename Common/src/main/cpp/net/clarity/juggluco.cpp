// SPDX-License-Identifier: GPL-3.0-or-later
#include "clarity.hpp"
#include "fromjava.h"
#include "share/logs.hpp"
#include <jni.h>
#include <string>
#include <array>

namespace clarity {
struct Status {
    bool available = false, enabled = false, libre3History = false, numbers = false;
    int64_t since = 0, lastSuccess = 0, acknowledgedCount = 0, acknowledgedLast = 0;
    int64_t pendingCount = 0, pendingFirst = 0, pendingLast = 0;
    std::string account, status;
};
}

#ifdef JUGGLUCO_CLARITY
#include "auth.hpp"
#include "client.hpp"
#include "certificate.hpp"
#include "net/libreview/numcategories.hpp"
#include "nums/numdata.hpp"
#include "outbox.hpp"
#include "account_store.hpp"
#include "number_categories.hpp"
#include "datbackup.hpp"
#include "received.hpp"
#include "sensoren.hpp"
#include "oauth_registration.hpp"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <sys/stat.h>
#include <thread>
extern Sensoren *sensors;
extern Backup *backup;
namespace clarity {
class Worker {
    std::mutex mutex, sourcesMutex, networkMutex;
    std::condition_variable condition;
    std::thread thread;
    bool stopping = false, dirty = false;
    uint64_t generation = 0;
    std::vector<Numdata *> sources;
    // Suppress repeated scan summaries while the same retained rows remain.
    std::map<std::string, std::array<int64_t, 7>> numberDiagnostics;
    std::string root, status = "Disabled";
    int64_t lastSuccess = 0;
    Json config, loginAttempt;
    GlucoseProgress glucoseProgress;
    using Options = Tings::ClaritySettingsData;
    Snapshot snapshot(const Options &o) {
        Snapshot result;
        if (sensors)
            for (int i = 0, last = sensors->last(); i <= last; ++i) {
                auto *s = sensors->getSensorData(i);
                if (!s)
                    continue;
                std::string name(s->sensorname()->data(), s->sensorname()->size());
                const auto start = s->getstarttime();
                const auto wear = s->getweardurationSEC();
                const auto warmup = s->getWarmupSEC();
                const bool history = s->isLibre3() && o.libre3History;
                int64_t first = 0;
                const auto now = time(nullptr);
                const auto runBegin = result.readings.size();
                auto noteFirst = [&](int64_t time, int mgdl) {
                    if (!first && time >= start && time <= now && mgdl >= 20 && mgdl <= 600)
                        first = time;
                };
                auto add = [&](int64_t time, int mgdl, double rate) {
                    noteFirst(time, mgdl);
                    if (time >= o.since)
                        result.readings.push_back({name, time, start, mgdl, rate, wear, warmup, history});
                };
                if (history) {
                    const int end = receivedHistoryEnd(*s);
                    for (int p = s->getstarthistory(); p < end; ++p) {
                        auto *r = s->getglucose(p);
                        if (r->valid())
                            add(r->gettime(), r->getmgdL(),
                                std::numeric_limits<double>::quiet_NaN());
                    }
                    // A newly active source still supersedes an older one
                    // while its first upload is waiting for backfill.
                    if (!first)
                        for (int p = std::max(s->getstarthistory(), end),
                                 storedEnd = s->getScanendhistory(); p < storedEnd && !first; ++p) {
                            auto *r = s->getglucose(p);
                            if (r->valid())
                                noteFirst(r->gettime(), r->getmgdL());
                        }
                } else {
                    const auto rows = receivedStream(*s);
                    for (const auto &r : rows)
                        if (r.valid(0))
                            add(r.gettime(), r.getmgdL(), r.getchange());
                    if (!first)
                        for (const auto &r : s->getPolldata()) {
                            if (r.valid(0))
                                noteFirst(r.gettime(), r.getmgdL());
                            if (first)
                                break;
                        }
                }
                if (first)
                    result.sensors.push_back({name, start, int64_t(start) + wear, first});
                mergeReadingRun(result.readings, runBegin);
            }
        struct Category { NumberKind kind; double weight; std::string label; };
        std::vector<Category> categories;
        bool mmol = false;
        if (o.numbers) {
            // Do not hold the worker mutex while taking a number-database
            // lock: a producer can wake the worker while holding that lock.
            std::lock_guard categoryLock(mutex);
            mmol = settings->data()->unit == 1;
            for (int i = 0; i < settings->varcount(); ++i) {
                NumberKind kind = NumberKind::Ignore;
                double weight = 1;
                if (i == settings->data()->bloodvar)
                    kind = NumberKind::Blood;
                else if (rapidWeight(i))
                    kind = NumberKind::Rapid;
                else if (longWeight(i))
                    kind = NumberKind::Long;
                else if ((weight = carboWeight(i)) > 0)
                    kind = NumberKind::Carbs;
                else if (isNote(i))
                    kind = NumberKind::Note;
                categories.push_back({kind, weight, std::string(settings->getlabel(i))});
            }
        }
        // Hold the registry until the copy is complete. closeNums removes its
        // pointer under this same mutex before destroying the database.
        std::lock_guard registry(sourcesMutex);
        if (o.numbers)
            for (auto *source : sources) {
                std::lock_guard lock(source->nummutex);
                const int first = source->getfirstpos(), last = source->getlastpos();
                if (first < 0 || last < first) {
                    diagnostic("error: invalid amount source bounds first=%d last=%d", first, last);
                    continue;
                }
                // A mirror may retain only a suffix. Do not reconcile its
                // pruned prefix (including timestamps shared with the boundary).
                // An empty pruned source proves deletion only for an event
                // previously seen with this same retention position.
                NumberInventory inventory{std::to_string(source->getnewident()),
                    first == 0 ? 0 : INT64_MAX, first};
                int64_t live = 0, removed = 0, zeroTime = 0;
                for (const auto *n = source->begin(), *end = source->end(); n < end; ++n) {
                    // Inventory membership uses keys, not chronological order.
                    // A zero-time placeholder or a tombstone must not disable
                    // deletions for the entire database. No rows are re-sorted.
                    if (first && n->time)
                        inventory.firstTime = std::min(inventory.firstTime, int64_t(n->time) + 1);
                    // Juggluco tombstones are genuine removals, not labels.
                    if (n->type & Numdata::removedbit) {
                        ++removed;
                        continue;
                    }
                    if (!n->time) {
                        ++zeroTime;
                        continue;
                    }
                    ++live;
                    if (!std::isfinite(n->value)) {
                        inventory.unreadable(n->time, n->type);
                        diagnostic("error: skipping non-finite number amount");
                        continue;
                    }
                    auto key = inventory.add(n->time, n->type);
                    if (n->time < o.since || n->type >= categories.size())
                        continue;
                    const auto &category = categories[n->type];
                    // Only unmapped and Do not send labels are excluded.
                    if (category.kind == NumberKind::Ignore)
                        continue;
                    result.numbers.push_back({key, category.label, n->time, n->value,
                                              category.kind, category.weight, mmol});
                }
                // A nonempty range consisting solely of uninitialized slots
                // provides no deletion evidence. Empty ranges and tombstones
                // do: the last genuine amount can have been deleted.
                const bool inventoryAvailable = first == last || live || removed;
                const std::array<int64_t, 7> detail{first, last, live, removed, zeroTime,
                                                    inventory.firstTime, inventoryAvailable};
                auto prior = numberDiagnostics.find(inventory.source);
                if (prior == numberDiagnostics.end() || prior->second != detail) {
                    diagnostic("amount inventory: source=%s first=%d last=%d live=%lld removed=%lld "
                               "zeroTime=%lld deletionCheck=%s", inventory.source.c_str(), first, last,
                               (long long)live, (long long)removed, (long long)zeroTime,
                               inventoryAvailable ? "enabled" : "deferred (only zero-time slots)");
                    numberDiagnostics[inventory.source] = detail;
                }
                if (inventoryAvailable)
                    result.numberInventories.push_back(std::move(inventory));
            }
        return result;
    }
    void run() {
        // Keep the mapped outbox open across ordinary upload scans.
        std::unique_ptr<Outbox> outbox;
        using Clock = std::chrono::steady_clock;
        diagnostic("upload worker started");
        auto next = Clock::now();
        unsigned failures = 0;
        bool idleLogged = false;
        uint64_t observedGeneration = 0;
        std::unique_lock lock(mutex);
        while (!stopping) {
            condition.wait_until(lock, next, [&] { return stopping || dirty; });
            if (stopping)
                break;
            dirty = false;
            checkCategoriesLocked();
            auto options = settings->data()->clarity;
            if (observedGeneration != generation) {
                observedGeneration = generation;
                failures = 0;
                idleLogged = false;
            }
            if (!options.enabled || config.is_null()) {
                next = Clock::now() + std::chrono::minutes(5);
                continue;
            }
            // New readings do not defeat exponential retry backoff.
            if (failures && Clock::now() < next)
                continue;
            auto current = config;
            auto revision = generation;
            status = "Preparing upload";
            lock.unlock();
            try {
                // The provider has process-global native state. Sign-in service
                // discovery and uploads must never initialize it concurrently.
                std::lock_guard network(networkMutex);
                const std::string account = current.at("account");
                auto folder = root + "/" + account;
                if (mkdir(folder.c_str(), 0700) && errno != EEXIST) {
                    diagnostic("error: create account directory: errno=%d (%s)", errno,
                               strerror(errno));
                    throw Error("Cannot create Clarity account directory");
                }
                if (!outbox || outbox->account() != account) {
                    outbox.reset();
                    outbox = std::make_unique<Outbox>(folder + "/outbox.map", account, current.at("installationId"));
                }
                auto data = snapshot(options);
                const bool prepared = outbox->prepare(data, options.since, time(nullptr), options.numbers);
                {
                    std::lock_guard guard(mutex);
                    if (generation == revision) {
                        glucoseProgress = outbox->glucoseProgress();
                        if (prepared)
                            status = "Uploading";
                    }
                }
                if (prepared) {
                    auto check = [&] {
                        std::lock_guard guard(mutex);
                        if (stopping || !settings->data()->clarity.enabled ||
                            generation != revision)
                            throw Error("Clarity configuration changed");
                    };
                    auto save = [&](const Json &updated) {
                        std::lock_guard guard(mutex);
                        if (generation != revision)
                            throw Error("Clarity configuration changed");
                        AccountStore(root + "/account.map").save(updated);
                        config = updated;
                    };
                    auto post = [&](const std::string &url, const std::string &body,
                                    const std::string &headers) {
                        return nativeAccountPost(current, url, body, headers);
                    };
                    uploadPending(current, *outbox, post, save, check);
                    idleLogged = false;
                    lock.lock();
                    if (generation == revision) {
                        lastSuccess = time(nullptr);
                        glucoseProgress = outbox->glucoseProgress();
                        status = "Upload acknowledged; checking for more data";
                    }
                    next = Clock::now() + std::chrono::seconds(5);
                } else {
                    if (!idleLogged) {
                        diagnostic(
                            "no eligible new records; waiting for data or five-minute timer");
                        idleLogged = true;
                    }
                    lock.lock();
                    if (generation == revision)
                        status = "Up to date";
                    next = Clock::now() + std::chrono::minutes(5);
                }
                failures = 0;
            } catch (const Error &e) {
                idleLogged = false;
                caught("upload worker");
                outbox.reset();
                if (!lock.owns_lock())
                    lock.lock();
                ++failures;
                const auto seconds = std::min(1800u, 30u << std::min(failures - 1, 6u));
                next = Clock::now() + std::chrono::seconds(seconds);
                diagnostic("upload retry scheduled in %u seconds (attempt=%u)", seconds, failures);
                status = std::string(e.what()) + "; batch retained. Retry in " +
                         std::to_string(seconds) + " seconds";
            } catch (...) {
                idleLogged = false;
                caught("upload worker");
                outbox.reset();
                if (!lock.owns_lock())
                    lock.lock();
                ++failures;
                const auto seconds = std::min(1800u, 30u << std::min(failures - 1, 6u));
                next = Clock::now() + std::chrono::seconds(seconds);
                diagnostic("upload retry scheduled in %u seconds (attempt=%u)", seconds, failures);
                // Exception text from a JSON parser can include private data.
                status = "Upload failed; batch retained. Retry in " + std::to_string(seconds) +
                         " seconds";
            }
        }
        diagnostic("upload worker stopped");
    }

  public:
    ~Worker() {
        {
            std::lock_guard lock(mutex);
            stopping = true;
            condition.notify_all();
        }
        if (thread.joinable())
            thread.join();
    }
    void start() {
        std::lock_guard lock(mutex);
        if (thread.joinable() || !settings)
            return;
        // New native format intentionally starts without the experimental JSON
        // account/outbox. Never silently reuse its credentials with empty receipts.
        root = std::string(globalbasedir) + "/clarity-indexed";
        if (mkdir(root.c_str(), 0700) && errno != EEXIST) {
            diagnostic("error: create Clarity directory: errno=%d (%s)", errno, strerror(errno));
            status = "Cannot open Clarity directory";
            return;
        }
        auto &o = settings->data()->clarity;
        if (o.version != 1) {
            o = {};
            o.version = 1;
            o.since = time(nullptr) - 14 * 86400;
            o.numbers = 0;
        }
        diagnostic("upload settings: enabled=%d Libre3History=%d numbers=%d start=%s",
                   int(o.enabled), int(o.libre3History), int(o.numbers), timestamp(o.since).c_str());
        try {
            config = AccountStore(root + "/account.map").load();
            if (!config.is_null()) validateConfig(config);
            else {
                o.enabled = 0;
                status = "Sign in to Dexcom";
                diagnostic("native Clarity storage has no account; uploads disabled until sign-in");
            }
        } catch (...) {
            caught("load mapped Clarity account");
            config = nullptr;
            o.enabled = 0;
            status = "Cannot read Clarity account storage";
        }
        thread = std::thread([this] {
            try {
                run();
            } catch (...) {
                caught("upload thread stopped unexpectedly");
            }
        });
    }
    void wake() {
        std::lock_guard lock(mutex);
        dirty = true;
        condition.notify_one();
    }
    void add(Numdata *n) {
        {
            std::lock_guard lock(sourcesMutex);
            if (std::find(sources.begin(), sources.end(), n) == sources.end())
                sources.push_back(n);
        }
        wake();
    }
    void remove(Numdata *n) {
        std::lock_guard lock(sourcesMutex);
        std::erase(sources, n);
    }
    std::vector<int> categoriesLocked() const {
        std::vector<int> result;
        for (int i = 0; i < settings->getlabelcount(); ++i)
            result.push_back(numberCategory(settings->data()->librenums[i].kind,
                                            i == settings->data()->bloodvar));
        return result;
    }
    std::vector<float> weightsLocked() const {
        std::vector<float> result;
        for (int i = 0; i < settings->getlabelcount(); ++i)
            result.push_back(settings->data()->librenums[i].weight);
        return result;
    }
    const char *categoryErrorLocked() const {
        return validateNumberCategories(categoriesLocked(), weightsLocked());
    }
    void checkCategoriesLocked() {
        auto &o = settings->data()->clarity;
        if (o.numbers && categoryErrorLocked()) {
            o.numbers = 0;
            ++generation;
            diagnostic("Send amounts disabled: label categorization required");
        }
    }
    std::vector<int> categories() {
        start();
        std::lock_guard lock(mutex);
        return categoriesLocked();
    }
    std::string setCategories(const std::vector<int> &kinds, const std::vector<float> &weights) {
        start();
        std::lock_guard lock(mutex);
        if (kinds.size() != size_t(settings->getlabelcount())) {
            diagnostic("error: labels changed while categorizing");
            return "Labels changed; reopen Categories";
        }
        if (auto error = validateNumberCategories(kinds, weights)) {
            diagnostic("error: %s", error);
            return error;
        }
        int blood = maxvarnr;
        bool changed = false;
        for (size_t i = 0; i < kinds.size(); ++i) {
            auto &category = settings->data()->librenums[i];
            const int kind = libreCategoryForClarity(kinds[i], category.kind);
            const float weight = kinds[i] == 6 && kind == category.kind ? category.weight :
                                 kind == 3 ? weights[i] : 0;
            changed |= category.kind != kind || category.weight != weight;
            category = {kind, weight};
            if (kinds[i] == 6)
                blood = i;
        }
        changed |= settings->data()->bloodvar != blood;
        settings->data()->bloodvar = blood;
        if (changed) {
            settings->updated();
            if (backup)
                backup->sendLibreNumbers();
            ++generation;
            dirty = true;
            condition.notify_one();
            diagnostic("amount categories saved; upload identities and acknowledgments retained");
        }
        return "";
    }
    Status view() {
        std::lock_guard lock(mutex);
        checkCategoriesLocked();
        const auto o = settings->data()->clarity;
        return {true, bool(o.enabled), bool(o.libre3History), bool(o.numbers), o.since,
                lastSuccess, glucoseProgress.acknowledged.count, glucoseProgress.acknowledged.last,
                glucoseProgress.pending.count, glucoseProgress.pending.first,
                glucoseProgress.pending.last, config.is_null() ? "" : config.value("account", ""),
                status};
    }
    std::string options(bool enabled, bool history, bool numbers, int64_t since) {
        start();
        std::lock_guard lock(mutex);
        if (since < 1598911200 || since > time(nullptr)) {
            diagnostic("error: invalid upload start date");
            return "Choose a valid start date";
        }
        if (enabled && config.is_null()) {
            diagnostic("error: enabling upload without a signed-in account");
            return "Sign in to Dexcom first";
        }
        if (numbers) {
            if (auto error = categoryErrorLocked()) {
                diagnostic("error: %s", error);
                return error;
            }
        }
        auto &o = settings->data()->clarity;
        o.enabled = enabled;
        o.libre3History = history;
        o.numbers = numbers;
        o.since = since;
        ++generation;
        dirty = true;
        diagnostic("settings saved: enabled=%d Libre3History=%d numbers=%d start=%s", enabled, history,
                   numbers, timestamp(since).c_str());
        status = enabled ? "Waiting to upload" : "Disabled";
        condition.notify_one();
        return "";
    }
    // Called with mutex held, only after token exchange and service discovery.
    void acceptConfig(Json c) {
        validateConfig(c);
        auto folder = root + "/" + c.at("account").get<std::string>();
        const auto path = folder + "/outbox.map";
        if (access(path.c_str(), F_OK) == 0)
            c["installationId"] = Outbox::savedInstallation(path, c.at("account"));
        else if (errno != ENOENT)
            throw Error("Cannot access mapped Clarity outbox");
        else if (!config.is_null() && c.at("account") == config.at("account"))
            c["installationId"] = config.at("installationId");
        AccountStore(root + "/account.map").save(c);
        if (config.is_null() || c.at("account") != config.at("account")) {
            lastSuccess = 0;
            glucoseProgress = {};
        }
        config = std::move(c);
        ++generation;
    }
    std::array<std::string, 2> loginBegin(const std::array<std::string, 5> &input) {
        start();
        try {
            const auto &[country, locale, manufacturer, model, osVersion] = input;
            const Json device{{"manufacturer", manufacturer}, {"model", model},
                              {"osVersion", osVersion}};
            std::lock_guard lock(mutex);
            if (root.empty() || !thread.joinable())
                throw Error("Clarity storage is unavailable");
            if (settings->data()->clarity.enabled)
                throw Error("Turn off Clarity uploads before signing in to another account");
            auto attempt =
                beginLogin(country, locale, device, time(nullptr));
            AccountStore(root + "/pending-login.map").clear();
            loginAttempt = std::move(attempt);
            ++generation;
            diagnostic("Dexcom sign-in begun; waiting for authorization callback");
            status = "Waiting for Dexcom sign-in";
            return {loginAttempt.at("url").get<std::string>(), ""};
        } catch (const Error &e) {
            caught("begin sign-in");
            return {"", e.what()};
        } catch (...) {
            caught("begin sign-in");
            return {"", "Cannot begin Dexcom sign-in"};
        }
    }
    std::string loginFinish(std::string_view callback) {
        start();
        std::lock_guard network(networkMutex);
        uint64_t revision = 0;
        try {
            Json attempt, next;
            {
                std::lock_guard lock(mutex);
                revision = generation;
                if (root.empty() || !thread.joinable())
                    throw Error("Clarity storage is unavailable");
                if (settings->data()->clarity.enabled)
                    throw Error("Turn off Clarity uploads before changing the account");
                if (callback.empty()) {
                    next = AccountStore(root + "/pending-login.map").load();
                    if (next.is_null()) throw Error("Start a new Dexcom sign-in");
                } else {
                    if (loginAttempt.is_null())
                        throw Error("Start a new Dexcom sign-in");
                    attempt = std::move(loginAttempt);
                    loginAttempt = nullptr; // Authorization codes are single use.
                }
                diagnostic("completing sign-in: %s",
                           callback.empty() ? "resuming saved tokens" : "authorization callback");
                status = "Completing Dexcom sign-in";
            }
            auto save = [&](const Json &staged) {
                std::lock_guard lock(mutex);
                if (generation != revision || settings->data()->clarity.enabled)
                    throw Error("Clarity configuration changed; sign in again");
                AccountStore(root + "/pending-login.map").save(staged);
            };
            if (!callback.empty()) {
                next =
                    exchangeLogin(attempt, callback, oauthClientSecret, nativePost, time(nullptr));
                // Preserve the newly issued refresh token even when subsequent
                // regional discovery is temporarily unavailable.
                save(next);
            }
            refreshAccess(next, nativePost, save);
            discoverEndpoints(next, nativePost);
            {
                std::lock_guard lock(mutex);
                if (generation != revision || settings->data()->clarity.enabled)
                    throw Error("Clarity configuration changed; sign in again");
                acceptConfig(std::move(next));
                AccountStore(root + "/pending-login.map").clear();
                diagnostic("Dexcom sign-in completed; account saved, uploads disabled");
                status = "Signed in; uploads disabled";
            }
            return "";
        } catch (const Error &e) {
            caught("complete sign-in");
            std::lock_guard lock(mutex);
            if (generation == revision)
                status = e.what();
            return e.what();
        } catch (...) {
            caught("complete sign-in");
            std::lock_guard lock(mutex);
            if (generation == revision)
                status = "Could not complete Dexcom sign-in";
            return "Could not complete Dexcom sign-in";
        }
    }
    std::string import(std::string_view body) {
        start();
        std::lock_guard network(networkMutex);
        try {
            auto c = Json::parse(body);
            validateConfig(c);
            std::lock_guard lock(mutex);
            if (settings->data()->clarity.enabled)
                throw Error("Turn off Clarity uploads before changing the account");
            acceptConfig(std::move(c));
            loginAttempt = nullptr;
            AccountStore(root + "/pending-login.map").clear();
            diagnostic("account imported; uploads disabled");
            status = "Account imported; uploads disabled";
            return "";
        } catch (const Error &e) {
            caught("import account");
            return e.what();
        } catch (...) {
            caught("import account");
            return "Invalid or incomplete Clarity setup file";
        }
    }
};
static Worker &worker() {
    static Worker instance;
    return instance;
}
} // namespace clarity
void startclaritythread() {
    try {
        clarity::worker().start();
    } catch (...) {
        clarity::caught("start upload worker");
    }
}
void wakeclarity() { clarity::worker().wake(); }
void claritynums(Numdata *n) { clarity::worker().add(n); }
void claritynumsremove(Numdata *n) { clarity::worker().remove(n); }
#endif
namespace {
// No C++ exception may escape a JNI entry point. Never log input strings or
// parser messages: the login callback and import file contain credentials.
template <class F>
jstring clarityString(JNIEnv *env, const char *operation, const char *fallback, F action) {
    try {
        auto value = action();
        if (env->ExceptionCheck()) {
            LOGGER("Clarity: %s: pending JNI exception\n", operation);
            return nullptr;
        }
        auto result = env->NewStringUTF(value.c_str());
        if (!result)
            LOGGER("Clarity: %s: NewStringUTF failed\n", operation);
        return result;
    } catch (...) {
#ifdef JUGGLUCO_CLARITY
        clarity::caught(operation);
#else
        LOGGER("Clarity: %s: unexpected exception\n", operation);
#endif
    }
    if (env->ExceptionCheck())
        return nullptr;
    auto result = env->NewStringUTF(fallback);
    if (!result)
        LOGGER("Clarity: %s: NewStringUTF fallback failed\n", operation);
    return result;
}
#ifdef JUGGLUCO_CLARITY
std::string clarityInput(JNIEnv *env, jstring input) {
    auto value = env->GetStringUTFChars(input, nullptr);
    if (!value) {
        LOGGER("Clarity: JNI GetStringUTFChars failed\n");
        return {};
    }
    struct Release {
        JNIEnv *env;
        jstring input;
        const char *value;
        ~Release() { env->ReleaseStringUTFChars(input, value); }
    } release{env, input, value};
    return value;
}
#endif
} // namespace
extern "C" JNIEXPORT jobject JNICALL fromjava(clarityStatus)(JNIEnv *env, jclass) {
    clarity::Status value;
    try {
#ifdef JUGGLUCO_CLARITY
        clarity::worker().start();
        value = clarity::worker().view();
#else
        value.status = "Clarity requires an ARM mobile build with CLARITY=ON";
#endif
    } catch (...) {
#ifdef JUGGLUCO_CLARITY
        clarity::caught("read status");
#else
        LOGGER("Clarity: read status: unexpected exception\n");
#endif
        value.status = "Cannot read Clarity settings";
    }
    auto type = env->FindClass("tk/glucodata/ClarityStatus");
    if (!type) {
        LOGGER("Clarity: ClarityStatus class lookup failed\n");
        return nullptr;
    }
    auto ctor = env->GetMethodID(type, "<init>", "()V");
    auto result = ctor ? env->NewObject(type, ctor) : nullptr;
    if (!result) {
        LOGGER("Clarity: ClarityStatus construction failed\n");
        env->DeleteLocalRef(type);
        return nullptr;
    }
    bool ok = true;
    auto field = [&](const char *name, const char *signature) -> jfieldID {
        if (!ok || env->ExceptionCheck())
            return nullptr;
        auto id = env->GetFieldID(type, name, signature);
        if (!id) {
            LOGGER("Clarity: status field lookup failed: %s\n", name);
            ok = false;
        }
        return id;
    };
    auto boolean = [&](const char *name, bool v) {
        if (auto id = field(name, "Z")) env->SetBooleanField(result, id, v);
    };
    auto number = [&](const char *name, int64_t v) {
        if (auto id = field(name, "J")) env->SetLongField(result, id, v);
    };
    auto string = [&](const char *name, const std::string &v) {
        if (auto id = field(name, "Ljava/lang/String;")) {
            auto text = env->NewStringUTF(v.c_str());
            if (text) {
                env->SetObjectField(result, id, text);
                env->DeleteLocalRef(text);
            } else {
                LOGGER("Clarity: status string allocation failed: %s\n", name);
                ok = false;
            }
        }
    };
    boolean("available", value.available);
    boolean("enabled", value.enabled);
    boolean("libre3History", value.libre3History);
    boolean("numbers", value.numbers);
    number("since", value.since);
    number("lastSuccess", value.lastSuccess);
    number("acknowledgedCount", value.acknowledgedCount);
    number("acknowledgedLast", value.acknowledgedLast);
    number("pendingCount", value.pendingCount);
    number("pendingFirst", value.pendingFirst);
    number("pendingLast", value.pendingLast);
    string("account", value.account);
    string("status", value.status);
    env->DeleteLocalRef(type);
    if (!ok || env->ExceptionCheck()) {
        LOGGER("Clarity: status field assignment failed\n");
        env->DeleteLocalRef(result);
        return nullptr;
    }
    return result;
}
extern "C" JNIEXPORT jintArray JNICALL fromjava(clarityCategories)(JNIEnv *env, jclass) {
    try {
#ifdef JUGGLUCO_CLARITY
        const auto kinds = clarity::worker().categories();
        auto result = env->NewIntArray(kinds.size());
        if (!result) {
            LOGGER("Clarity: category array allocation failed\n");
            return nullptr;
        }
        env->SetIntArrayRegion(result, 0, kinds.size(), kinds.data());
        if (env->ExceptionCheck()) {
            LOGGER("Clarity: category array assignment failed\n");
            return nullptr;
        }
        return result;
#else
        LOGGER("Clarity: categories requested but support was not built\n");
#endif
    } catch (...) {
#ifdef JUGGLUCO_CLARITY
        clarity::caught("read categories");
#else
        LOGGER("Clarity: categories failed\n");
#endif
    }
    return nullptr;
}
extern "C" JNIEXPORT jstring JNICALL fromjava(claritySetCategories)(
    JNIEnv *env, jclass, jintArray categories, jfloatArray weights) {
    return clarityString(env, "save categories", "Cannot save categories", [&]() -> std::string {
#ifdef JUGGLUCO_CLARITY
        if (!categories || !weights)
            throw clarity::Error("Missing categories");
        const auto count = env->GetArrayLength(categories);
        if (count > maxvarnr || env->GetArrayLength(weights) != count)
            throw clarity::Error("Invalid category count");
        std::vector<int> kinds(count);
        std::vector<float> factors(count);
        env->GetIntArrayRegion(categories, 0, count, kinds.data());
        if (env->ExceptionCheck()) {
            LOGGER("Clarity: reading category array failed\n");
            return {};
        }
        env->GetFloatArrayRegion(weights, 0, count, factors.data());
        if (env->ExceptionCheck()) {
            LOGGER("Clarity: reading category weights failed\n");
            return {};
        }
        return clarity::worker().setCategories(kinds, factors);
#else
        LOGGER("Clarity: save categories requested but support was not built\n");
        return "Clarity support was not built";
#endif
    });
}
extern "C" JNIEXPORT jstring JNICALL fromjava(clarityConfigure)(JNIEnv *env, jclass,
                                                                jboolean enabled, jboolean history,
                                                                jboolean numbers, jlong since) {
    return clarityString(env, "save settings", "Cannot save Clarity settings",
                         [&]() -> std::string {
#ifdef JUGGLUCO_CLARITY
                             return clarity::worker().options(enabled, history, numbers, since);
#else
        LOGGER("Clarity: configure requested but Clarity support was not built\n");
        return "Clarity support was not built";
#endif
                         });
}
extern "C" JNIEXPORT jstring JNICALL fromjava(clarityImport)(JNIEnv *env, jclass, jbyteArray data) {
    return clarityString(
        env, "import account", "Cannot import Clarity account", [&]() -> std::string {
#ifdef JUGGLUCO_CLARITY
            if (!data || env->GetArrayLength(data) > 128 * 1024)
                throw clarity::Error("Invalid setup file size");
            clarity::Bytes b(env->GetArrayLength(data));
            env->GetByteArrayRegion(data, 0, b.size(), reinterpret_cast<jbyte *>(b.data()));
            if (env->ExceptionCheck()) {
                LOGGER("Clarity: JNI GetByteArrayRegion failed\n");
                return {};
            }
            return clarity::worker().import(clarity::text(b));
#else
        LOGGER("Clarity: import requested but Clarity support was not built\n");
        return "Clarity support was not built";
#endif
        });
}
extern "C" JNIEXPORT void JNICALL fromjava(clarityWake)(JNIEnv *, jclass) {
    try {
        wakeclarity();
    } catch (...) {
#ifdef JUGGLUCO_CLARITY
        clarity::caught("wake upload worker");
#else
        LOGGER("Clarity: wake failed with an exception\n");
#endif
    }
}
// [0] authorization URL, [1] error; no serialized JSON across JNI.
extern "C" JNIEXPORT jobjectArray JNICALL fromjava(clarityLoginBegin)(
    JNIEnv *env, jclass, jstring country, jstring locale, jstring manufacturer,
    jstring model, jstring osVersion) {
    std::array<std::string, 2> value{"", "Cannot begin Dexcom sign-in"};
    try {
#ifdef JUGGLUCO_CLARITY
        const std::array<jstring, 5> input{country, locale, manufacturer, model, osVersion};
        std::array<std::string, 5> copy;
        for (size_t i = 0; i < input.size(); ++i) {
            if (!input[i] || env->GetStringUTFLength(input[i]) > 1024)
                throw clarity::Error("Invalid login settings");
            copy[i] = clarityInput(env, input[i]);
            if (env->ExceptionCheck())
                return nullptr;
        }
        value = clarity::worker().loginBegin(copy);
#else
        LOGGER("Clarity: sign-in requested but support was not built\n");
        value[1] = "Clarity support was not built";
#endif
    } catch (...) {
#ifdef JUGGLUCO_CLARITY
        clarity::caught("begin sign-in");
#else
        LOGGER("Clarity: begin sign-in: unexpected exception\n");
#endif
    }
    auto type = env->FindClass("java/lang/String");
    if (!type) {
        LOGGER("Clarity: String class lookup failed\n");
        return nullptr;
    }
    auto result = env->NewObjectArray(2, type, nullptr);
    env->DeleteLocalRef(type);
    if (!result) {
        LOGGER("Clarity: login result array allocation failed\n");
        return nullptr;
    }
    for (size_t i = 0; i < value.size(); ++i) {
        auto item = env->NewStringUTF(value[i].c_str());
        if (!item) {
            LOGGER("Clarity: login result string allocation failed\n");
            return nullptr;
        }
        env->SetObjectArrayElement(result, i, item);
        env->DeleteLocalRef(item);
        if (env->ExceptionCheck()) {
            LOGGER("Clarity: login result assignment failed\n");
            return nullptr;
        }
    }
    return result;
}
extern "C" JNIEXPORT jstring JNICALL fromjava(clarityLoginFinish)(JNIEnv *env, jclass,
                                                                  jstring input) {
    return clarityString(env, "complete sign-in", "Cannot complete Dexcom sign-in",
                         [&]() -> std::string {
#ifdef JUGGLUCO_CLARITY
                             if (!input || env->GetStringUTFLength(input) > 64 * 1024)
                                 throw clarity::Error("Invalid login response");
                             auto copy = clarityInput(env, input);
                             if (env->ExceptionCheck())
                                 return {};
                             return clarity::worker().loginFinish(copy);
#else
        LOGGER("Clarity: complete sign-in requested but Clarity support was not built\n");
        return "Clarity support was not built";
#endif
                         });
}
