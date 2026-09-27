cmake_minimum_required(VERSION 3.28...4.2.3)
message(STATUS "CMake version: " ${CMAKE_VERSION} " is ok")

project(storage_engine
    VERSION 0.0
    DESCRIPTION "Мини база данных"
    LANGUAGES CXX 
)

set(MIN_GCC_COMPILER_VERSION "15.2.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS)

