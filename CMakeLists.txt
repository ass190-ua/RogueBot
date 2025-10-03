cmake_minimum_required(VERSION 3.16)
project(RogueBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warnings útiles
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Fuentes
set(SOURCES
  src/main.cpp
  src/Game.cpp
  src/Player.cpp
  src/Enemy.cpp
  src/Map.cpp
  src/HUD.cpp
  src/State.cpp
)

add_executable(roguebot ${SOURCES})

# Incluye cabeceras propias y vendor/include si lo usas
target_include_directories(roguebot PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/vendor/include
)

# ---- Raylib: usar del sistema si existe, si no, FetchContent (descarga/compila) ----
include(FetchContent)
find_package(raylib QUIET)

if(NOT raylib_FOUND)
  message(STATUS "raylib no encontrado en el sistema → usando FetchContent (raylib 5.0)")
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
  )
  FetchContent_MakeAvailable(raylib)
endif()

# Enlace con raylib
target_link_libraries(roguebot PRIVATE raylib)

# Extras de Linux (no afectan a Windows/macOS)
if(UNIX AND NOT APPLE)
  target_link_libraries(roguebot PRIVATE m pthread dl rt X11)
endif()
