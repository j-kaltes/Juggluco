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
/*      Thu Oct 15 12:54:42 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "tools.hpp"
#include <mutex>

namespace jgchat {
Json file_tool_definition();
Json prepare_files(const Json&);

// Generated files live below filesDir/additions/chatgpt, separate from account
// credentials and databases. Every answer creates new, immutable bundle IDs.
class FileStore {
public:
    explicit FileStore(const std::string& files_directory);
    ~FileStore();
    FileStore(const FileStore&) = delete;
    FileStore& operator=(const FileStore&) = delete;
    Json save(const Json& bundles, const std::atomic_bool& cancel);
    void rollback(const Json& saved) noexcept;
    Json list() const;
    std::string read_text(const std::string& id, const std::string& file) const;
    std::string relative_path(const std::string& id, const std::string& file) const;
    // Copy to a caller-owned, opened SAF descriptor. No seek/truncate/close;
    // document providers may supply pipes rather than regular files.
    std::size_t copy_to_fd(const std::string& id, const std::string& file, int output) const;
private:
    int fd_ = -1;
    mutable std::recursive_mutex mutex_;
};
}
