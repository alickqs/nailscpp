//
// Created by Uliana S on 28.02.2026.
//

#ifndef NAILSCPP_PHOTO_H
#define NAILSCPP_PHOTO_H

#include <string>
#include <cstdint>
#include <filesystem>
#include <chrono>
#include <optional>

using PhotoId = std::uint64_t;

struct Photo {
    PhotoId id;
    std::string description;
    std::filesystem::path filePath;
    std::chrono::system_clock::time_point createdAt;
    std::string repoType;
    std::optional<std::string> ownerId;

    Photo(PhotoId id,
          std::string description,
          std::filesystem::path filePath);

    Photo(PhotoId id,
          std::string description,
          std::filesystem::path filePath,
          std::chrono::system_clock::time_point createdAt);

    Photo(PhotoId id,
          std::string description,
          std::filesystem::path filePath,
          std::chrono::system_clock::time_point createdAt,
          std::string repoType,
          std::optional<std::string> ownerId);
};
#endif //NAILSCPP_PHOTO_H
