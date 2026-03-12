#ifndef NAILSCPP_METADATA_STORE_H
#define NAILSCPP_METADATA_STORE_H

#include "photo_repository/maniqure_data_updated.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

class MetadataStore {
public:
    explicit MetadataStore(std::filesystem::path databasePath);
    ~MetadataStore();

    MetadataStore(const MetadataStore&) = delete;
    MetadataStore& operator=(const MetadataStore&) = delete;
    MetadataStore(MetadataStore&&) noexcept = delete;
    MetadataStore& operator=(MetadataStore&&) noexcept = delete;

    void ensureSchema();
    std::vector<ManiqureDataUpdated> loadAll(const std::string& repoType,
                                             const std::optional<std::string>& ownerId) const;
    PhotoId loadMaxId() const;
    void insertPhoto(const ManiqureDataUpdated& photo);
    void deletePhoto(PhotoId id,
                     const std::string& repoType,
                     const std::optional<std::string>& ownerId);

private:
    std::filesystem::path databasePath_;
    sqlite3* db_{};

    bool hasColumn(const std::string& tableName, const std::string& columnName) const;
    static void bindRepositoryScope(sqlite3_stmt* statement,
                                    int startIndex,
                                    const std::string& repoType,
                                    const std::optional<std::string>& ownerId);
    static std::int64_t toUnixTime(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point fromUnixTime(std::int64_t seconds);
};

#endif // NAILSCPP_METADATA_STORE_H
