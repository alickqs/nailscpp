//
// Created by Uliana S on 28.02.2026.
//

#ifndef NAILSCPP_PHOTO_REPO_H
#define NAILSCPP_PHOTO_REPO_H

#include "photo_repository/metadata_store.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "word2vec/model/word2vec.hpp"
#include "word2vec/search/description_embedder.hpp"
#include "word2vec/search/similarity_engine.hpp"
#include "word2vec/search/similarity_engine.hpp"
#include "word2vec/model/word2vec.hpp"


class PhotoRepositoryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class PhotoNotFoundError : public PhotoRepositoryError {
public:
    using PhotoRepositoryError::PhotoRepositoryError;
};

class FileOperationError : public PhotoRepositoryError {
public:
    using PhotoRepositoryError::PhotoRepositoryError;
};

class InvalidPhotoError : public PhotoRepositoryError {
public:
    using PhotoRepositoryError::PhotoRepositoryError;
};

class PhotoRepository {
public:
    static constexpr std::size_t kDefaultPageSize = 10;

    explicit PhotoRepository(std::filesystem::path storageDirectory,
                             std::string repoType = "shared",
                             std::optional<std::string> ownerId = std::nullopt);
    virtual ~PhotoRepository() = default;

    PhotoRepository(const PhotoRepository&) = delete;
    PhotoRepository& operator=(const PhotoRepository&) = delete;
    PhotoRepository(PhotoRepository&&) noexcept = default;
    PhotoRepository& operator=(PhotoRepository&&) noexcept = default;

    PhotoId uploadFromDevice(const std::filesystem::path& sourcePath,
                             std::string description);

    std::vector<ManiqureDataUpdated> listPhotos(
        std::size_t page = 0,
        std::size_t pageSize = kDefaultPageSize) const;

    std::optional<ManiqureDataUpdated> getPhotoInfo(PhotoId id) const;

    void downloadToDevice(PhotoId id,
                          const std::filesystem::path& destinationPath) const;

    bool contains(PhotoId id) const noexcept;
    std::size_t size() const noexcept;

    std::vector<PhotoId> findByDescription(std::string_view query) const;

protected:
    PhotoId copyPhotoToThisRepository(const ManiqureDataUpdated& photoToCopy);
    void removePhoto(PhotoId id);

    const std::filesystem::path& getStorageDirectory() const noexcept;
    const ManiqureDataUpdated* findPhotoById(PhotoId id) const noexcept;
    ManiqureDataUpdated* findPhotoById(PhotoId id) noexcept;

private:
    using Storage = std::vector<std::unique_ptr<ManiqureDataUpdated>>;

    std::filesystem::path storageDirectory_;
    std::string repoType_;
    std::optional<std::string> ownerId_;
    std::unique_ptr<MetadataStore> metadataStore_;
    Storage photos_;
    PhotoId nextId_{1};

    PhotoId generateId() noexcept;
    void ensureStorageDirectoryExists() const;
    std::filesystem::path buildStoredFilePath(PhotoId id, const std::filesystem::path& originalPath) const;
};

#endif // NAILSCPP_PHOTO_REPO_H
