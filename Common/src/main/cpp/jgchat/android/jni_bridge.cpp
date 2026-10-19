// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "jgchat/client.hpp"
#include "jgchat/storage.hpp"
#include "jgchat/diagnostics.hpp"
#include "jgchat/files.hpp"
#include "jgchat/chat_export.hpp"
#include "jgchat/workspace.hpp"
#include "juggluco_data.hpp"
#include "juggluco_extra.hpp"
#include "juggluco_system.hpp"
#include <jni.h>
#include <memory>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace {
using namespace jgchat;
struct State {
    std::mutex dispatch, mutex;
    std::atomic_bool cancel{false};
    std::thread worker;
    std::unique_ptr<Storage> storage;
    std::unique_ptr<ChatClient> client;
    std::unique_ptr<FileStore> files;
    std::unique_ptr<Workspace> workspace;
    std::string directory, account_id;
    bool busy = false, logged_in = false;
    UiMessage status = UiCode::sign_in_to_ask_a_question, error, completed_status;
    std::string login_url, user_code;
    Json models = Json::array(), messages = Json::array();
    Json activity = Json::array();
    Json pending_browser = nullptr;
    uint64_t operation_id = 0;
    std::string operation, pending_question;
    std::chrono::steady_clock::time_point started{};
    long long elapsed_ms = 0;
    ~State() { cancel = true; if (worker.joinable()) worker.join(); }
};
State state;

