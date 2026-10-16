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
#include "files.hpp"
#include <optional>
#include <vector>

namespace jgchat {
Json workspace_tool_definitions();
bool is_workspace_tool(std::string_view);

// Immutable result bodies in private no-backup storage. The caller saves
// snapshot() together with the conversation, then calls commit(). A failed
// turn restores the index and deletes its uncommitted bodies via rollback().
// One worker owns this object; it is never accessed by the UI polling thread.
class Workspace {
public:
    Workspace(const std::string& private_directory, std::string account_id, FileStore* files = nullptr);
    ~Workspace();
    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;
    void restore(const Json&);
    Json snapshot() const;
    void begin();
    void commit() noexcept;
    void rollback() noexcept;
    void clear(); // Index only, until the containing conversation is committed.
    void remember_files(const Json&);
    Json capture(std::string_view tool, const Json& args, const Json& result);
    Json execute(std::string_view, const Json&, const std::atomic_bool&);
    // Import legacy results/turns once. Compact only tool result bodies, leaving
    // function calls paired and opaque reasoning/output items untouched.
    void import_history(Json&);
    void remember_turn(const Json&);
    void compact(Json&, bool keep_latest_results) const;
    std::string context() const;
private:
    int fd_ = -1;
    std::string account_;
    FileStore* files_;
    Json index_, created_ = Json::array();
    std::optional<Json> checkpoint_;
    Json put(Json metadata, const Json& data);
    Json body(const std::string& id) const;
    const Json& metadata(const std::string& id) const;
    Json describe(const Json&, const Json&) const;
    Json read(const Json&) const;
    Json search(const Json&) const;
    Json note(const Json&);
    Json query(const Json&, const std::atomic_bool&);
    Json numerical(std::string_view, const Json&, const std::atomic_bool&);
    void prune() noexcept;
};

// Tables retain JSON types. Delimited input stays text except empty/nonfinite
// numeric tokens, which become NULL. SQL must explicitly CAST numeric columns.
struct AnalysisTable { std::vector<std::string> columns; Json rows = Json::array(); };
AnalysisTable analysis_table(const Json& value, std::string_view format);
Json analysis_tables(const Json& value);
Json run_analysis_query(const std::string& sql,
    const std::vector<std::pair<std::string, AnalysisTable>>& tables, const std::atomic_bool& cancel);
}
