//
// Created by Uliana S on 28.02.2026.
//

#include "photo_repository/personal_repo.h"

#include "photo_repository/shared_repo.h"

PersonalRepository::PersonalRepository(std::filesystem::path storageDirectory, std::string ownerId)
    : PhotoRepository(std::move(storageDirectory), "personal", std::move(ownerId)) {}

PhotoId PersonalRepository::exportPhotoToShared(PhotoId id, SharedRepository& sharedRepository, bool removeAfterTransfer) {
    const std::optional<Photo> photo = getPhotoInfo(id);
    if (!photo.has_value()) {
        throw PhotoNotFoundError("Photo not found in personal repository");
    }

    const PhotoId sharedPhotoId = sharedRepository.importFromPersonal(*photo);

    if (removeAfterTransfer) {
        removePhoto(id);
    }

    return sharedPhotoId;
}
