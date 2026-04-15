#pragma once
#include <sqlite3.h>
#include <string_view>
#include "vosfs/common/error.hpp"

namespace vosfs::auth::detail {
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


private:
    sqlite3* db_;
};
} // namespace vosfs::auth::detail