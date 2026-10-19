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
#include "jgchat/storage.hpp"
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

namespace jgchat {
namespace {
void check_name(std::string_view name) {
    if (name != "tokens.json" && name != "conversation.json")
        throw jgchat::UiError(jgchat::UiCode::invalid_private_storage_name);
}
struct File {
    int fd;
    ~File() { if (fd >= 0) ::close(fd); }
};
}
Storage::Storage(const std::string& directory) {
    if (directory.empty() || directory.front() != '/')
        throw jgchat::UiError(jgchat::UiCode::application_storage_directory_must_be_absolute);
    if (::mkdir(directory.c_str(), 0700) != 0 && errno != EEXIST)
        throw jgchat::UiError(jgchat::UiCode::cannot_create_private_chat_storage);
    fd_ = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd_ < 0) throw jgchat::UiError(jgchat::UiCode::cannot_open_private_chat_storage);
    if (::fchmod(fd_, 0700) != 0) {
        ::close(fd_); fd_ = -1;
        throw jgchat::UiError(jgchat::UiCode::cannot_restrict_private_chat_storage_permissions);
    }
}
Storage::~Storage() { if (fd_ >= 0) ::close(fd_); }
std::optional<nlohmann::json> Storage::read(const char* name) const {
    check_name(name);
    File file{::openat(fd_, name, O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)};
    if (file.fd < 0) {
        if (errno == ENOENT) return std::nullopt;
        throw jgchat::UiError(jgchat::UiCode::cannot_open_saved_chat_state);
    }
    struct stat st{};
    if (::fstat(file.fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        static_cast<unsigned long long>(st.st_size) > std::string{}.max_size())
        throw jgchat::UiError(jgchat::UiCode::saved_chat_state_is_not_a_bounded_regular_file);
    if (::fchmod(file.fd, 0600) != 0)
        throw jgchat::UiError(jgchat::UiCode::cannot_restrict_saved_chat_state_permissions);
    std::string text;
    std::array<char, 8192> buffer{};
    for (;;) {
        const auto n = ::read(file.fd, buffer.data(), buffer.size());
        if (n == 0) break;
        if (n < 0) { if (errno == EINTR) continue; throw jgchat::UiError(jgchat::UiCode::cannot_read_saved_chat_state); }
        text.append(buffer.data(), static_cast<std::size_t>(n));
    }
    try { return nlohmann::json::parse(text); }
    catch (...) { throw jgchat::UiError(jgchat::UiCode::saved_chat_state_is_invalid_sign_in_again_or_start_a_new_chat); }
}
void Storage::write(const char* name, const nlohmann::json& value) const {
    check_name(name);
    const std::string data = value.dump();
    const std::string temporary = std::string(name) + ".tmp";
    if (::unlinkat(fd_, temporary.c_str(), 0) != 0 && errno != ENOENT)
        throw jgchat::UiError(jgchat::UiCode::cannot_replace_temporary_chat_state);
    File file{::openat(fd_, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600)};
    if (file.fd < 0) throw jgchat::UiError(jgchat::UiCode::cannot_create_private_chat_state);
    try {
        for (std::size_t pos = 0; pos < data.size();) {
            const auto n = ::write(file.fd, data.data() + pos, data.size() - pos);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) throw jgchat::UiError(jgchat::UiCode::cannot_write_private_chat_state);
            pos += static_cast<std::size_t>(n);
        }
        if (::fsync(file.fd) != 0 || ::renameat(fd_, temporary.c_str(), fd_, name) != 0)
            throw jgchat::UiError(jgchat::UiCode::cannot_save_private_chat_state);
        // Some Android filesystems do not support directory fsync. The file was
        // already flushed and atomically renamed; do not undo that valid state.
        ::fsync(fd_);
    } catch (...) {
        ::unlinkat(fd_, temporary.c_str(), 0);
        throw;
    }
}
void Storage::remove(const char* name) const {
    check_name(name);
    if (::unlinkat(fd_, name, 0) != 0 && errno != ENOENT)
        throw jgchat::UiError(jgchat::UiCode::cannot_remove_saved_chat_state);
    const std::string temporary = std::string(name) + ".tmp";
    if (::unlinkat(fd_, temporary.c_str(), 0) != 0 && errno != ENOENT)
        throw jgchat::UiError(jgchat::UiCode::cannot_remove_temporary_chat_state);
    ::fsync(fd_);
}
}
