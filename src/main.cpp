//
// Created by Uliana S on 07.03.2026.
//
#include "photo_repository/personal_repo.h"
#include "photo_repository/shared_repo.h"

#include <iostream>

int main() {
    namespace fs = std::filesystem;

    PersonalRepository personal("test_data/personal_repo", "123456789"); // owner_id
    SharedRepository shared("test_data/shared_repo");

    const fs::path sourcePhoto = "test_data/source/photo1.jpg";
    const PhotoId personalId = personal.uploadFromDevice(sourcePhoto, "manual test");

    const PhotoId sharedId = personal.exportPhotoToShared(personalId, shared, false);

    const fs::path downloaded = "test_data/downloads/from_shared.jpg";
    shared.downloadToDevice(sharedId, downloaded);

    std::cout << "Downloaded to: " << downloaded << '\n';
    return 0;
}



