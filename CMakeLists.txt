cmake_minimum_required(VERSION 3.20)
project(nailscpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(BUILD_BENCHMARKS "Build benchmark targets" OFF)

add_subdirectory(src)
add_subdirectory(app)

if(BUILD_BENCHMARKS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/CMakeLists.txt")
        add_subdirectory(benchmark)
    else()
        message(WARNING "BUILD_BENCHMARKS is ON, but benchmark/CMakeLists.txt is missing. Benchmark targets will not be built.")
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