// JNI uses modified UTF-8. Convert Java UTF-16 explicitly so emoji and other
// supplementary characters remain ordinary UTF-8 in JSON sent to the server.
std::string from_java(JNIEnv* env, jstring text) {
    if (!text) return {};
    const auto size = env->GetStringLength(text);
    const jchar* raw = env->GetStringChars(text, nullptr);
    if (!raw) throw jgchat::UiError(jgchat::UiCode::cannot_read_text);
    std::string out;
    try {
        for (jsize i = 0; i < size; ++i) {
            unsigned cp = raw[i];
            if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < size && raw[i + 1] >= 0xdc00 && raw[i + 1] <= 0xdfff)
                cp = 0x10000 + ((cp - 0xd800) << 10) + (raw[++i] - 0xdc00);
            else if (cp >= 0xd800 && cp <= 0xdfff) cp = 0xfffd;
            if (cp < 0x80) out += static_cast<char>(cp);
            else if (cp < 0x800) { out += static_cast<char>(0xc0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 63)); }
            else if (cp < 0x10000) { out += static_cast<char>(0xe0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 63)); out += static_cast<char>(0x80 | (cp & 63)); }
            else { out += static_cast<char>(0xf0 | (cp >> 18)); out += static_cast<char>(0x80 | ((cp >> 12) & 63)); out += static_cast<char>(0x80 | ((cp >> 6) & 63)); out += static_cast<char>(0x80 | (cp & 63)); }
        }
    } catch (...) { env->ReleaseStringChars(text, raw); throw; }
    env->ReleaseStringChars(text, raw);
    return out;
}
jstring to_java(JNIEnv* env, std::string_view text) {
    std::u16string out;
    for (std::size_t i = 0; i < text.size();) {
        unsigned char first = text[i++];
        unsigned cp = first;
        unsigned count = 0;
        if (first >= 0xc2 && first <= 0xdf) { cp = first & 31; count = 1; }
        else if (first >= 0xe0 && first <= 0xef) { cp = first & 15; count = 2; }
        else if (first >= 0xf0 && first <= 0xf4) { cp = first & 7; count = 3; }
        else if (first >= 0x80) cp = 0xfffd;
        const unsigned total = count;
        for (; count; --count) {
            if (i >= text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80) { cp = 0xfffd; break; }
            cp = (cp << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        }
        if ((total == 1 && cp < 0x80) || (total == 2 && cp < 0x800) ||
            (total == 3 && cp < 0x10000) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) cp = 0xfffd;
        if (cp < 0x10000) out += static_cast<char16_t>(cp);
        else { cp -= 0x10000; out += static_cast<char16_t>(0xd800 + (cp >> 10)); out += static_cast<char16_t>(0xdc00 + (cp & 1023)); }
    }
    if (out.size() > std::size_t(std::numeric_limits<jsize>::max())) throw jgchat::UiError(jgchat::UiCode::text_is_too_long);
    return env->NewString(reinterpret_cast<const jchar*>(out.data()), static_cast<jsize>(out.size()));
}
Json ui_json(const UiMessage& message) {
    if (message.empty()) return nullptr;
    return {{"id", static_cast<unsigned>(message.code)}, {"format", static_cast<unsigned>(message.format)}, {"args", message.args}};
}
jobject to_java_message(JNIEnv* env, const UiMessage& message) {
    if (message.empty()) return nullptr;
    // Invoked only on a Java-entered thread; FindClass uses its app loader.
    jclass type = env->FindClass("tk/glucodata/JugglucoChat$NativeMessage");
    if (!type) return nullptr;
    jmethodID constructor = env->GetMethodID(type, "<init>", "(II[Ljava/lang/String;)V");
    if (!constructor) { env->DeleteLocalRef(type); return nullptr; }
    jclass string_type = env->FindClass("java/lang/String");
    if (!string_type) { env->DeleteLocalRef(type); return nullptr; }
    jobjectArray args = env->NewObjectArray(static_cast<jsize>(message.args.size()), string_type, nullptr);
    env->DeleteLocalRef(string_type);
    if (!args) { env->DeleteLocalRef(type); return nullptr; }
    for (std::size_t i = 0; i < message.args.size(); ++i) {
        jstring value = to_java(env, message.args[i]);
        if (!value) { env->DeleteLocalRef(args); env->DeleteLocalRef(type); return nullptr; }
        env->SetObjectArrayElement(args, static_cast<jsize>(i), value);
        env->DeleteLocalRef(value);
    }
    jobject out = env->NewObject(type, constructor, static_cast<jint>(message.code), static_cast<jint>(message.format), args);
    env->DeleteLocalRef(args); env->DeleteLocalRef(type);
    return out;
}
template<class F> jobject result(JNIEnv* env, F action) {
    try { action(); return nullptr; }
    catch (const Json::exception&) {
        diagnostic("jni call failed category=json");
        return to_java_message(env, UiCode::invalid_chat_state_or_server_response);
    }
    catch (...) {
        auto message = current_ui_error();
        diagnostic("jni call failed message_id=%u", static_cast<unsigned>(message.code));
        return to_java_message(env, message);
    }
}
void progress(const UiMessage& message) {
    std::lock_guard lock(state.mutex);
    if (state.status == message) return;
    state.status = message;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - state.started).count();
    // Coalesce public summary fragments by their ID, never by English text.
    Json entry{{"elapsed_ms", elapsed}, {"message", ui_json(message)}};
    if (message.code == UiCode::thinking_summary && !state.activity.empty() &&
        state.activity.back()["message"].value("id", 0u) == static_cast<unsigned>(UiCode::thinking_summary))
        state.activity.back() = std::move(entry);
    else state.activity.push_back(std::move(entry));
    if (state.activity.size() > 20) state.activity.erase(state.activity.begin());
}

