find_package(GTest QUIET)

if (NOT GTest_FOUND)
    include(FetchContent)

    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(nailscpp_smoke_test
        smoke_test1.cpp
        ../include/bot/manicure_data.h
)

target_link_libraries(nailscpp_smoke_test PRIVATE
        photo_repository
        GTest::gtest_main
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

gtest_discover_tests(nailscpp_smoke_test)
