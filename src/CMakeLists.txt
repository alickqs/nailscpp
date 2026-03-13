find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# Photo repository library
add_library(photo_repository
        photo_repository/photo_repo.cpp
        photo_repository/personal_repo.cpp
        photo_repository/shared_repo.cpp
        photo_repository/metadata_store.cpp
)

target_include_directories(photo_repository PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(photo_repository PUBLIC
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
)

# Bot library
add_library(bot_lib
        bot/tg_bot.cpp
)

target_include_directories(bot_lib PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(bot_lib PUBLIC
        photo_repository
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Main executable
add_executable(nailscpp
        main.cpp
)

target_link_libraries(nailscpp PRIVATE
        bot_lib
)

# warnings
if (MSVC)
    target_compile_options(photo_repository PRIVATE /W4)
    target_compile_options(bot_lib PRIVATE /W4)
    target_compile_options(nailscpp PRIVATE /W4)
else()
    target_compile_options(photo_repository PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(bot_lib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(nailscpp PRIVATE -Wall -Wextra -Wpedantic)
endif()