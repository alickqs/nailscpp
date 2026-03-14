if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG QUIET)

add_executable(nailscpp_bot
        bot/main.cpp
)

target_include_directories(nailscpp_bot PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

if (TARGET Boost::headers)
    target_link_libraries(nailscpp_bot PRIVATE Boost::headers)
elseif(TARGET Boost::boost)
    target_link_libraries(nailscpp_bot PRIVATE Boost::boost)
else()
    target_include_directories(nailscpp_bot PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(nailscpp_bot PRIVATE nlohmann_json::nlohmann_json)
endif()

target_link_libraries(nailscpp_bot PRIVATE
        bot_lib
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

if (MSVC)
    target_compile_options(nailscpp_bot PRIVATE /W4)
else()
    target_compile_options(nailscpp_bot PRIVATE -Wall -Wextra -Wpedantic)
endif()
