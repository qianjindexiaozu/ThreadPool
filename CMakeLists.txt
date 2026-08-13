cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
    ThreadPool
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

add_executable(
    ThreadPool
    ThreadPool.cpp
)

target_include_directories(
    ThreadPool
    PRIVATE
)

target_compile_options(
    ThreadPool
    PRIVATE
)

target_link_libraries(
    ThreadPool
    PRIVATE
    Threads::Threads
)

