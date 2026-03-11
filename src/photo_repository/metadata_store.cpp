#include "photo_repository/metadata_store.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

void throwSqliteError(sqlite3* db, const std::string& context) {
    throw std::runtime_error(context + ": " + sqlite3_errmsg(db));
}

class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throwSqliteError(db_, "Failed to prepare SQL statement");
        }
    }

    ~Statement() {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const noexcept {
        return stmt_;
    }

private:
    sqlite3* db_{};
    sqlite3_stmt* stmt_{};
};

}

MetadataStore::MetadataStore(std::filesystem::path databasePath)
    : databasePath_(std::move(databasePath)) {
    if (sqlite3_open(databasePath_.string().c_str(), &db_) != SQLITE_OK) {
        const std::string error = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Failed to open SQLite database: " + error);
    }

    if (sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        throwSqliteError(db_, "Failed to enable SQLite WAL mode");
    }

    if (sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        throwSqliteError(db_, "Failed to set SQLite busy_timeout");
    }
}

MetadataStore::~MetadataStore() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

void MetadataStore::ensureSchema() {
    constexpr const char* kCreatePhotosSql =
        "CREATE TABLE IF NOT EXISTS photos ("
        "id INTEGER PRIMARY KEY,"
        "description TEXT NOT NULL,"
        "file_path TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "repo_type TEXT NOT NULL DEFAULT 'shared',"
        "owner_id TEXT NULL"
        ");";

    if (sqlite3_exec(db_, kCreatePhotosSql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throwSqliteError(db_, "Failed to create SQLite schema");
    }

    if (!hasColumn("photos", "repo_type")) {
        if (sqlite3_exec(
                db_,
                "ALTER TABLE photos ADD COLUMN repo_type TEXT NOT NULL DEFAULT 'shared';",
                nullptr,
                nullptr,
                nullptr) != SQLITE_OK) {
            throwSqliteError(db_, "Failed to add repo_type column");
        }
    }

    if (!hasColumn("photos", "owner_id")) {
        if (sqlite3_exec(
                db_,
                "ALTER TABLE photos ADD COLUMN owner_id TEXT NULL;",
                nullptr,
                nullptr,
                nullptr) != SQLITE_OK) {
            throwSqliteError(db_, "Failed to add owner_id column");
        }
    }
}

std::vector<Photo> MetadataStore::loadAll(const std::string& repoType,
                                          const std::optional<std::string>& ownerId) const {
    constexpr const char* kSelectSql =
        "SELECT id, description, file_path, created_at, repo_type, owner_id "
        "FROM photos "
        "WHERE repo_type = ? AND ((? IS NULL AND owner_id IS NULL) OR owner_id = ?) "
        "ORDER BY id ASC;";

    Statement statement(db_, kSelectSql);
    bindRepositoryScope(statement.get(), 1, repoType, ownerId);

    std::vector<Photo> photos;

    while (true) {
        const int rc = sqlite3_step(statement.get());
        if (rc == SQLITE_DONE) {
            break;
        }

        if (rc != SQLITE_ROW) {
            throwSqliteError(db_, "Failed to load metadata from SQLite");
        }

        const auto id = static_cast<PhotoId>(sqlite3_column_int64(statement.get(), 0));
        const auto* descriptionText =
            reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1));
        const auto* filePathText =
            reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2));
        const auto createdAtSeconds = sqlite3_column_int64(statement.get(), 3);
        const auto* repoTypeText =
            reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 4));

        std::optional<std::string> ownerIdValue;
        if (sqlite3_column_type(statement.get(), 5) != SQLITE_NULL) {
            const auto* ownerIdText =
                reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 5));
            if (ownerIdText != nullptr) {
                ownerIdValue = std::string(ownerIdText);
            }
        }

        if (descriptionText == nullptr || filePathText == nullptr || repoTypeText == nullptr) {
            throw std::runtime_error("SQLite returned null text field for non-null column");
        }

        photos.emplace_back(
            id,
            std::string(descriptionText),
            std::filesystem::path(filePathText),
            fromUnixTime(createdAtSeconds),
            std::string(repoTypeText),
            ownerIdValue);
    }

    return photos;
}

