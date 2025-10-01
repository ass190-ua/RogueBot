cmake_minimum_required(VERSION 3.16)
project(RogueBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warnings útiles
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Fuentes del Hito 1
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

# Donde están las cabeceras propias y (futuras) de vendor
target_include_directories(roguebot PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/vendor/include
)

# (Opcional futuro) libs en vendor/lib
# link_directories(${CMAKE_SOURCE_DIR}/vendor/lib)
# target_link_libraries(roguebot <libname>)
