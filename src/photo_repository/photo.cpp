//
// Created by Uliana S on 28.02.2026.
//


#include "photo_repository/photo.h"

Photo::Photo(PhotoId id,
             std::string description,
             std::filesystem::path filePath)
        : Photo(id,
                std::move(description),
                std::move(filePath),
                std::chrono::system_clock::now(),
                "shared",
                std::nullopt)
{}

Photo::Photo(PhotoId id,
             std::string description,
             std::filesystem::path filePath,
             std::chrono::system_clock::time_point createdAt)
        : Photo(id,
                std::move(description),
                std::move(filePath),
                createdAt,
                "shared",
                std::nullopt)
{}

Photo::Photo(PhotoId id,
             std::string description,
             std::filesystem::path filePath,
             std::chrono::system_clock::time_point createdAt,
             std::string repoType,
             std::optional<std::string> ownerId)
        : id(id),
          description(std::move(description)),
          filePath(std::move(filePath)),
          createdAt(createdAt),
          repoType(std::move(repoType)),
          ownerId(std::move(ownerId))
{}
