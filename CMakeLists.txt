cmake_minimum_required(VERSION 3.16)
project(RogueBot LANGUAGES CXX)

# --- Toolchain ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warnings útiles
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
elseif (MSVC)
  add_compile_options(/W4)
endif()

# --- Fuentes: detectar automáticamente todos los .cpp de src/ ---
# CONFIGURE_DEPENDS hace que CMake reconfigure cuando cambian los contenidos de src/
file(GLOB_RECURSE RB_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(roguebot ${RB_SOURCES})

# Incluye cabeceras propias (no falla si la carpeta no existe)
target_include_directories(roguebot PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/vendor/include
)

# --- Raylib: intenta del sistema y si no, usa FetchContent (5.0) ---
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

target_link_libraries(roguebot PRIVATE raylib)

# Extras Linux (raylib suele arrastrar todo, pero por compatibilidad mantenemos)
if(UNIX AND NOT APPLE)
  target_link_libraries(roguebot PRIVATE m pthread dl rt X11)
endif()

# Carpeta de salida binarios en ./build/bin
set_target_properties(roguebot PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Definición para localizar assets en runtime si usáis rutas relativas
target_compile_definitions(roguebot PRIVATE RB_ASSET_ROOT="${CMAKE_SOURCE_DIR}/assets")
