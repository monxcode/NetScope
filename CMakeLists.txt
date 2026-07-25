cmake_minimum_required(VERSION 3.20)
project(NetScope VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
    add_compile_options(/permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-Wno-unused-parameter)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(NETSCOPE_DEBUG)
    if(MSVC)
        add_compile_options(/Od /RTC1)
    else()
        add_compile_options(-g -O0)
    endif()
else()
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
    endif()
endif()

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

if(WIN32)
    set(PLATFORM_LIBS ws2_32 iphlpapi)
    add_compile_definitions(NETSCOPE_WINDOWS)
    add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(UNIX)
    set(PLATFORM_LIBS pthread)
    add_compile_definitions(NETSCOPE_LINUX)
endif()

add_subdirectory(src)

option(NETSCOPE_BUILD_TESTS "Build unit tests" ON)
if(NETSCOPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
