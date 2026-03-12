//
// Created by Uliana S on 28.02.2026.
//

#include "photo_repository/shared_repo.h"

SharedRepository::SharedRepository(std::filesystem::path storageDirectory)
    : PhotoRepository(std::move(storageDirectory), "shared", std::nullopt) {}

PhotoId SharedRepository::importFromPersonal(const ManiqureDataUpdated& data) {
    return copyPhotoToThisRepository(data);
}
