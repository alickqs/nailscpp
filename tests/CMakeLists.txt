# ---------- Always build our own static GoogleTest ----------
include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(nailscpp_smoke_test
        photo_repository_smoke_test.cpp
        photo_repository_sanity_test.cpp
)

target_link_libraries(nailscpp_smoke_test
        PRIVATE
        GTest::gtest_main
        bot_lib
)

# ⭐ THIS is the real fix for macOS static archive resolution
target_link_options(nailscpp_smoke_test PRIVATE
        "LINKER:-all_load"
)


target_compile_definitions(nailscpp_smoke_test PRIVATE
        NAILSCPP_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        NAILSCPP_BINARY_DIR="${CMAKE_BINARY_DIR}"
)

if (MSVC)
    target_compile_options(nailscpp_smoke_test PRIVATE /W4)
else()
    target_compile_options(nailscpp_smoke_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(nailscpp_smoke_test DISCOVERY_TIMEOUT 30)