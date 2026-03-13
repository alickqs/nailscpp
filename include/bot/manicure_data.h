//
// Created by Anna Mitricheva on 11.03.2026.
//
#ifndef NAILSCPP_PHOTO_H
#define NAILSCPP_PHOTO_H

#include <string>
#include <cstdint>
#include <filesystem>
#include <chrono>
#include <optional>

using PhotoId = std::uint64_t;

struct ManicureData {
    PhotoId id;
    std::string description;
    std::filesystem::path filePath;
    std::chrono::system_clock::time_point createdAt;
    std::string repoType;
    std::optional<std::string> ownerId;
};
#endif //NAILSCPP_PHOTO_H