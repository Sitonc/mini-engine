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
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(SDL3 json)

add_executable(mini_engine
    src/main.cpp
    src/Window.cpp
    src/Benchmark.cpp
    src/EcsBenchmark.cpp
    src/SceneLoader.cpp
    src/JobBenchmark.cpp
 "src/LinearAllocator.h" "src/PoolAllocator.h" "src/Benchmark.h" "src/Benchmark.cpp" "src/Ecs.h" "src/EcsBenchmark.h" "src/EcsBenchmark.cpp" "src/Components.h" "src/SceneLoader.cpp" "src/SceneLoader.h" "src/FileWatcher.h" "src/JobSystem.h" "src/JobBenchmark.h" "src/JobBenchmark.cpp")

find_package(Threads REQUIRED)
target_link_libraries(mini_engine PRIVATE Threads::Threads)
target_include_directories(mini_engine PRIVATE ${sdl3_SOURCE_DIR}/include)
target_link_libraries(mini_engine PRIVATE SDL3::SDL3-static)
target_link_libraries(mini_engine PRIVATE SDL3::SDL3-static nlohmann_json::nlohmann_json)
target_compile_definitions(mini_engine PRIVATE "ASSET_DIR=\"${CMAKE_SOURCE_DIR}/assets\"")

if (MSVC)
    target_compile_options(mini_engine PRIVATE /W4)
else()
    target_compile_options(mini_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()