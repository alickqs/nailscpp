find_package(SQLite3 REQUIRED)

add_library(photo_repository
        photo_repository/photo_repo.cpp
        photo_repository/personal_repo.cpp
        photo_repository/shared_repo.cpp
        photo_repository/metadata_store.cpp
        ../include/bot/manicure_data.h
)

target_include_directories(photo_repository PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(photo_repository PUBLIC
        SQLite::SQLite3
)

add_executable(nailscpp
        main.cpp
        ../include/bot/manicure_data.h
)

target_link_libraries(nailscpp PRIVATE
        photo_repository
)

if (MSVC)
    target_compile_options(photo_repository PRIVATE /W4)
    target_compile_options(nailscpp PRIVATE /W4)
else()
    target_compile_options(photo_repository PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(nailscpp PRIVATE -Wall -Wextra -Wpedantic)
endif()
