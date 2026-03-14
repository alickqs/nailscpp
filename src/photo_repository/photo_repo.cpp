#include "photo_repository/photo_repo.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "word2vec/model/word2vec.hpp"
#include "word2vec/search/description_embedder.hpp"
#include "word2vec/search/similarity_engine.hpp"
#include "globals.h"


#include "photo_repository/photo_repo.h"

// define them once here
Word2VecModel model{50};
DescriptionEmbedder embedder{50};
SimilarityEngine search_index;
bool ml_ready = false;
PhotoRepository::PhotoRepository(std::filesystem::path storageDirectory,
                                 std::string repoType,
                                 std::optional<std::string> ownerId)
    : storageDirectory_(std::move(storageDirectory)),
      repoType_(std::move(repoType)),
      ownerId_(std::move(ownerId)) {
    ensureStorageDirectoryExists();

    metadataStore_ = std::make_unique<MetadataStore>(storageDirectory_ / "metadata.sqlite3");
    metadataStore_->ensureSchema();

    for (ManiqureDataUpdated& photo : metadataStore_->loadAll(repoType_, ownerId_)) {
        photos_.push_back(std::make_unique<ManiqureDataUpdated>(std::move(photo)));
    }

    nextId_ = metadataStore_->loadMaxId() + 1;
    if (!photos_.empty()) {

        std::vector<std::vector<int>> corpus;

        auto& vocab = model.get_vocabulary();

        Tokenizer tok;

        for (auto& p : photos_) {

            auto tokens = tok.tokenize(p->description);

            std::vector<int> sentence;

            for (auto& t : tokens)
                sentence.push_back(vocab.add_word(t));

            corpus.push_back(sentence);
        }

        model.initialize_sampler();
        model.train(corpus);

        for (auto& p : photos_) {

            auto vec = embedder.embed(p->description, vocab);
            search_index.add(p->id, vec);
        }

        ml_ready = true;
    }

}

PhotoId PhotoRepository::uploadFromDevice(const std::filesystem::path& sourcePath,
                                          std::string description) {
    if (description.empty()) {
        throw InvalidPhotoError("Photo description cannot be empty");
    }

    if (!std::filesystem::exists(sourcePath)) {
        throw FileOperationError("Source file does not exist: " + sourcePath.string());
    }

    if (!std::filesystem::is_regular_file(sourcePath)) {
        throw FileOperationError("Source path is not a regular file: " + sourcePath.string());
    }

    const PhotoId newId = generateId();
    const std::filesystem::path storedPath = buildStoredFilePath(newId, sourcePath);

    try {
        std::filesystem::copy_file(
            sourcePath,
            storedPath,
            std::filesystem::copy_options::overwrite_existing);

        auto photo = std::make_unique<ManiqureDataUpdated>();
        photo->id = newId;
        photo->description = std::move(description);
        photo->filePath = storedPath;
        photo->createdAt = std::chrono::system_clock::now();
        photo->repoType = repoType_;
        photo->ownerId = ownerId_;
        photo->photoUrl = storedPath.string();
        photo->userId = ownerId_.value_or("");
        metadataStore_->insertPhoto(*photo);
        photos_.push_back(std::move(photo));
    } catch (const std::filesystem::filesystem_error& e) {
        throw FileOperationError("Failed to copy file into repository: " + std::string(e.what()));
    } catch (const std::exception& e) {
        try {
            if (std::filesystem::exists(storedPath)) {
                std::filesystem::remove(storedPath);
            }
        } catch (...) {
        }
        throw PhotoRepositoryError("Failed to persist uploaded photo metadata: " + std::string(e.what()));
    }

    return newId;
}

std::vector<ManiqureDataUpdated> PhotoRepository::listPhotos(
    std::size_t page,
    std::size_t pageSize) const {
    std::vector<ManiqureDataUpdated> result;

    if (pageSize == 0) {
        return result;
    }

    const std::size_t startIndex = page * pageSize;
    if (startIndex >= photos_.size()) {
        return result;
    }

    const std::size_t endIndex = std::min(startIndex + pageSize, photos_.size());
    result.reserve(endIndex - startIndex);

    for (std::size_t i = startIndex; i < endIndex; ++i) {
        result.push_back(*photos_[i]);
    }

    return result;
}

std::optional<ManiqureDataUpdated> PhotoRepository::getPhotoInfo(PhotoId id) const {
    const ManiqureDataUpdated* photo = findPhotoById(id);
    if (photo == nullptr) {
        return std::nullopt;
    }

    return *photo;
}