void make_client(const Tokens& tokens) {
    auto workspace = std::make_unique<Workspace>(state.directory,tokens.account_id,state.files.get());
    state.client = std::make_unique<ChatClient>(native_https(), tokens,
        [](std::string_view name, const Json& args) { return execute_data_tool(name, args, &state.cancel); },
        [](const Tokens& next) {
            diagnostic("tokens saving rotated credentials");
            state.storage->write("tokens.json", tokens_to_json(next));
            diagnostic("tokens rotated credentials saved");
        }, workspace.get(), jgchatdata::glucose_display_preferences);
    state.workspace = std::move(workspace);
    state.account_id = tokens.account_id;
}
void require_idle() {
    std::lock_guard lock(state.mutex);
    if (state.busy) throw jgchat::UiError(jgchat::UiCode::cancel_the_current_operation_and_wait_for_it_to_finish);
    if (!state.storage) throw jgchat::UiError(jgchat::UiCode::chat_storage_has_not_been_initialized);
}
void require_signed_in() {
    if (!state.client) throw jgchat::UiError(jgchat::UiCode::sign_in_first);
}
void load_models() {
    progress(UiCode::loading_available_models);
    auto models = state.client->list_models(state.cancel);
    if (state.cancel) return;
    std::lock_guard lock(state.mutex);
    state.models = std::move(models);
    state.status = state.models.empty() ? UiCode::no_models_returned_reload_models : UiCode::ready;
    state.completed_status = state.status;
}
template<class F> void start(const char* name, F operation, std::string question = {}) {
    // JNI starts/resets are serialized separately from the short snapshot lock.
    // This lets the UI cancel and poll without waiting on TLS or device login.
    std::lock_guard dispatch(state.dispatch);
    require_idle();
    if (state.worker.joinable()) state.worker.join();
    {
        std::lock_guard lock(state.mutex);
        state.busy = true; state.cancel = false; state.error.clear();
        ++state.operation_id; state.operation = name;
        state.pending_question = std::move(question);
        state.completed_status.clear();
        state.status = UiCode::connecting;
        state.activity = Json::array(); state.elapsed_ms = 0;
        state.pending_browser = nullptr;
        state.started = std::chrono::steady_clock::now();
    }
    try {
        state.worker = std::thread([operation, name] {
            DiagnosticOperation trace(name);
            UiMessage error;
            try { operation(); }
            catch (const Json::exception&) { error = UiCode::invalid_chat_state_or_server_response; }
            catch (...) { error = current_ui_error(); }
            std::lock_guard lock(state.mutex);
            // Cancellation must not conceal a failed credential/history save.
            // Explicit cancellation codes distinguish cancellation from save failures.
            const bool ordinary_cancel = state.cancel && (error.empty() ||
                is_cancellation(error));
            if (!error.empty() && !ordinary_cancel) {
                state.status = UiCode::operation_failed; state.error = std::move(error);
            }
            else if (state.cancel) {
                state.status = state.completed_status.empty() ? UiMessage(UiCode::cancelled) : state.completed_status;
                state.error.clear();
            }
            if (state.error.empty()) {
                if (ordinary_cancel && state.completed_status.empty()) trace.cancelled();
                else trace.success();
            }
            state.busy = false;
            state.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - state.started).count();
            state.login_url.clear(); state.user_code.clear();
        });
    } catch (...) {
        std::lock_guard lock(state.mutex); state.busy = false;
        throw;
    }
}
// Called with state.mutex held. The hidden view needs only this small snapshot
// to release its operation wake lock, not the complete conversation/attachments.
Json work_snapshot() {
    const auto elapsed = state.busy ? std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - state.started).count() : state.elapsed_ms;
    return {{"busy", state.busy}, {"operation_id", state.operation_id}, {"operation", state.operation},
        {"pending_question", state.pending_question}, {"elapsed_ms", elapsed},
        {"status", ui_json(state.status)}, {"error", ui_json(state.error)}, {"logged_in", state.logged_in}};
}
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeInitWithFilesMessages(JNIEnv* env, jclass, jstring directory, jstring files_directory) {
    return result(env, [&] {
        DiagnosticOperation trace("initialize");
        const auto path = from_java(env, directory);
        std::lock_guard dispatch(state.dispatch);
        if (state.storage) {
            if (path != state.directory) throw jgchat::UiError(jgchat::UiCode::chat_storage_is_already_initialized_elsewhere);
            trace.success();
            return;
        }
        auto storage = std::make_unique<Storage>(path);
        auto files = std::make_unique<FileStore>(from_java(env, files_directory));
        state.files = std::move(files);
        state.storage = std::move(storage); state.directory = path;
        std::lock_guard lock(state.mutex);
        UiMessage error;
        try {
            if (auto saved = state.storage->read("tokens.json")) {
                make_client(tokens_from_json(*saved));
                state.logged_in = true;
                state.status = UiCode::signed_in_select_a_model;
            }
        } catch (...) { state.client.reset(); error = UiCode::saved_sign_in_could_not_be_read_sign_in_again; }
        try {
            if (state.client) if (auto saved = state.storage->read("conversation.json")) {
                if (saved->value("account_id", "") == state.account_id) {
                    const auto& messages = saved->at("messages");
                    if (!messages.is_array())
                        throw jgchat::UiError(jgchat::UiCode::invalid_transcript);
                    for (const auto& m : messages) {
                        if (!m.is_object() || !m.contains("text") || !m.at("text").is_string() ||
                            (m.value("role", "") != "user" && m.value("role", "") != "assistant"))
                            throw jgchat::UiError(jgchat::UiCode::invalid_transcript);
                    }
                    state.client->restore_history(saved->at("history"));
                    if (saved->contains("workspace")) state.workspace->restore(saved->at("workspace"));
                    state.messages = messages;
                }
            }
        } catch (...) { if (state.client) state.client->reset(); error = UiCode::saved_conversation_or_analysis_index_could_not_be_read_starting_a_new_chat; }
        diagnostic("initialize logged_in=%d messages=%zu saved_state_error=%d", state.logged_in ? 1 : 0,
            state.messages.size(), error.empty() ? 0 : 1);
        state.error = std::move(error);
        trace.success();
    });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeStartLoginMessages(JNIEnv* env, jclass) {
    return result(env, [] { start("login", [] {
        const auto http = native_https();
        const auto code = request_device_code(http, state.cancel);
        diagnostic("login device code received; waiting for authorization");
        {
            std::lock_guard lock(state.mutex);
            state.login_url = code.verification_url; state.user_code = code.user_code;
            state.status = UiCode::open_the_sign_in_page_and_enter_the_code;
        }
        const auto tokens = complete_device_login(http, code, state.cancel);
        // A successful exchange may race Cancel. Persist its credentials before
        // finishing, just as we must persist a successfully rotated refresh token.
        diagnostic("login saving credentials");
        state.storage->write("tokens.json", tokens_to_json(tokens));
        diagnostic("login credentials saved");
        make_client(tokens);
        {
            std::lock_guard lock(state.mutex);
            state.logged_in = true; state.messages = Json::array(); state.models = Json::array();
            state.login_url.clear(); state.user_code.clear(); state.status = UiCode::signed_in;
            state.completed_status = state.status;
        }
        diagnostic("login resetting saved conversation");
        state.storage->write("conversation.json", {{"account_id", state.account_id},
            {"history", Json::array()}, {"messages", Json::array()}, {"workspace",state.workspace->snapshot()}});
        state.workspace->commit(); // Remove bodies left by the previous account.
        if (state.cancel) return;
        load_models();
    }); });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeLoadModelsMessages(JNIEnv* env, jclass) {
    return result(env, [] { start("load_models", [] { require_signed_in(); load_models(); }); });
}
// A distinct JNI name makes a mixed old/new Java/native installation fail with
// a caught missing-method error rather than calling an incompatible signature.
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeSendWithOptionsMessages(JNIEnv* env, jclass, jstring question, jstring model, jstring effort, jboolean internet) {
    return result(env, [&] {
        auto q = from_java(env, question), m = from_java(env, model), e = from_java(env, effort);
        if (q.empty()) throw jgchat::UiError(jgchat::UiCode::enter_a_question_shorter_than_32_kib);
        if (m.empty()) throw jgchat::UiError(jgchat::UiCode::select_a_model);
        const auto pending = q;
        start("question", [q = std::move(q), m = std::move(m), e = std::move(e), internet] {
            require_signed_in();
            const auto previous = state.client->history();
            Json saved_files = Json::array();
            state.workspace->begin();
            try {
                // Upgrade old file references without exposing other accounts'
                // generated bundles in the shared local web-file folder.
                Json old_messages;
                { std::lock_guard lock(state.mutex); old_messages = state.messages; }
                for (const auto& message : old_messages) if (message.contains("files")) state.workspace->remember_files(message.at("files"));
                auto answer = state.client->ask(q, m, state.cancel, progress, e, internet);
                if (state.cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
                if (!state.client->files().empty()) {
                    progress(UiCode::saving_generated_files);
                    saved_files = state.files->save(state.client->files(), state.cancel);
                    diagnostic("generated files saved bundles=%zu", saved_files.size());
                }
                Json messages;
                std::string model_name = m;
                {
                    std::lock_guard lock(state.mutex);
                    messages = state.messages;
                    for (const auto& candidate : state.models) if (candidate.value("id", "") == m) {
                        const auto display = candidate.value("display_name", "");
                        if (!display.empty()) model_name = display;
                        break;
                    }
                }
                messages.push_back({{"role", "user"}, {"text", q}});
                messages.push_back({{"role", "assistant"}, {"text", answer}, {"plots", state.client->plots()},
                    {"files", saved_files}, {"citations", state.client->citations()},
                    {"model_id", m}, {"model_name", model_name}, {"reasoning_effort", e.empty() ? "default" : e}});
                state.workspace->remember_files(saved_files);
                if (state.cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
                Json open_file = nullptr;
                for (const auto& bundle : saved_files) if (open_file.is_null() && bundle.value("open_in_browser", false))
                    open_file = {{"id", bundle["id"]}, {"file", bundle["entrypoint"]}};
                diagnostic("conversation saving messages=%zu", messages.size());
                state.storage->write("conversation.json", {{"account_id", state.account_id},
                    {"history", state.client->history()}, {"messages", messages}, {"workspace",state.workspace->snapshot()}});
                state.workspace->commit();
                std::lock_guard lock(state.mutex);
                state.messages = std::move(messages); state.status = UiCode::ready;
                state.pending_question.clear();
                state.pending_browser = std::move(open_file);
                state.completed_status = state.status;
                diagnostic("conversation saved messages=%zu", state.messages.size());
            } catch (...) {
                state.workspace->rollback();
                state.files->rollback(saved_files);
                diagnostic("question failed; restoring previous conversation"); state.client->restore_history(previous); throw;
            }
        }, pending);
    });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeCancelMessages(JNIEnv* env, jclass) {
    return result(env, [] { diagnostic("cancel requested"); state.cancel = true; });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeNewChatMessages(JNIEnv* env, jclass) {
    return result(env, [] {
        DiagnosticOperation trace("new_chat");
        std::lock_guard dispatch(state.dispatch); require_idle();
        if (state.workspace) {
            state.workspace->begin();
            try {
                auto history = state.client->history();
                state.workspace->import_history(history);
                for (const auto& message : state.messages) if (message.contains("files")) state.workspace->remember_files(message.at("files"));
                state.storage->write("conversation.json", {{"account_id",state.account_id},{"history",Json::array()},
                    {"messages",Json::array()},{"workspace",state.workspace->snapshot()}});
                state.workspace->commit();
            } catch (...) { state.workspace->rollback(); throw; }
        } else state.storage->remove("conversation.json");
        if (state.client) state.client->reset();
        std::lock_guard lock(state.mutex);
        state.messages = Json::array(); state.activity = Json::array(); state.error.clear();
        state.pending_browser = nullptr;
        state.pending_question.clear();
        state.status = state.logged_in ? UiCode::ready : UiCode::sign_in_to_ask_a_question;
        trace.success();
    });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeClearAnalysisMessages(JNIEnv* env, jclass) {
    return result(env, [] {
        std::lock_guard dispatch(state.dispatch); require_idle(); require_signed_in();
        state.workspace->begin();
        try {
            state.workspace->clear();
            state.storage->write("conversation.json",{{"account_id",state.account_id},{"history",Json::array()},
                {"messages",Json::array()},{"workspace",state.workspace->snapshot()}});
            state.workspace->commit();
        } catch (...) { state.workspace->rollback(); throw; }
        state.client->reset();
        std::lock_guard lock(state.mutex);
        state.messages = Json::array(); state.activity = Json::array(); state.error.clear();
        state.pending_question.clear(); state.pending_browser = nullptr; state.status = UiCode::analysis_memory_cleared;
        diagnostic("analysis memory cleared");
    });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeLogoutMessages(JNIEnv* env, jclass) {
    return result(env, [] {
        DiagnosticOperation trace("logout");
        std::lock_guard dispatch(state.dispatch); require_idle();
        bool token_cleanup_failed = false, history_cleanup_failed = false;
        try { state.storage->remove("tokens.json"); } catch (...) { token_cleanup_failed = true; }
        state.client.reset(); state.account_id.clear();
        {
            std::lock_guard lock(state.mutex);
            state.logged_in = false; state.messages = Json::array(); state.models = Json::array(); state.activity = Json::array();
            state.pending_browser = nullptr;
            state.pending_question.clear();
            state.error.clear(); state.login_url.clear(); state.user_code.clear();
            state.status = UiCode::signed_out;
        }
        try { state.storage->remove("conversation.json"); } catch (...) { history_cleanup_failed = true; }
        if (state.workspace) {
            try { state.workspace->begin(); state.workspace->clear(); state.workspace->commit(); }
            catch (...) { history_cleanup_failed = true; }
            state.workspace.reset();
        }
        diagnostic("logout credentials_cleanup_failed=%d history_cleanup_failed=%d",
            token_cleanup_failed ? 1 : 0, history_cleanup_failed ? 1 : 0);
        if (token_cleanup_failed)
            throw jgchat::UiError(jgchat::UiCode::signed_out_for_this_run_but_saved_credentials_could_not_be_removed_retry_sign_out);
        if (history_cleanup_failed)
            throw jgchat::UiError(jgchat::UiCode::signed_out_but_the_saved_conversation_could_not_be_removed_retry_sign_out);
        trace.success();
    });
}
extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativePollMessages(JNIEnv* env, jclass) {
    try {
        std::lock_guard lock(state.mutex);
        auto display_messages = state.messages;
        for (auto& message : display_messages) if (message.contains("plots") && message["plots"].is_array())
            for (auto& plot : message["plots"]) if (plot.is_object()) plot.erase("svg");
        auto snapshot = work_snapshot();
        snapshot.update({{"login_url", state.login_url}, {"user_code", state.user_code},
            {"models", state.models}, {"messages", std::move(display_messages)}, {"activity", state.activity}});
        return to_java(env, snapshot.dump());
    } catch (...) {
        diagnostic("poll state serialization failed");
        return nullptr;
    }
}

extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativePollWorkMessages(JNIEnv* env, jclass) {
    try {
        std::lock_guard lock(state.mutex);
        return to_java(env, work_snapshot().dump());
    } catch (...) { return nullptr; }
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeSetWearNodesMessages(JNIEnv* env, jclass, jstring raw) {
    return result(env, [&] { jgchatdata::update_wear_nodes(Json::parse(from_java(env, raw))); });
}
extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeSetPhoneActivityMessages(JNIEnv* env, jclass, jstring raw) {
    return result(env, [&] { jgchatdata::update_phone_activity(Json::parse(from_java(env, raw))); });
}

extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativeGetPlot(JNIEnv* env, jclass, jint message, jint plot) {
    try {
        std::lock_guard lock(state.mutex);
        if (message < 0 || plot < 0 || static_cast<std::size_t>(message) >= state.messages.size()) return nullptr;
        const auto& plots = state.messages.at(message).at("plots");
        if (!plots.is_array() || static_cast<std::size_t>(plot) >= plots.size()) return nullptr;
        return to_java(env, plots.at(plot).at("svg").get_ref<const std::string&>());
    } catch (...) { return nullptr; }
}

extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativeListFilesMessages(JNIEnv* env, jclass) {
    try {
        std::lock_guard dispatch(state.dispatch);
        if (!state.files) throw jgchat::UiError(jgchat::UiCode::chat_storage_is_not_initialized);
        return to_java(env, Json{{"files", state.files->list()}}.dump());
    } catch (const std::exception&) { return to_java(env, Json{{"error", ui_json(UiCode::cannot_list_saved_files)}}.dump()); }
}
extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativeFileUrlMessages(JNIEnv* env, jclass, jstring id, jstring file) {
    try {
        std::lock_guard dispatch(state.dispatch);
        if (!state.files) throw jgchat::UiError(jgchat::UiCode::chat_storage_is_not_initialized);
        const auto relative = state.files->relative_path(from_java(env, id), from_java(env, file));
        return to_java(env, Json{{"url", jgchatdata::web_file_url(relative)}}.dump());
    } catch (const std::exception& e) { return to_java(env, Json{{"error", ui_json(current_ui_error())}}.dump()); }
}
extern "C" JNIEXPORT jstring JNICALL Java_tk_glucodata_JugglucoChat_nativeTakeBrowserFile(JNIEnv* env, jclass) {
    try {
        std::lock_guard lock(state.mutex);
        if (state.busy || state.pending_browser.is_null()) return nullptr;
        auto out = to_java(env, state.pending_browser.dump());
        state.pending_browser = nullptr;
        return out;
    } catch (...) { return nullptr; }
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeExportFileMessages(
        JNIEnv* env, jclass, jstring files_directory, jstring id, jstring file, jint descriptor) {
    return result(env, [&] {
        diagnostic("document export started");
        // The picker can outlive the chat and even process recreation. This
        // reads persisted bundles independently of sign-in or the chat worker.
        FileStore store(from_java(env, files_directory));
        const auto bytes = store.copy_to_fd(from_java(env, id), from_java(env, file), descriptor);
        diagnostic("document export copied bytes=%zu", bytes);
        // Java owns the descriptor and reports provider/close errors too.
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeWriteChatExportWithStringsMessages(
        JNIEnv* env, jclass, jstring format, jint descriptor, jstring strings) {
    return result(env, [&] {
        Json messages;
        std::string pending;
        bool busy;
        {
            std::lock_guard lock(state.mutex);
            messages = state.messages; pending = state.pending_question; busy = state.busy;
        }
        // Rendering and descriptor I/O must not block polling or the answer
        // worker. Copy only the public transcript, including original SVGs.
        const auto labels = Json::parse(from_java(env, strings));
        if (!labels.is_object() || labels.size() > 32)
            throw jgchat::UiError(jgchat::UiCode::invalid_chat_state_or_server_response);
        for (const auto& value : labels)
            if (!value.is_string() || value.get_ref<const std::string&>().size() > 4096)
                throw jgchat::UiError(jgchat::UiCode::invalid_chat_state_or_server_response);
        const auto contents = render_chat_export(messages, pending, busy, from_java(env, format), -1, -1, labels);
        const auto bytes = write_chat_export(descriptor, contents);
        diagnostic("chat export prepared messages=%zu bytes=%zu", messages.size(), bytes);
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeWritePlotExportMessages(
        JNIEnv* env, jclass, jstring svg, jint descriptor) {
    return result(env, [&] {
        // Freeze the plot displayed in the viewer. A newly completed answer
        // can otherwise prune/reindex the transcript while that viewer is open.
        const auto contents = from_java(env, svg);
        if (!contents.starts_with("<svg "))
            throw jgchat::UiError(jgchat::UiCode::the_plot_is_unavailable);
        const auto bytes = write_chat_export(descriptor, contents);
        diagnostic("plot export prepared bytes=%zu", bytes);
    });
}

extern "C" JNIEXPORT jobject JNICALL Java_tk_glucodata_JugglucoChat_nativeCopyChatExportMessages(
        JNIEnv* env, jclass, jint input, jint output) {
    return result(env, [&] {
        const auto bytes = copy_chat_export(input, output);
        diagnostic("chat export copied bytes=%zu", bytes);
    });
}
