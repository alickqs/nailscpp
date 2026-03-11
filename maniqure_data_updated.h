//
// Created by Uliana S on 11.03.2026.
//

#ifndef NAILSCPP_MANIQURE_DATA_UPDATED_H
#define NAILSCPP_MANIQURE_DATA_UPDATED_H

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

using PhotoId = std::uint64_t;

struct ManiqureDataUpdated {
    PhotoId id{};
    std::string description;
    std::filesystem::path filePath;
    std::chrono::system_clock::time_point createdAt{};
    std::string repoType;
    std::optional<std::string> ownerId;

    std::string photoUrl;
    std::string userId;
    std::string timestamp;
};

#endif
