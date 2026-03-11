cmake_minimum_required(VERSION 3.20)
project(nailscpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(BUILD_TELEGRAM_BOT "Build the Telegram bot application" OFF)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(BUILD_TELEGRAM_BOT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/CMakeLists.txt")
        add_subdirectory(app)
    else()
        message(WARNING "BUILD_TELEGRAM_BOT is ON, but app/CMakeLists.txt is missing. Telegram bot target will not be built.")
    endif()
endif()
