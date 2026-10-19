/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Thu Oct 15 12:53:38 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "jgchat/files.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <set>
#include <signal.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace jgchat {
namespace {
struct Fd { int fd; ~Fd() { if (fd >= 0) ::close(fd); } };
void fail(UiCode message) { throw UiError(message); }
void check(bool ok) { if (!ok) throw jgchat::UiArgumentError(jgchat::UiCode::invalid_file_bundle_use_1_8_text_files_safe_filenames_at_most_48_kib_total_and_an_htm); }
bool filename(std::string_view name, bool system = false) {
    if (name.empty() || name.front() == '.' || (!system && name.starts_with("jg-"))) return false;
    for (const unsigned char c : name)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return false;
    const auto dot = name.rfind('.');
    if (dot == std::string_view::npos || name.find("..") != std::string_view::npos) return false;
    const auto ext = name.substr(dot);
    return ext == ".html" || ext == ".htm" || ext == ".js" || ext == ".mjs" || ext == ".css" ||
        ext == ".json" || ext == ".csv" || ext == ".tsv" || ext == ".txt" || ext == ".md";
}
bool bundle_id(std::string_view id) {
    return id.size() == 32 && std::all_of(id.begin(), id.end(), [](char c) { return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9'); });
}
bool html(std::string_view name) { return name.ends_with(".html") || name.ends_with(".htm"); }
std::string escape(std::string_view text) {
    std::string out;
    for (const char c : text) {
        if (c == '&') out += "&amp;"; else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;"; else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;"; else out += c;
    }
    return out;
}
const char* api_js = R"JG("use strict";
(() => {
  const base = new URL("../../../", location.href);
  function url(path, parameters = {}) {
    if (typeof path !== "string" || !/^(api\/v[123]\/|x\/|pebble(?:\?|$)|sgv\.json(?:\?|$)|status\.json(?:\?|$))/.test(path))
      throw new Error("Use a relative Juggluco read endpoint, for example api/v1/entries.json");
    const target = new URL(path, base);
    if (target.origin !== base.origin || !target.pathname.startsWith(base.pathname) || path.includes("..") || path.includes("\\") || path.includes("#"))
      throw new Error("Invalid Juggluco endpoint");
    for (const [key, value] of Object.entries(parameters)) target.searchParams.set(key, String(value));
    return target.href;
  }
  async function request(path, parameters) {
    const result = await fetch(url(path, parameters), {method: "GET", cache: "no-store", credentials: "same-origin", redirect: "error", referrerPolicy: "no-referrer"});
    if (!result.ok) throw new Error("Juggluco returned HTTP " + result.status + ". Check that the web server is enabled and reopen this page from Juggluco if the secret or port changed.");
    return result;
  }
  Object.defineProperty(window, "Juggluco", {value: Object.freeze({url,
    fetch: request, json: async (path, params) => (await request(path, params)).json(),
    text: async (path, params) => (await request(path, params)).text()})});
  function error(message) {
    let box = document.getElementById("jg-runtime-error");
    if (!box) { box = document.createElement("pre"); box.id = "jg-runtime-error"; box.style.cssText = "white-space:pre-wrap;border:1px solid;padding:12px"; document.body.appendChild(box); }
    box.textContent = "Page error: " + message;
  }
  addEventListener("unhandledrejection", event => error(String(event.reason && event.reason.message || event.reason)));
  addEventListener("error", event => error(event.message || "A script could not be loaded"));
})();
)JG";
std::string page(std::string_view title, std::string_view body) {
    // The policy is emitted before any generated content, so later markup
    // cannot loosen it. Ordinary JS/Canvas/SVG and local assets remain usable.
    return "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta name=\"referrer\" content=\"no-referrer\"><meta http-equiv=\"Content-Security-Policy\" content=\""
        "default-src 'none'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; connect-src 'self'; "
        "img-src 'self' data: blob:; font-src 'self'; media-src 'self' blob:; object-src 'none'; base-uri 'none'; form-action 'none'; worker-src 'none'\">"
        "<title>" + escape(title) + "</title><style>:root{color-scheme:light dark}body{font:16px system-ui;margin:16px;background:Canvas;color:CanvasText}"
        "a{color:LinkText}button,input,select{font:inherit}pre{white-space:pre-wrap}</style><script src=\"jg-api.js\"></script></head><body>" +
        std::string(body) + "</body></html>";
}
int subdirectory(int parent, const char* name) {
    if (::mkdirat(parent, name, 0700) != 0 && errno != EEXIST) fail(UiCode::cannot_create_generated_file_directory);
    const int fd = ::openat(parent, name, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) fail(UiCode::cannot_open_generated_file_directory);
    return fd;
}
void write(int directory, const std::string& name, std::string_view text) {
    Fd file{::openat(directory, name.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600)};
    if (file.fd < 0) fail(UiCode::cannot_create_generated_file);
    for (std::size_t pos = 0; pos < text.size();) {
        const auto n = ::write(file.fd, text.data() + pos, text.size() - pos);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) fail(UiCode::cannot_write_generated_file);
        pos += static_cast<std::size_t>(n);
    }
    if (::fsync(file.fd) != 0) fail(UiCode::cannot_flush_generated_file);
}
std::string read(int directory, const char* name, std::size_t maximum = std::string{}.max_size()) {
    Fd file{::openat(directory, name, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)};
    struct stat info{};
    if (file.fd < 0 || ::fstat(file.fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 || uint64_t(info.st_size) > maximum)
        fail(UiCode::cannot_read_saved_file);
    std::string out(static_cast<std::size_t>(info.st_size), '\0');
    for (std::size_t pos = 0; pos < out.size();) {
        const auto n = ::read(file.fd, out.data() + pos, out.size() - pos);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) fail(UiCode::cannot_read_saved_file);
        pos += static_cast<std::size_t>(n);
    }
    return out;
}
// A provider may close a pipe on failure. Report EPIPE without terminating the
// app, preserving this thread's signal mask and any pre-existing pending signal.
class PipeSignalGuard {
    sigset_t previous_{}, only_pipe_{};
    bool already_pending_ = false;
public:
    PipeSignalGuard() {
        sigemptyset(&only_pipe_); sigaddset(&only_pipe_, SIGPIPE);
        if (pthread_sigmask(SIG_BLOCK, &only_pipe_, &previous_) != 0)
            fail(UiCode::cannot_prepare_document_write);
        sigset_t pending{};
        already_pending_ = sigpending(&pending) != 0 || sigismember(&pending, SIGPIPE) == 1;
    }
    void broken_pipe() const {
        if (already_pending_) return;
        const timespec nowait{};
        // Android exposes the libc sigtimedwait wrapper only from API 23.
        // Linux/Android kernels already support it; their signal-set ABI is
        // 64 bits even where the older 32-bit Bionic sigset_t is only 32 bits.
        const uint64_t kernel_set = uint64_t{1} << (SIGPIPE - 1);
        while (syscall(SYS_rt_sigtimedwait, &kernel_set, static_cast<siginfo_t*>(nullptr),
                       &nowait, sizeof(kernel_set)) < 0 && errno == EINTR) {}
    }
    ~PipeSignalGuard() {
        // Change only SIGPIPE back. Older 32-bit Bionic sigset_t cannot
        // represent high real-time signals, so never replace the entire mask.
        if (sigismember(&previous_, SIGPIPE) == 0)
            pthread_sigmask(SIG_UNBLOCK, &only_pipe_, nullptr);
    }
};
std::string random_id() {
    Fd random{::open("/dev/urandom", O_RDONLY | O_CLOEXEC)};
    if (random.fd < 0) fail(UiCode::cannot_generate_file_identifier);
    std::array<unsigned char, 16> bytes{};
    for (std::size_t pos = 0; pos < bytes.size();) {
        const auto n = ::read(random.fd, bytes.data() + pos, bytes.size() - pos);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) fail(UiCode::cannot_generate_file_identifier);
        pos += static_cast<std::size_t>(n);
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string id;
    for (auto byte : bytes) { id += hex[byte >> 4]; id += hex[byte & 15]; }
    return id;
}
void remove_directory(int parent, const std::string& name) noexcept {
    Fd fd{::openat(parent, name.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (fd.fd < 0) return;
    DIR* directory = ::fdopendir(::dup(fd.fd));
    if (!directory) return;
    while (const auto* entry = ::readdir(directory)) {
        if (std::string_view(entry->d_name) == "." || std::string_view(entry->d_name) == "..") continue;
        ::unlinkat(fd.fd, entry->d_name, 0); // Never follows links or removes other directories.
    }
    ::closedir(directory);
    ::unlinkat(parent, name.c_str(), AT_REMOVEDIR);
}
}

Json file_tool_definition() {
    const Json file{{"type", "object"}, {"properties", {
        {"name", {{"type", "string"}, {"description", "Flat filename using ASCII letters, digits, dot, underscore or dash; extensions html/htm/js/mjs/css/json/csv/tsv/txt/md. Names starting jg- are reserved."}}},
        {"content", {{"type", "string"}, {"description", "UTF-8 text. For HTML give body markup, including style/script tags as needed; the app supplies the document head and Juggluco helper."}}}}},
        {"required", Json::array({"name", "content"})}, {"additionalProperties", false}};
    return {{"type", "function"}, {"name", "juggluco_save_files"}, {"strict", true},
        {"description", "Prepare new local files for the answer, saved after successful completion. No imposed file-count, content-size or bundle-count cap. Existing files are never overwritten. The user can tap Save as to choose a destination in Android's document picker, without enabling the web server. This tool cannot choose a destination or claim that external export succeeded. HTML can run ordinary JavaScript/Canvas and query the same-origin Juggluco web server with the supplied helper; external scripts/network are blocked. Use juggluco_web_context for endpoint details. A browser download page is also included."},
        {"parameters", {{"type", "object"}, {"properties", {
            {"title", {{"type", "string"}}},
            {"files", {{"type", "array"}, {"minItems", 1}, {"items", file}}},
            {"entrypoint", {{"type", Json::array({"string", "null"})}, {"description", "Name of the HTML file to open, or null for the download listing."}}},
            {"open_in_browser", {{"type", "boolean"}, {"description", "True only if the user requested opening/running the generated page."}}}}},
            {"required", Json::array({"title", "files", "entrypoint", "open_in_browser"})}, {"additionalProperties", false}}}};
}
Json prepare_files(const Json& args) {
    check(args.is_object() && args.size() == 4 && args.contains("title") && args.contains("files") && args.contains("entrypoint") && args.contains("open_in_browser"));
    check(args["title"].is_string() && !args["title"].get_ref<const std::string&>().empty() && args["open_in_browser"].is_boolean());
    const auto& title = args["title"].get_ref<const std::string&>();
    check(std::none_of(title.begin(), title.end(), [](unsigned char c) { return c < 32 || c == 127; }));
    check(args["files"].is_array() && !args["files"].empty());
    check(args["entrypoint"].is_null() || args["entrypoint"].is_string());
    std::set<std::string> names;
    std::size_t bytes = 0;
    Json metadata = Json::array();
    for (const auto& file : args["files"]) {
        check(file.is_object() && file.size() == 2 && file.contains("name") && file.contains("content") && file["name"].is_string() && file["content"].is_string());
        const auto name = file["name"].get<std::string>();
        const auto& content = file["content"].get_ref<const std::string&>();
        check(filename(name) && names.insert(name).second && content.find('\0') == std::string::npos);
        bytes += content.size(); metadata.push_back({{"name", name}, {"bytes", content.size()}});
    }
    if (args["entrypoint"].is_string()) {
        const auto entry = args["entrypoint"].get<std::string>(); check(html(entry) && names.count(entry));
    }
    return {{"status", "ok"}, {"title", args["title"]}, {"files", metadata}, {"bytes", bytes}, {"bundle", args}};
}

FileStore::FileStore(const std::string& files_directory) {
    if (files_directory.empty() || files_directory.front() != '/') fail(UiCode::invalid_app_files_directory);
    Fd base{::open(files_directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (base.fd < 0) fail(UiCode::cannot_open_app_files_directory);
    Fd additions{subdirectory(base.fd, "additions")};
    fd_ = subdirectory(additions.fd, "chatgpt");
}
FileStore::~FileStore() { if (fd_ >= 0) ::close(fd_); }
Json FileStore::save(const Json& bundles, const std::atomic_bool& cancel) {
    std::lock_guard lock(mutex_);
    check(bundles.is_array());
    Json saved = Json::array();
    std::string temporary;
    try {
        for (const auto& bundle : bundles) {
            if (cancel) fail(UiCode::request_cancelled);
            const auto checked = prepare_files(bundle);
            const auto id = random_id(); temporary = ".pending-" + id;
            if (::mkdirat(fd_, temporary.c_str(), 0700) != 0) fail(UiCode::cannot_create_new_file_bundle);
            Fd directory{::openat(fd_, temporary.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
            if (directory.fd < 0) fail(UiCode::cannot_open_new_file_bundle);
            const auto title = bundle["title"].get<std::string>();
            const auto entry = bundle["entrypoint"].is_null() ? "jg-files.html" : bundle["entrypoint"].get<std::string>();
            std::string listing = "<h1>" + escape(title) + "</h1><p>Saved by Juggluco. Use Download to save a copy.</p><ul>";
            for (const auto& file : bundle["files"]) {
                if (cancel) fail(UiCode::request_cancelled);
                const auto name = file["name"].get<std::string>();
                const auto& content = file["content"].get_ref<const std::string&>();
                write(directory.fd, name, html(name) ? page(title, content) : content);
                listing += "<li><a href=\"" + name + "\">" + name + "</a> &nbsp; <a href=\"" + name + "\" download=\"" + name + "\">Download</a></li>";
            }
            listing += "</ul><p><a href=\"jg-api.js\" download=\"jg-api.js\">Download the Juggluco JavaScript helper</a></p>";
            write(directory.fd, "jg-api.js", api_js);
            write(directory.fd, "jg-files.html", page(title, listing));
            Json metadata{{"id", id}, {"title", title}, {"entrypoint", entry}, {"open_in_browser", bundle["open_in_browser"]},
                {"files", checked["files"]}, {"created_at", std::time(nullptr)}};
            write(directory.fd, "jg-manifest.json", metadata.dump());
            ::fsync(directory.fd);
            if (cancel) fail(UiCode::request_cancelled);
            // IDs are unpredictable and each write is a new directory, never
            // a model-chosen path or an overwrite of a prior file.
            struct stat exists{};
            if (::fstatat(fd_, id.c_str(), &exists, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT)
                fail(UiCode::generated_file_identifier_collision);
            saved.push_back(std::move(metadata));
            if (::renameat(fd_, temporary.c_str(), fd_, id.c_str()) != 0) fail(UiCode::cannot_commit_generated_files);
            temporary.clear();
        }
        ::fsync(fd_);
        return saved;
    } catch (...) {
        if (!temporary.empty()) remove_directory(fd_, temporary);
        rollback(saved); throw;
    }
}
void FileStore::rollback(const Json& saved) noexcept {
    std::lock_guard lock(mutex_);
    try { for (const auto& item : saved) {
        const auto id = item.at("id").get<std::string>();
        if (bundle_id(id)) remove_directory(fd_, id);
    } } catch (...) {}
}
Json FileStore::list() const {
    std::lock_guard lock(mutex_);
    // A separate open description is needed: dup() would share readdir's
    // directory offset and make later listings appear empty.
    const int copy = ::openat(fd_, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    DIR* directory = copy < 0 ? nullptr : ::fdopendir(copy);
    if (!directory) { if (copy >= 0) ::close(copy); fail(UiCode::cannot_list_saved_files); }
    Json result = Json::array();
    while (const auto* entry = ::readdir(directory)) {
        if (!bundle_id(entry->d_name)) continue;
        try {
            Fd file{::openat(fd_, entry->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
            auto metadata = Json::parse(read(file.fd, "jg-manifest.json"));
            if (metadata.is_object() && metadata.value("id", "") == entry->d_name && metadata.contains("title") && metadata["title"].is_string() &&
                metadata.contains("files") && metadata["files"].is_array() &&
                metadata.contains("entrypoint") && metadata["entrypoint"].is_string() && filename(metadata["entrypoint"].get<std::string>(), true))
                result.push_back(std::move(metadata));
        } catch (...) { /* An unrelated/corrupt entry does not expose its bytes. */ }
    }
    ::closedir(directory);
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.value("created_at", 0LL) > b.value("created_at", 0LL); });
    return result;
}
std::string FileStore::relative_path(const std::string& id, const std::string& file) const {
    std::lock_guard lock(mutex_);
    if (!bundle_id(id) || !filename(file, true)) fail(UiCode::invalid_saved_file);
    Fd directory{::openat(fd_, id.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
    Fd content{directory.fd < 0 ? -1 : ::openat(directory.fd, file.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC)};
    struct stat info{};
    if (content.fd < 0 || ::fstat(content.fd, &info) || !S_ISREG(info.st_mode)) fail(UiCode::the_saved_file_is_no_longer_available);
    return "additions/chatgpt/" + id + "/" + file;
}
std::string FileStore::read_text(const std::string& id, const std::string& file) const {
        std::lock_guard lock(mutex_);
        if (!bundle_id(id) || !filename(file, true)) fail(UiCode::invalid_saved_file);
        Fd directory{::openat(fd_, id.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC)};
        const auto manifest = Json::parse(read(directory.fd, "jg-manifest.json"));
        if (!manifest.is_object() || manifest.value("id", "") != id ||
            !manifest.contains("files") || !manifest["files"].is_array())
            fail(UiCode::invalid_saved_file_manifest);
        bool listed = file == "jg-api.js";
        for (const auto& item : manifest["files"])
            if (item.is_object() && item.contains("name") && item["name"] == file) listed = true;
        if (!listed) fail(UiCode::file_is_not_part_of_this_generated_bundle);
        // Snapshot before writing and release the lock before slow provider I/O.
        return read(directory.fd, file.c_str());
}
std::size_t FileStore::copy_to_fd(const std::string& id, const std::string& file, int output) const {
    const auto bytes = read_text(id, file);
    const int flags = fcntl(output, F_GETFL);
    if (flags < 0 || (flags & O_ACCMODE) == O_RDONLY) fail(UiCode::document_is_not_writable);
    PipeSignalGuard signals;
    for (std::size_t pos = 0; pos < bytes.size();) {
        const auto count = ::write(output, bytes.data() + pos, bytes.size() - pos);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0 && errno == EPIPE) signals.broken_pipe();
        if (count <= 0) fail(UiCode::could_not_write_the_selected_document);
        pos += static_cast<std::size_t>(count);
    }
    struct stat info{};
    if (fstat(output, &info) != 0) fail(UiCode::could_not_check_the_selected_document);
    if (S_ISREG(info.st_mode)) {
        int flushed;
        do { flushed = fsync(output); } while (flushed < 0 && errno == EINTR);
        if (flushed < 0) fail(UiCode::could_not_finish_writing_the_selected_document);
    }
    return bytes.size();
}
}