PhotoId MetadataStore::loadMaxId() const {
    constexpr const char* kMaxIdSql = "SELECT COALESCE(MAX(id), 0) FROM photos;";
    Statement statement(db_, kMaxIdSql);

    const int rc = sqlite3_step(statement.get());
    if (rc != SQLITE_ROW) {
        throwSqliteError(db_, "Failed to read max id from SQLite");
    }

    return static_cast<PhotoId>(sqlite3_column_int64(statement.get(), 0));
}

void MetadataStore::insertPhoto(const Photo& photo) {
    constexpr const char* kInsertSql =
        "INSERT INTO photos (id, description, file_path, created_at, repo_type, owner_id) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    Statement statement(db_, kInsertSql);

    if (sqlite3_bind_int64(statement.get(), 1, static_cast<sqlite3_int64>(photo.id)) != SQLITE_OK ||
        sqlite3_bind_text(statement.get(), 2, photo.description.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(statement.get(), 3, photo.filePath.string().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_int64(
            statement.get(), 4, static_cast<sqlite3_int64>(toUnixTime(photo.createdAt))) != SQLITE_OK ||
        sqlite3_bind_text(statement.get(), 5, photo.repoType.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throwSqliteError(db_, "Failed to bind SQLite insert parameters");
    }

    const int ownerBindRc = photo.ownerId.has_value()
                                ? sqlite3_bind_text(
                                      statement.get(), 6, photo.ownerId->c_str(), -1, SQLITE_TRANSIENT)
                                : sqlite3_bind_null(statement.get(), 6);
    if (ownerBindRc != SQLITE_OK) {
        throwSqliteError(db_, "Failed to bind SQLite owner_id parameter");
    }

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throwSqliteError(db_, "Failed to insert photo metadata into SQLite");
    }
}

void MetadataStore::deletePhoto(PhotoId id,
                                const std::string& repoType,
                                const std::optional<std::string>& ownerId) {
    constexpr const char* kDeleteSql =
        "DELETE FROM photos "
        "WHERE id = ? AND repo_type = ? AND ((? IS NULL AND owner_id IS NULL) OR owner_id = ?);";

    Statement statement(db_, kDeleteSql);

    if (sqlite3_bind_int64(statement.get(), 1, static_cast<sqlite3_int64>(id)) != SQLITE_OK) {
        throwSqliteError(db_, "Failed to bind SQLite delete id parameter");
    }

    bindRepositoryScope(statement.get(), 2, repoType, ownerId);

    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throwSqliteError(db_, "Failed to delete photo metadata from SQLite");
    }
}

bool MetadataStore::hasColumn(const std::string& tableName, const std::string& columnName) const {
    const std::string sql = "PRAGMA table_info(" + tableName + ");";
    Statement statement(db_, sql.c_str());

    while (true) {
        const int rc = sqlite3_step(statement.get());
        if (rc == SQLITE_DONE) {
            return false;
        }

        if (rc != SQLITE_ROW) {
            throwSqliteError(db_, "Failed to inspect table schema");
        }

        const auto* name = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1));
        if (name != nullptr && columnName == name) {
            return true;
        }
    }
}

void MetadataStore::bindRepositoryScope(sqlite3_stmt* statement,
                                        int startIndex,
                                        const std::string& repoType,
                                        const std::optional<std::string>& ownerId) {
    if (sqlite3_bind_text(statement, startIndex, repoType.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind SQLite repo_type parameter");
    }

    const int nullOrOwnerRc = ownerId.has_value()
                                  ? sqlite3_bind_text(statement,
                                                      startIndex + 1,
                                                      ownerId->c_str(),
                                                      -1,
                                                      SQLITE_TRANSIENT)
                                  : sqlite3_bind_null(statement, startIndex + 1);
    if (nullOrOwnerRc != SQLITE_OK) {
        throw std::runtime_error("Failed to bind SQLite owner_id parameter");
    }

    const int ownerEqRc = ownerId.has_value()
                              ? sqlite3_bind_text(statement,
                                                  startIndex + 2,
                                                  ownerId->c_str(),
                                                  -1,
                                                  SQLITE_TRANSIENT)
                              : sqlite3_bind_null(statement, startIndex + 2);
    if (ownerEqRc != SQLITE_OK) {
        throw std::runtime_error("Failed to bind SQLite owner_id comparison parameter");
    }
}

std::int64_t MetadataStore::toUnixTime(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point MetadataStore::fromUnixTime(std::int64_t seconds) {
    return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}