void PhotoRepository::downloadToDevice(PhotoId id,
                                       const std::filesystem::path& destinationPath) const {
    const ManiqureDataUpdated* photo = findPhotoById(id);
    if (photo == nullptr) {
        throw PhotoNotFoundError("Photo not found");
    }

    std::filesystem::path finalPath = destinationPath;

    if (std::filesystem::exists(destinationPath) && std::filesystem::is_directory(destinationPath)) {
        finalPath /= photo->filePath.filename();
    }

    try {
        std::filesystem::copy_file(
            photo->filePath,
            finalPath,
            std::filesystem::copy_options::overwrite_existing);
    } catch (const std::filesystem::filesystem_error& e) {
        throw FileOperationError("Failed to download photo: " + std::string(e.what()));
    }
}

bool PhotoRepository::contains(PhotoId id) const noexcept {
    return findPhotoById(id) != nullptr;
}

std::size_t PhotoRepository::size() const noexcept {
    return photos_.size();
}

PhotoId PhotoRepository::copyPhotoToThisRepository(const ManiqureDataUpdated& photoToCopy) {
    if (!std::filesystem::exists(photoToCopy.filePath)) {
        throw FileOperationError("Original photo file does not exist: " +
                                 photoToCopy.filePath.string());
    }

    const PhotoId newId = generateId();
    const std::filesystem::path newStoredPath = buildStoredFilePath(newId, photoToCopy.filePath);

    try {
        std::filesystem::copy_file(
            photoToCopy.filePath,
            newStoredPath,
            std::filesystem::copy_options::overwrite_existing);

        auto copiedPhoto = std::make_unique<ManiqureDataUpdated>();
        copiedPhoto->id = newId;
        copiedPhoto->description = photoToCopy.description;
        copiedPhoto->filePath = newStoredPath;
        copiedPhoto->createdAt = photoToCopy.createdAt;
        copiedPhoto->repoType = repoType_;
        copiedPhoto->ownerId = ownerId_;
        copiedPhoto->photoUrl = newStoredPath.string();
        copiedPhoto->userId = ownerId_.value_or("");
        copiedPhoto->timestamp = photoToCopy.timestamp;

        metadataStore_->insertPhoto(*copiedPhoto);
        photos_.push_back(std::move(copiedPhoto));
    } catch (const std::filesystem::filesystem_error& e) {
        throw FileOperationError("Failed to copy photo between repositories: " + std::string(e.what()));
    } catch (const std::exception& e) {
        try {
            if (std::filesystem::exists(newStoredPath)) {
                std::filesystem::remove(newStoredPath);
            }
        } catch (...) {
        }
        throw PhotoRepositoryError("Failed to persist copied photo metadata: " + std::string(e.what()));
    }

    return newId;
}

void PhotoRepository::removePhoto(PhotoId id) {
    auto it = std::find_if(
        photos_.begin(),
        photos_.end(),
        [id](const std::unique_ptr<ManiqureDataUpdated>& photo) {
            return photo->id == id;
        });

    if (it == photos_.end()) {
        throw PhotoNotFoundError("Photo not found");
    }

    try {
        if (std::filesystem::exists((*it)->filePath)) {
            std::filesystem::remove((*it)->filePath);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw FileOperationError("Failed to remove photo file: " + std::string(e.what()));
    }

    try {
        metadataStore_->deletePhoto(id, repoType_, ownerId_);
    } catch (const std::exception& e) {
        throw PhotoRepositoryError("Failed to remove photo metadata: " + std::string(e.what()));
    }

    photos_.erase(it);
}

const std::filesystem::path& PhotoRepository::getStorageDirectory() const noexcept {
    return storageDirectory_;
}

const ManiqureDataUpdated* PhotoRepository::findPhotoById(PhotoId id) const noexcept {
    const auto it = std::find_if(
        photos_.begin(),
        photos_.end(),
        [id](const std::unique_ptr<ManiqureDataUpdated>& photo) {
            return photo->id == id;
        });

    return (it != photos_.end()) ? it->get() : nullptr;
}

ManiqureDataUpdated* PhotoRepository::findPhotoById(PhotoId id) noexcept {
    const auto it = std::find_if(
        photos_.begin(),
        photos_.end(),
        [id](const std::unique_ptr<ManiqureDataUpdated>& photo) {
            return photo->id == id;
        });

    return (it != photos_.end()) ? it->get() : nullptr;
}

PhotoId PhotoRepository::generateId() noexcept {
    return nextId_++;
}

void PhotoRepository::ensureStorageDirectoryExists() const {
    try {
        if (!std::filesystem::exists(storageDirectory_)) {
            std::filesystem::create_directories(storageDirectory_);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw FileOperationError("Failed to create storage directory: " + std::string(e.what()));
    }
}

std::filesystem::path PhotoRepository::buildStoredFilePath(
    PhotoId id,
    const std::filesystem::path& originalPath) const {
    const std::string fileName = std::to_string(id) + originalPath.extension().string();
    return storageDirectory_ / fileName;
}

std::vector<PhotoId> PhotoRepository::findByDescription(std::string_view query) const {
    (void)query;
    std::vector<PhotoId> result;
    /*
    место для реализации поиска
     */
    return result;
}
