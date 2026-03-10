add_library(photo_repository
        photo_repository/photo.cpp
        photo_repository/photo_repo.cpp
        photo_repository/personal_repo.cpp
        photo_repository/shared_repo.cpp
)

target_include_directories(photo_repository PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/photo_repository
)

#add_library(word2vec
#        word2vec/word2vec.cpp
#)
#
#
#
#set(CMAKE_PREFIX_PATH "/opt/homebrew")
#
#find_package(Boost REQUIRED COMPONENTS filesystem)  # только filesystem
#find_package(nlohmann_json REQUIRED)
#find_package(OpenSSL REQUIRED)
#
#add_library(tg_bot
#        main.cpp
#        ${CMAKE_SOURCE_DIR}/include/bot/tg_bot.h
#        tg_bot.cpp
#)
#
#target_link_libraries(me
#        OpenSSL::SSL
#        OpenSSL::Crypto
#        nlohmann_json::nlohmann_json
#        Boost::filesystem
#)
#
#
#target_include_directories(word2vec PUBLIC
#        ${CMAKE_CURRENT_SOURCE_DIR}/../include
#)
#
#add_executable(nailscpp
#        main.cpp
#)
#
#target_link_libraries(nailscpp PRIVATE photo_repository word2vec)
#
#if (MSVC)
#    target_compile_options(photo_repository PRIVATE /W4)
#    target_compile_options(word2vec PRIVATE /W4)
#    target_compile_options(nailscpp PRIVATE /W4)
#else()
#    target_compile_options(photo_repository PRIVATE -Wall -Wextra -Wpedantic)
#    target_compile_options(word2vec PRIVATE -Wall -Wextra -Wpedantic)
#    target_compile_options(nailscpp PRIVATE -Wall -Wextra -Wpedantic)
#endif()


set(CMAKE_PREFIX_PATH "/opt/homebrew")

find_package(Boost REQUIRED COMPONENTS filesystem)  # только filesystem
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(tg_bot
        ${CMAKE_SOURCE_DIR}/src/bot/tg_bot.cpp
)

target_include_directories(photo_repository PUBLIC
        ${CMAKE_SOURCE_DIR}/include/bot
)

target_link_libraries(tg_bot
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Boost::filesystem
)
