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
#include "jgchat/workspace.hpp"
#include "sqlite/sqlite3.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <set>
#include <stdexcept>

namespace jgchat {
namespace {
constexpr std::size_t max_rows = 100000, max_columns = 64, max_cells = 500000;
std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
void columns_ok(const std::vector<std::string>& columns) {
    if (columns.size() > max_columns) throw jgchat::UiError(jgchat::UiCode::analysis_table_exceeds_64_columns);
    std::set<std::string> seen;
    for (const auto& c : columns)
        if (c.empty() || c.size() > 128 || c.find('\0') != std::string::npos || !seen.insert(lower(c)).second)
            throw jgchat::UiError(jgchat::UiCode::analysis_columns_must_have_unique_nonempty_names_case_insensitive);
}
std::string quoted(const std::string& s) {
    std::string out = "\"";
    for (char c : s) { out += c; if (c == '"') out += c; }
    return out + '"';
}
Json delimited_cell(const std::string& s) {
    const auto v = lower(s);
    if (s.empty() || v == "nan" || v == "+nan" || v == "-nan" ||
        v == "inf" || v == "+inf" || v == "-inf" || v == "infinity") return nullptr;
    return s; // No automatic conversion of IDs, labels, timestamps or numbers.
}
struct Database {
    sqlite3* db = nullptr;
    Database() {
        if (sqlite3_open_v2(":memory:", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr) != SQLITE_OK) {
            if (db) sqlite3_close(db);
            throw jgchat::UiError(jgchat::UiCode::cannot_initialize_local_analysis_database);
        }
    }
    ~Database() { sqlite3_close(db); }
};
struct Statement {
    sqlite3_stmt* stmt = nullptr;
    ~Statement() { sqlite3_finalize(stmt); }
};
void check_sql(sqlite3* db, int code) {
    if (code != SQLITE_OK) throw UiError(UiMessage::detail(UiCode::analysis_sql, sqlite3_errmsg(db)));
}
struct Budget {
    const std::atomic_bool& cancel;
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    unsigned ticks = 0;
    bool exhausted = false;
    static int check(void* ptr) {
        auto& b = *static_cast<Budget*>(ptr);
        b.exhausted = b.cancel.load() || ++b.ticks > 20000 || std::chrono::steady_clock::now() >= b.end;
        return b.exhausted ? 1 : 0;
    }
};
int authorize(void* ptr, int action, const char* a, const char* b, const char* database, const char*) {
    const auto& names = *static_cast<const std::set<std::string>*>(ptr);
    switch (action) {
    case SQLITE_SELECT: case SQLITE_RECURSIVE: return SQLITE_OK;
    case SQLITE_READ:
        return a && database && std::string_view(database) == "main" && names.count(lower(a)) ? SQLITE_OK : SQLITE_DENY;
    case SQLITE_FUNCTION: {
        static const std::set<std::string> functions{
            "abs","avg","coalesce","count","ifnull","nullif","min","max","sum","total","round",
            "median","percentile","percentile_cont","percentile_disc",
            "sqrt","pow","power","exp","ln","log","log10","log2","floor","ceil","ceiling","trunc","mod",
            "sin","cos","tan","asin","acos","atan","atan2","pi","degrees","radians",
            "lower","upper","length","substr","substring","trim","ltrim","rtrim","replace","instr",
            "like","glob","concat","concat_ws","group_concat","string_agg","typeof","unicode","char",
            "json","json_array","json_object","json_extract","json_type","json_array_length","json_valid",
            "json_group_array","json_group_object","->","->>",
            "date","time","datetime","julianday","unixepoch","strftime","timediff",
            "row_number","rank","dense_rank","percent_rank","cume_dist","ntile","lag","lead",
            "first_value","last_value","nth_value","iif","if"
        };
        return b && functions.count(lower(b)) ? SQLITE_OK : SQLITE_DENY;
    }
    default: return SQLITE_DENY;
    }
}
} // namespace

AnalysisTable analysis_table(const Json& value, std::string_view format) {
    AnalysisTable out;
    if (format == "json") {
        const Json rows = value.is_object() ? Json::array({value}) : value;
        if (!rows.is_array() || rows.size() > max_rows) throw jgchat::UiError(jgchat::UiCode::json_table_must_be_an_array_with_at_most_100000_rows);
        std::set<std::string> columns;
        for (const auto& row : rows) {
            if (row.is_object()) for (auto it = row.begin(); it != row.end(); ++it) columns.insert(it.key());
            else columns.insert("value");
            if (columns.size() > max_columns) throw jgchat::UiError(jgchat::UiCode::analysis_table_exceeds_64_columns);
        }
        out.columns.assign(columns.begin(), columns.end());
        columns_ok(out.columns);
        if (out.columns.size() * rows.size() > max_cells) throw jgchat::UiError(jgchat::UiCode::analysis_table_exceeds_500000_cells);
        for (const auto& row : rows) {
            Json cells = Json::array();
            for (const auto& c : out.columns) {
                if (row.is_object()) cells.push_back(row.value(c, Json(nullptr)));
                else cells.push_back(c == "value" ? row : Json(nullptr));
            }
            out.rows.push_back(std::move(cells));
        }
        return out;
    }
    if ((format != "csv" && format != "tsv") || !value.is_string())
        throw jgchat::UiError(jgchat::UiCode::table_format_must_be_json_csv_or_tsv_and_match_its_source);
    const auto& text = value.get_ref<const std::string&>();
    const char separator = format == "csv" ? ',' : '\t';
    bool in_quote = false, closed_quote = false, field_start = true;
    std::vector<std::string> row;
    std::string cell;
    const auto finish_row = [&] {
        row.push_back(std::move(cell)); cell.clear();
        if (out.columns.empty()) { out.columns = row; columns_ok(out.columns); }
        else {
            if (row.size() != out.columns.size()) throw jgchat::UiError(jgchat::UiCode::delimited_table_has_inconsistent_column_counts);
            if (out.rows.size() >= max_rows || (out.rows.size()+1) * out.columns.size() > max_cells)
                throw jgchat::UiError(jgchat::UiCode::analysis_table_exceeds_100000_rows_500000_cells);
            Json cells = Json::array();
            for (const auto& c : row) cells.push_back(delimited_cell(c));
            out.rows.push_back(std::move(cells));
        }
        row.clear(); field_start = true; closed_quote = false;
    };
    std::size_t pos = text.starts_with("\xef\xbb\xbf") ? 3 : 0;
    for (; pos < text.size(); ++pos) {
        const char c = text[pos];
        if (in_quote) {
            if (c == '"') {
                if (pos + 1 < text.size() && text[pos + 1] == '"') { cell += '"'; ++pos; }
                else { in_quote = false; closed_quote = true; }
            } else cell += c;
        } else if (c == separator) {
            row.push_back(std::move(cell)); cell.clear(); field_start = true; closed_quote = false;
            if (row.size() >= max_columns) throw jgchat::UiError(jgchat::UiCode::analysis_table_exceeds_64_columns);
        } else if (c == '\r' || c == '\n') {
            if (c == '\r' && pos + 1 < text.size() && text[pos + 1] == '\n') ++pos;
            finish_row();
        } else if (c == '"' && field_start) { in_quote = true; field_start = false; }
        else {
            if (closed_quote || c == '"') throw jgchat::UiError(jgchat::UiCode::invalid_delimited_file_quoting);
            cell += c; field_start = false;
        }
    }
    if (in_quote) throw jgchat::UiError(jgchat::UiCode::unclosed_delimited_file_quote);
    if (!cell.empty() || !row.empty() || !field_start || closed_quote) finish_row();
    if (out.columns.empty()) throw jgchat::UiError(jgchat::UiCode::delimited_table_is_missing_its_header);
    return out;
}

Json analysis_tables(const Json& value) {
    Json found = Json::array();
    const auto walk = [&](auto&& self, const Json& v, const std::string& path, unsigned depth) -> void {
        if (depth > 6 || found.size() >= 24) return;
        if (v.is_array()) {
            try {
                const auto table = analysis_table(v, "json");
                found.push_back({{"path", path}, {"format", "json"}, {"rows", table.rows.size()}, {"columns", table.columns}});
            } catch (...) {} // Deep/wide arrays remain available through memory_read.
        } else if (v.is_object()) {
            const auto format = v.value("format", Json()).is_string() ? v["format"].get<std::string>() : "";
            if ((format == "tsv" || format == "csv") && v.contains("data")) {
                try {
                    const auto table = analysis_table(v["data"], format);
                    found.push_back({{"path", path + "/data"}, {"format", format}, {"rows", table.rows.size()}, {"columns", table.columns}});
                } catch (...) {}
            }
            for (auto i = v.begin(); i != v.end(); ++i) {
                std::string key;
                for (char c : i.key()) { if (c == '~') key += "~0"; else if (c == '/') key += "~1"; else key += c; }
                self(self, i.value(), path + "/" + key, depth + 1);
            }
        }
    };
    walk(walk, value, "", 0);
    return found;
}

Json run_analysis_query(const std::string& sql,
    const std::vector<std::pair<std::string, AnalysisTable>>& tables, const std::atomic_bool& cancel) {
    if (sql.empty() || sql.size() > 16384 || sql.find('\0') != std::string::npos || tables.size() > 16)
        throw jgchat::UiError(jgchat::UiCode::analysis_needs_one_sql_select_at_most_16_kib_and_at_most_16_tables);
    if (cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
    // This statically linked, hidden SQLite instance is used only here. It
    // never opens app databases or files, registers extensions, or uses JNI.
    sqlite3_hard_heap_limit64(64 * 1024 * 1024);
    Database database;
    auto* db = database.db;
    sqlite3_db_config(db, SQLITE_DBCONFIG_DEFENSIVE, 1, nullptr);
    sqlite3_db_config(db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, nullptr);
    sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 1024 * 1024);
    sqlite3_limit(db, SQLITE_LIMIT_SQL_LENGTH, 16384);
    sqlite3_limit(db, SQLITE_LIMIT_COLUMN, 64);
    sqlite3_limit(db, SQLITE_LIMIT_EXPR_DEPTH, 50);
    sqlite3_limit(db, SQLITE_LIMIT_COMPOUND_SELECT, 16);
    sqlite3_limit(db, SQLITE_LIMIT_VDBE_OP, 100000);
    sqlite3_limit(db, SQLITE_LIMIT_FUNCTION_ARG, 32);
    sqlite3_limit(db, SQLITE_LIMIT_ATTACHED, 0);
    sqlite3_limit(db, SQLITE_LIMIT_LIKE_PATTERN_LENGTH, 256);
    sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, 64);
    Budget budget{cancel};
    sqlite3_progress_handler(db, 1000, Budget::check, &budget);
    std::set<std::string> names;
    std::size_t input_rows = 0, input_cells = 0;
    for (const auto& [name, table] : tables) {
        if (name.empty() || name.size() > 32 || name.starts_with("sqlite_") ||
            !std::isalpha(static_cast<unsigned char>(name[0])) ||
            !std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; }) ||
            !names.insert(lower(name)).second) throw jgchat::UiError(jgchat::UiCode::invalid_or_duplicate_analysis_table_alias);
        columns_ok(table.columns);
        if (table.columns.empty()) throw jgchat::UiError(jgchat::UiCode::empty_source_has_no_columns_inspect_its_metadata_first);
        input_rows += table.rows.size();
        input_cells += table.rows.size() * table.columns.size();
        if (input_rows > max_rows || input_cells > max_cells)
            throw jgchat::UiError(jgchat::UiCode::analysis_input_exceeds_100000_rows_500000_cells_aggregate_smaller_windows_first);
        std::string create = "CREATE TABLE " + quoted(name) + " (", insert = "INSERT INTO " + quoted(name) + " VALUES (";
        for (std::size_t i = 0; i < table.columns.size(); ++i) {
            if (i) { create += ','; insert += ','; }
            create += quoted(table.columns[i]); insert += '?';
        }
        create += ')'; insert += ')';
        check_sql(db, sqlite3_exec(db, create.c_str(), nullptr, nullptr, nullptr));
        Statement statement;
        check_sql(db, sqlite3_prepare_v2(db, insert.c_str(), -1, &statement.stmt, nullptr));
        for (const auto& row : table.rows) {
            if (cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
            if (row.size() != table.columns.size()) throw jgchat::UiError(jgchat::UiCode::invalid_analysis_row_width);
            for (std::size_t i = 0; i < row.size(); ++i) {
                const int at = static_cast<int>(i + 1);
                const auto& v = row[i];
                int code;
                if (v.is_null()) code = sqlite3_bind_null(statement.stmt, at);
                else if (v.is_boolean()) code = sqlite3_bind_int(statement.stmt, at, v.get<bool>());
                else if (v.is_number_unsigned() && v.get<uint64_t>() > INT64_MAX) throw jgchat::UiError(jgchat::UiCode::unsigned_value_exceeds_sql_integer_range);
                else if (v.is_number_integer()) code = sqlite3_bind_int64(statement.stmt, at, v.get<int64_t>());
                else if (v.is_number()) code = sqlite3_bind_double(statement.stmt, at, v.get<double>());
                else {
                    const auto text = v.is_string() ? v.get<std::string>() : v.dump();
                    code = sqlite3_bind_text(statement.stmt, at, text.data(), static_cast<int>(text.size()), SQLITE_TRANSIENT);
                }
                check_sql(db, code);
            }
            if (sqlite3_step(statement.stmt) != SQLITE_DONE) check_sql(db, sqlite3_errcode(db));
            check_sql(db, sqlite3_reset(statement.stmt));
        }
    }
    sqlite3_set_authorizer(db, authorize, &names);
    Statement statement;
    const char* tail = nullptr;
    check_sql(db, sqlite3_prepare_v3(db, sql.data(), static_cast<int>(sql.size()), 0, &statement.stmt, &tail));
    while (tail && *tail && std::isspace(static_cast<unsigned char>(*tail))) ++tail;
    if (!statement.stmt || (tail && *tail) || !sqlite3_stmt_readonly(statement.stmt) ||
        sqlite3_bind_parameter_count(statement.stmt) || sqlite3_column_count(statement.stmt) == 0)
        throw jgchat::UiError(jgchat::UiCode::only_one_read_only_select_without_parameters_is_allowed);
    std::vector<std::string> columns;
    for (int i = 0; i < sqlite3_column_count(statement.stmt); ++i) columns.emplace_back(sqlite3_column_name(statement.stmt, i));
    columns_ok(columns); // Refuse ambiguous duplicate result keys.
    Json rows = Json::array();
    std::size_t bytes = 0;
    for (;;) {
        const auto code = sqlite3_step(statement.stmt);
        if (cancel) throw jgchat::UiError(jgchat::UiCode::request_cancelled);
        if (budget.exhausted) throw jgchat::UiError(jgchat::UiCode::analysis_reached_its_five_second_20_million_step_budget_narrow_or_aggregate_the_input);
        if (code == SQLITE_DONE) break;
        if (code != SQLITE_ROW) check_sql(db, code);
        Json row = Json::object();
        for (std::size_t i = 0; i < columns.size(); ++i) {
            const int at = static_cast<int>(i);
            switch (sqlite3_column_type(statement.stmt, at)) {
            case SQLITE_NULL: row[columns[i]] = nullptr; break;
            case SQLITE_INTEGER: row[columns[i]] = sqlite3_column_int64(statement.stmt, at); break;
            case SQLITE_FLOAT: {
                const double value = sqlite3_column_double(statement.stmt, at);
                if (!std::isfinite(value)) throw jgchat::UiError(jgchat::UiCode::analysis_produced_a_nonfinite_numeric_result);
                row[columns[i]] = value; break;
            }
            case SQLITE_TEXT: row[columns[i]] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement.stmt, at)), sqlite3_column_bytes(statement.stmt, at)); break;
            default: throw jgchat::UiError(jgchat::UiCode::binary_sql_results_are_not_supported);
            }
        }
        bytes += row.dump().size();
        if (rows.size() >= 20000 || bytes > 2 * 1024 * 1024)
            throw jgchat::UiError(jgchat::UiCode::analysis_output_exceeds_20000_rows_2_mib_use_aggregation_or_an_explicit_limit_no_part);
        rows.push_back(std::move(row));
    }
    return {{"status", "ok"}, {"engine", "SQLite " SQLITE_VERSION}, {"columns", columns},
        {"rows", std::move(rows)}, {"input_rows", input_rows}, {"execution_complete", true},
        {"coverage", "Complete execution of this SQL over the supplied snapshots only; input truncation, missing readings, SQL filters and LIMIT still apply."}};
}
} // namespace jgchat
