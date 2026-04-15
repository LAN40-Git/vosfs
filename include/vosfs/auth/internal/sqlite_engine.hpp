#pragma once
#include <sqlite3.h>
#include <string_view>
#include <unordered_map>
#include <kosio/common/debug.hpp>
#include "vosfs/common/error.hpp"

namespace vosfs::auth::detail {
// 单行多列结果：key=列名，value=列值
using SingleRowResult = std::unordered_map<std::string, std::string>;

class SQLiteEngine {
private:
    explicit SQLiteEngine(sqlite3* db)
        : db_(db) {}

public:
    ~SQLiteEngine() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    SQLiteEngine(const SQLiteEngine&) = delete;
    auto operator=(const SQLiteEngine&) -> SQLiteEngine& = delete;

    SQLiteEngine(SQLiteEngine&& other) noexcept
        : db_(other.db_) {}
    auto operator=(SQLiteEngine&& other) noexcept -> SQLiteEngine& {
        if (this == &other) {
            return *this;
        }

        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        other.db_ = nullptr;
        return *this;
    }

public:
    [[nodiscard]]
    static auto create(std::string_view db_path) -> Result<SQLiteEngine> {
        sqlite3* db = nullptr;

        int rc = sqlite3_open(db_path.data(), &db);
        if (rc != SQLITE_OK) {
            std::string err = sqlite3_errmsg(db);
            sqlite3_close(db);
            return std::unexpected{make_error(Error::kCreateSQLiteEngineFailed)};
        }

        return SQLiteEngine{db};
    }

public:
    [[nodiscard]]
    auto execute(std::string_view sql) const -> Result<void> {
        char* err_msg = nullptr;
        int rc = sqlite3_exec(db_, sql.data(), nullptr, nullptr, &err_msg);

        if (rc != SQLITE_OK) {
            std::string err = err_msg ? err_msg : "unknown error";
            LOG_ERROR("sql error: {}", err_msg);
            sqlite3_free(err_msg);
            return std::unexpected{make_error(Error::kSQLError)};
        }

        return Result<void>{};
    }

    [[nodiscard]]
    auto query_single_row(std::string_view sql) const -> Result<SingleRowResult> {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            LOG_ERROR("sql error: {}", sqlite3_errmsg(db_));
            return std::unexpected{make_error(Error::kSQLError)};
        }

        SingleRowResult result;

        // 执行一步，获取第一行
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            int col_count = sqlite3_column_count(stmt);

            for (int i = 0; i < col_count; ++i) {
                const char* col_name = reinterpret_cast<const char*>(sqlite3_column_name(stmt, i));
                const char* col_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));

                result[col_name] = col_val ? col_val : "";
            }
        }

        sqlite3_finalize(stmt);

        if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
            LOG_ERROR("sql error: {}", sqlite3_errmsg(db_));
            return std::unexpected{make_error(Error::kSQLError)};
        }

        return result;
    }

    [[nodiscard]]
    auto exists(std::string_view sql) const -> Result<bool> {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.data(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            LOG_ERROR("sql error: {}", sqlite3_errmsg(db_));
            return std::unexpected{make_error(Error::kSQLError)};
        }

        bool found = false;
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            found = true;
        } else if (rc != SQLITE_DONE) {
            LOG_ERROR("sql step error: {}", sqlite3_errmsg(db_));
            sqlite3_finalize(stmt);
            return std::unexpected{make_error(Error::kSQLError)};
        }

        sqlite3_finalize(stmt);
        return found;
    }

private:
    sqlite3* db_;
};
} // namespace vosfs::auth::detail