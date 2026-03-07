add_executable(app ${CMAKE_SOURCE_DIR}/app/bot/main.cpp)
target_link_libraries(app PRIVATE tg_bot)