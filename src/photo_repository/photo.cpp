//
// Created by Uliana S on 28.02.2026.
//

// structure "photo" - идентификатор, описание, путь к файлу/данные, метаданные
// !! хранение фото лучше сделать через RAII-объект, который сам управляет ресурсом (например, файл на диске или буфер памяти).

#include "../../include/photo_repository/photo.h"

Photo::Photo(PhotoId id,
             std::string description,
             std::filesystem::path filePath)
        : id(id),
          description(std::move(description)),
          filePath(std::move(filePath)),
          createdAt(std::chrono::system_clock::now())
{}