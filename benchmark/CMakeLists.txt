find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)


add_executable(photo_repo_benchmark
        photo_repository_benchmark.cpp
)

target_include_directories(photo_repo_benchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(photo_repo_benchmark
        photo_repository
        SQLite::SQLite3
)


add_executable(tg_bot_benchmark
        tg_bot_benchmark.cpp
)

target_include_directories(tg_bot_benchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tg_bot_benchmark
        bot_lib
        photo_repository
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
)