cmake_minimum_required(VERSION 3.28...4.2.3)
message(STATUS "CMake version: " ${CMAKE_VERSION} " is ok")

project(storage_engine
    VERSION 0.0
    DESCRIPTION "Движок хранения данных"
    LANGUAGES CXX 
)

include(FetchContent)

set(MIN_GCC_COMPILER_VERSION "10.0.0")
set(MIN_CLANG_COMPILER_VERSION "10.0.0")


IF (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} " is unsupported")
ENDIF()

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_GCC_COMPILER_VERSION})
    message(STATUS "Current version of " ${CMAKE_CXX_COMPILER_ID} "is: " ${CMAKE_CXX_COMPILER_VERSION})
    message(FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " version is less " ${MIN_GCC_COMPILER_VERSION} " and is not acceptable")
ENDIF()

IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${MIN_CLANG_COMPILER_VERSION})
    message(STATUS "Current version of " ${CMAKE_CXX_COMPILER_ID} "is: " ${CMAKE_CXX_COMPILER_VERSION})
    message(FATAL_ERROR ${CMAKE_CXX_COMPILER_ID} " version is less " ${MIN_CLANG_COMPILER_VERSION} " and is not acceptable")
ENDIF()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

IF (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
ELSEIF (NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND
        NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND
        NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(FATAL_ERROR "Unsupported build type")
ENDIF()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(SANITIZER_TYPE "off" CACHE STRING "Используются ли санитайзеры")

#Общие флаги компиляции на все виды

set(COMMON_COMPILE_OPTIONS 
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -pedantic-errors)

#Общие флаги линковки на все виды
set(COMMON_LINK_OPTIONS)

IF (SANITIZER_TYPE STREQUAL "ADDRESS_UB_SANITIZE")
    set(SANITIZER_FLAGS
    -fsanitize=address
    -fsanitize=undefined
    -fno-sanitize-recover=all)
ELSEIF (SANITIZER_TYPE STREQUAL "UB_THREAD_SANITIZE")
    set(SANITIZER_FLAGS
    -fsanitize=thread
    -fsanitize=undefined
    -fno-sanitize-recover=all)
ELSEIF (SANITIZER_TYPE STREQUAL "off")
    set(SANITIZER_FLAGS)
ELSE()
    message(FATAL_ERROR "Unsupported sanitizer type")
ENDIF()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)
enable_testing()

add_library(storage_engine STATIC
src/initfile.cpp)
target_include_directories(storage_engine PUBLIC include/)

target_compile_options(storage_engine PRIVATE 
${COMMON_COMPILE_OPTIONS}
$<$<CONFIG:Debug,RelWithDebInfo>:-fno-omit-frame-pointer>
${SANITIZER_FLAGS}
)

target_link_options(storage_engine PRIVATE
${COMMON_LINK_OPTIONS}
${SANITIZER_FLAGS}
)

add_executable(asan_test tests/asan_test.cpp)
target_link_libraries(asan_test PRIVATE GTest::gtest_main storage_engine)
add_test(NAME asan_test COMMAND asan_test)


