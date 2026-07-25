FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    test_main.cpp
    test_config.cpp
    test_device.cpp
    test_logger.cpp
    test_thread_pool.cpp
    test_scanner.cpp
    test_ping_sweep.cpp
)

add_executable(netscope_tests ${TEST_SOURCES})

target_include_directories(netscope_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(netscope_tests PRIVATE
    gtest_main
    gmock
    nlohmann_json::nlohmann_json
    ${PLATFORM_LIBS}
)

gtest_discover_tests(netscope_tests)
