//
// Created by Uliana S on 28.02.2026.
//

#ifndef NAILSCPP_PERSONAL_REPO_H
#define NAILSCPP_PERSONAL_REPO_H

#include "photo_repository/photo_repo.h"

class SharedRepository;

class PersonalRepository final : public PhotoRepository {
public:
    PersonalRepository(std::filesystem::path storageDirectory, std::string ownerId);

    PhotoId exportPhotoToShared(PhotoId id,
                                SharedRepository& sharedRepository,
                                bool removeAfterTransfer = false);
};

#endif //NAILSCPP_PERSONAL_REPO_H
