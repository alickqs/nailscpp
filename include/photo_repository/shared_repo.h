//
// Created by Uliana S on 28.02.2026.
//

#ifndef NAILSCPP_SHARED_REPO_H
#define NAILSCPP_SHARED_REPO_H

#include "photo_repository/photo_repo.h"

class SharedRepository final : public PhotoRepository {
public:
    explicit SharedRepository(std::filesystem::path storageDirectory);

    PhotoId importFromPersonal(const Photo& photo);
};

#endif //NAILSCPP_SHARED_REPO_H
