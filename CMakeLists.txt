CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

PROJECT(
    asyncio
    LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -ggdb -O0")
SET(CMAKE_CXX_VISIBILITY_PRESET hidden)
SET(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
SET(CMAKE_EXPORT_COMPILE_COMMANDS on)
SET(BUILD_TESTS true CACHE BOOL "if to build test")

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug)
endif()

INCLUDE(FetchContent)
INCLUDE(GenerateExportHeader)

FetchContent_Declare(
    tiny_thread_pool
    GIT_REPOSITORY "https://github.com/kewuaa/tiny_thread_pool.git"
    GIT_TAG master
)
FetchContent_MakeAvailable(tiny_thread_pool)

FIND_PACKAGE(spdlog REQUIRED)


ADD_LIBRARY(${PROJECT_NAME} SHARED)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PUBLIC "-D_DEBUG")
endif()
TARGET_SOURCES(
    ${PROJECT_NAME}
    PRIVATE
        src/event_loop.cpp
        src/epoll.cpp
        src/socket.cpp
        src/timer.cpp
        src/promise.cpp
        src/exception.cpp
)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC include)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC spdlog::spdlog)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC tiny_thread_pool)
GENERATE_EXPORT_HEADER(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/asyncio_export.hpp)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/exports)

ADD_EXECUTABLE("test_${PROJECT_NAME}")
TARGET_SOURCES("test_${PROJECT_NAME}" PRIVATE main.cpp)
TARGET_INCLUDE_DIRECTORIES("test_${PROJECT_NAME}" PRIVATE include)
TARGET_LINK_LIBRARIES("test_${PROJECT_NAME}" PRIVATE ${PROJECT_NAME})

if (BUILD_TESTS)
    FetchContent_Declare(
        ut
        GIT_REPOSITORY "https://github.com/boost-ext/ut.git"
        GIT_TAG master
    )
    FetchContent_MakeAvailable(ut)

    ENABLE_TESTING()
    ADD_SUBDIRECTORY(tests)
endif()
