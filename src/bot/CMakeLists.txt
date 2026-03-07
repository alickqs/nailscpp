cmake_minimum_required(VERSION 3.14)
project(me)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "/opt/homebrew")

find_package(Boost REQUIRED COMPONENTS filesystem)  # только filesystem
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(me
        main.cpp
        tg_bot.h
        ../nailscpp/src/bot/tg_bot.cpp
)

target_link_libraries(me
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Boost::filesystem
)
