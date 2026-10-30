cmake_minimum_required(VERSION 3.24)
project(mini_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Con generatori single-config (Ninja, Makefile) usa Release di default.
# Con Visual Studio questa parte viene ignorata.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- Opzioni ---------------------------------------------------------------
option(ENABLE_PROFILER "Abilita il profiler Tracy" ON)

# SDL come libreria statica (niente problemi con SDL3.dll mancante)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# Tracy
set(TRACY_ENABLE ${ENABLE_PROFILER} CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)

# --- Dipendenze ------------------------------------------------------------
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

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 json tracy)

find_package(Threads REQUIRED)

# --- Eseguibile ------------------------------------------------------------
add_executable(mini_engine
    src/main.cpp
    src/Window.cpp
    src/Benchmark.cpp
    src/EcsBenchmark.cpp
    src/SceneLoader.cpp
    src/JobBenchmark.cpp
    src/Sim.cpp
 "src/Profiler.h")

target_include_directories(mini_engine PRIVATE ${sdl3_SOURCE_DIR}/include)

target_link_libraries(mini_engine PRIVATE
    SDL3::SDL3-static
    nlohmann_json::nlohmann_json
    Tracy::TracyClient
    Threads::Threads
)

# Cartella assets (usata per l'hot reload di scene.json)
target_compile_definitions(mini_engine PRIVATE
    "ASSET_DIR=\"${CMAKE_SOURCE_DIR}/assets\""
)

if(MSVC)
    target_compile_options(mini_engine PRIVATE /W4)
else()
    target_compile_options(mini_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()