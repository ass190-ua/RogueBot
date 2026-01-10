cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# RogueBot — Sistema de build con CMake (Multiplataforma: Desktop + Web)
# -----------------------------------------------------------------------------
project(roguebot
  VERSION 1.1
  DESCRIPTION "RogueBot — Roguelike con raylib"
  LANGUAGES CXX)

# Usar C++17 de forma obligatoria
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Opciones Generales ---
option(ENABLE_I18N "Activar i18n/gettext (libintl)" ON)
option(PREFER_RAYLIB_STATIC "Preferir enlazar raylib de forma estática" ON)
option(USE_EXTERNAL_RAYLIB "Usar raylib instalada en el sistema" OFF)

# Buscar la librería de internacionalización (si se activa)
if(ENABLE_I18N)
  find_package(Intl)
endif()

# Configuración de la raíz de assets
# EN WEB: Los assets se montan en la raíz virtual (/assets), así que RB_ASSET_ROOT debe ser "/"
if(EMSCRIPTEN)
  set(ASSET_ROOT "/" CACHE STRING "Root assets web" FORCE)
else()
  set(ASSET_ROOT "assets" CACHE STRING "Directorio raíz de los assets del juego")
endif()

# --- 2. Fuentes e Includes ---
file(GLOB_RECURSE ROGUEBOT_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")

set(ROGUEBOT_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/src/systems"
  "${PROJECT_SOURCE_DIR}/src/core"
)

# --- 3. Dependencia Raylib ---
if(USE_EXTERNAL_RAYLIB)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(RAYLIB QUIET raylib)
  endif()
  if(NOT RAYLIB_FOUND)
    find_package(raylib QUIET)
  endif()
  if(NOT RAYLIB_FOUND AND NOT raylib_FOUND)
    message(FATAL_ERROR "USE_EXTERNAL_RAYLIB activado pero no se encontró raylib.")
  endif()

elseif(EXISTS "${PROJECT_SOURCE_DIR}/raylib/CMakeLists.txt")
  message(STATUS "Usando raylib local.")
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  set(OPTION_CHECK_UPDATE OFF CACHE BOOL "" FORCE)
  
  if(PREFER_RAYLIB_STATIC)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  endif()
  
  add_subdirectory("${PROJECT_SOURCE_DIR}/raylib" raylib_build)

else()
  message(STATUS "Descargando raylib (FetchContent)...")
  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
  )
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  set(OPTION_CHECK_UPDATE OFF CACHE BOOL "" FORCE)
  
  if(PREFER_RAYLIB_STATIC)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  endif()
  
  FetchContent_MakeAvailable(raylib)
endif()

# --- 4. Ejecutable Principal ---
add_executable(${PROJECT_NAME} ${ROGUEBOT_SOURCES})

# --- 5. Configuración Específica WEB (EMSCRIPTEN) ---
if(EMSCRIPTEN)
  # Cambiar extensión a .html
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

  # Forzar que el archivo se llame "index" (index.html, index.js, etc.)
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")

  # Flags de enlazado para Web
  target_link_options(${PROJECT_NAME} PRIVATE
    "-sUSE_GLFW=3"
    "-sASYNCIFY"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFORCE_FILESYSTEM=1"
    # Empaquetar la carpeta local 'assets' dentro del sistema de archivos virtual
    "--preload-file" "${PROJECT_SOURCE_DIR}/assets@/assets" 
    # Usar shell por defecto de raylib (mejora ajuste de pantalla)
    "--shell-file" "${raylib_SOURCE_DIR}/src/shell.html"
  )
  
  # Definición para detectar plataforma en C++
  target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WEB)
  # Forzar OpenGL ES 2.0 en Web (Raylib lo necesita a veces explícitamente)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GRAPHICS_API_OPENGL_ES2)
endif()

# --- 6. Configuración General y Desktop ---

# Vincular Intl (Gettext) si corresponde
if(ENABLE_I18N AND Intl_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE Intl::Intl)
endif()

# Windows MinGW Static
if(WIN32 AND MINGW)
  target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Includes
target_include_directories(${PROJECT_NAME} PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

# Defines globales (ASSET_ROOT dinámico y flag I18N)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  RB_ASSET_ROOT="${ASSET_ROOT}"
  RB_ENABLE_I18N=$<BOOL:${ENABLE_I18N}>
)

# Icono Windows
if(WIN32)
  set(ROGUEBOT_RC "${PROJECT_SOURCE_DIR}/packaging/windows/roguebot.rc")
  target_sources(${PROJECT_NAME} PRIVATE ${ROGUEBOT_RC})
endif()

# Enlazar Raylib
if(USE_EXTERNAL_RAYLIB)
  if(TARGET raylib)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
  else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARIES})
  endif()
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

# Librerías del sistema Linux (EXCLUYENDO EMSCRIPTEN)
# Importante: AND NOT EMSCRIPTEN para que no intente enlazar X11 en la web
if(UNIX AND NOT APPLE AND PREFER_RAYLIB_STATIC AND NOT USE_EXTERNAL_RAYLIB AND NOT EMSCRIPTEN)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    m pthread dl rt
    X11 Xrandr Xi Xxf86vm Xinerama Xcursor GL)
endif()

# Librerías del sistema Windows
if(WIN32 AND PREFER_RAYLIB_STATIC AND NOT USE_EXTERNAL_RAYLIB)
  target_link_libraries(${PROJECT_NAME} PRIVATE winmm gdi32 opengl32)
endif()

# --- 7. Instalación y Empaquetado ---
include(GNUInstallDirs)

if(WIN32)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/ DESTINATION assets PATTERN ".*" EXCLUDE)
else()
  # En Linux/Web
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  # Solo instalar assets en share si NO es emscripten (en web van empotrados)
  if(NOT EMSCRIPTEN)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/assets
      PATTERN ".*" EXCLUDE)
    install(FILES ${PROJECT_SOURCE_DIR}/packaging/roguebot.desktop
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/packaging/icons/roguebot.png
      DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps)
  endif()
endif()

# Configuración CPack
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Roguebot")
set(CPACK_PACKAGE_CONTACT "Roguebot <info@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "RogueBot")
  set(CPACK_NSIS_DISPLAY_NAME "RogueBot")
  set(CPACK_NSIS_PACKAGE_NAME "RogueBot")
  set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "roguebot.exe")
  set(CPACK_NSIS_MENU_LINKS "roguebot.exe" "RogueBot")
  set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\RogueBot.lnk' '$INSTDIR\\\\roguebot.exe'")
  set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\RogueBot.lnk'")
else()
  set(CPACK_GENERATOR "DEB;TGZ")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Roguebot")
  set(CPACK_DEBIAN_PACKAGE_SECTION "games")
  if(USE_EXTERNAL_RAYLIB AND NOT PREFER_RAYLIB_STATIC)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libraylib5 (>= 5.0)")
  endif()
endif()

# --- 8. Tests ---
# Desactivar tests si estamos en Emscripten (no suelen correr bien en CI sin node)
if(NOT EMSCRIPTEN)
  enable_testing()
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()

# Comando personalizado para crear el ZIP web automáticamente tras compilar
if(EMSCRIPTEN)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar cfv "roguebot_web.zip" --format=zip
            "${CMAKE_BINARY_DIR}/index.html"
            "${CMAKE_BINARY_DIR}/index.js"
            "${CMAKE_BINARY_DIR}/index.wasm"
            "${CMAKE_BINARY_DIR}/index.data"
    COMMENT "Empaquetando version Web (index.html) en roguebot_web.zip..."
  )
endif()

include(CPack)