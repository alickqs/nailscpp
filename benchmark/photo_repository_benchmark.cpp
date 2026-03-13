#include "photo_repository/metadata_store.h"

#include <chrono>
#include <filesystem>
#include <iostream>

int main() {
    const std::filesystem::path dbPath = "benchmark_db.sqlite3";

    MetadataStore store(dbPath);
    store.ensureSchema();

    const int N = 1000;

    // -------- INSERT BENCHMARK --------
    auto startInsert = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N; ++i) {
        ManiqureDataUpdated photo;
        photo.id = i + 1;
        photo.description = "test photo";
        photo.filePath = "file.jpg";
        photo.createdAt = std::chrono::system_clock::now();
        photo.repoType = "shared";
        photo.ownerId = std::nullopt;

        store.insertPhoto(photo);
    }

    auto endInsert = std::chrono::high_resolution_clock::now();

    // -------- SELECT BENCHMARK --------
    auto startSelect = std::chrono::high_resolution_clock::now();

    auto photos = store.loadAll("shared", std::nullopt);

    auto endSelect = std::chrono::high_resolution_clock::now();

    auto insertTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(endInsert - startInsert);

    auto selectTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(endSelect - startSelect);

    std::cout << "Inserted " << N << " photos in "
              << insertTime.count() << " ms\n";

    std::cout << "Loaded " << photos.size() << " photos in "
              << selectTime.count() << " ms\n";
}