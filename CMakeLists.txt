cmake_minimum_required(VERSION 3.16)
project(RogueBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(roguebot
  src/main.cpp
)

target_include_directories(roguebot PRIVATE include)
