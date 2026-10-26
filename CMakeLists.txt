cmake_minimum_required(VERSION 3.24)
project(mini_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
)
FetchContent_MakeAvailable(SDL3)

add_executable(mini_engine
    src/main.cpp
    src/Window.cpp
    src/Benchmark.cpp
 "src/LinearAllocator.h" "src/PoolAllocator.h" "src/Benchmark.h" "src/Benchmark.cpp")

target_include_directories(mini_engine PRIVATE ${sdl3_SOURCE_DIR}/include)
target_link_libraries(mini_engine PRIVATE SDL3::SDL3-static)

if (MSVC)
    target_compile_options(mini_engine PRIVATE /W4)
else()
    target_compile_options(mini_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()