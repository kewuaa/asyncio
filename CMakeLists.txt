cmake_minimum_required(VERSION 3.15)

project(
    asyncio
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -ggdb -O0")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(BUILD_ECHO TRUE CACHE BOOL "if to build echo server")
set(BUILD_TESTS TRUE CACHE BOOL "if to build test")

set(ASYNCIO_MAX_EVENTS_NUM 1024 CACHE STRING "max events number of asyncio")
set(ASYNCIO_MAX_SELECT_TIMEOUT "24 * 3600000" CACHE STRING "max timeout for select(milliseconds)")
set(ASYNCIO_THREAD_POOL_SIZE 4 CACHE STRING "thread pool size for asyncio")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(BUILD_ECHO FALSE)
    set(BUILD_TESTS FALSE)
endif()

include(cmake/CPM.cmake)
include(GenerateExportHeader)

CPMAddPackage(
    tiny_thread_pool
    GIT_REPOSITORY "https://github.com/kewuaa/tiny_thread_pool.git"
    GIT_TAG master
)

find_package(spdlog REQUIRED)

add_library(${PROJECT_NAME} SHARED)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_options(${PROJECT_NAME} PUBLIC "-D_DEBUG")
endif()

configure_file(
    include/asyncio/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/asyncio_config.hpp
)

generate_export_header(
    ${PROJECT_NAME}
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/asyncio_export.hpp
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        src/event_loop.cpp
        src/epoll.cpp
        src/socket.cpp
        src/timer.cpp
        src/handle.cpp
        src/coro_handle.cpp
        src/exception.cpp
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC tiny_thread_pool)

if (BUILD_ECHO)
    add_executable(echo_server)
    target_sources(echo_server PRIVATE echo.cpp)
    target_link_libraries(echo_server PRIVATE ${PROJECT_NAME})
endif()

if (BUILD_TESTS)
    CPMAddPackage(
        GIT_REPOSITORY "https://github.com/boost-ext/ut.git"
        GIT_TAG v2.3.1
        OPTIONS "BOOST_UT_DISABLE_MODULE ON"
    )

    ENABLE_TESTING()
    ADD_SUBDIRECTORY(tests)
endif()
