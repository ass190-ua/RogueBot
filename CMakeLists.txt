cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# RogueBot — Sistema de build con CMake (Hito 3)
#
# Este CMakeLists.txt define una configuración de compilación
# multiplataforma para RogueBot (Linux y Windows).
#
# Sustituye al Makefile clásico y añade:
#  - Detección / descarga automática de la dependencia raylib.
#  - Opciones para elegir entre raylib externa o compilada desde código.
#  - Generación de paquetes de distribución (ZIP/NSIS en Windows,
#    DEB/TGZ en Linux) mediante CPack.
#
# Variables de configuración importantes (opcionales al invocar CMake):
#
#  - PREFER_RAYLIB_STATIC:
#       ON  → enlazar raylib de forma estática (por defecto).
#       OFF → enlazar raylib de forma dinámica (si está disponible).
#
#  - USE_EXTERNAL_RAYLIB:
#       ON  → buscar una raylib ya instalada en el sistema
#             (pkg-config, find_package(raylib)).
#       OFF → descargar y compilar raylib desde el repositorio oficial.
#
#  - ASSET_ROOT:
#       Ruta (relativa o absoluta) que se incrusta en el binario
#       como la constante RB_ASSET_ROOT, usada para localizar
#       la carpeta de assets en tiempo de ejecución.
#       Por defecto es "assets".
#
# Ejemplo de configuración (Release, raylib compilada desde código, assets/):
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release \
#         -DPREFER_RAYLIB_STATIC=ON \
#         -DUSE_EXTERNAL_RAYLIB=OFF \
#         -DASSET_ROOT=assets
#
# Tras configurar, se puede compilar y empaquetar así:
#
#   cmake --build build --config Release
#   cmake --build build --target package
# -----------------------------------------------------------------------------
project(roguebot
  VERSION 1.1
  DESCRIPTION "RogueBot — Roguelike con raylib"
  LANGUAGES CXX)

# Usar C++17 de forma obligatoria en todo el proyecto
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar la librería de internacionalización (necesaria para gettext)
option(ENABLE_I18N "Activar i18n/gettext (libintl)" ON)

if(ENABLE_I18N)
  find_package(Intl)
endif()

# Opciones de usuario para controlar cómo se obtiene/enlaza raylib
option(PREFER_RAYLIB_STATIC "Preferir enlazar raylib de forma estática" ON)
option(USE_EXTERNAL_RAYLIB "Usar raylib instalada en el sistema (pkg-config/find_package)" OFF)

# Configuración de la raíz de assets.
# Este valor se inyecta en el código como RB_ASSET_ROOT y el juego lo usa
# para localizar la carpeta de recursos (sprites, docs, etc.).
set(ASSET_ROOT "assets" CACHE STRING "Directorio raíz de los assets del juego")

# Recoger todos los .cpp de src/ recursivamente.
# CONFIGURE_DEPENDS hace que si añadimos/eliminamos fuentes, CMake regenere
# los archivos de build automáticamente (similar a un "find" en el Makefile).
file(GLOB_RECURSE ROGUEBOT_SOURCES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Directorios de inclusión donde están los .hpp del proyecto.
# Aquí añadimos src/ y subcarpetas relevantes (systems, core, etc.).
set(ROGUEBOT_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/src/systems"
  "${PROJECT_SOURCE_DIR}/src/core"
)

# -----------------------------------------------------------------------------
# Dependencia raylib
#
# Tres modos posibles:
#   1) USE_EXTERNAL_RAYLIB=ON  → buscar raylib instalada en el sistema
#      (pkg-config / find_package).
#   2) Carpeta local raylib/   → usar la copia incluida en el repositorio
#      (submódulo / vendor).
#   3) Si no hay nada de lo anterior → descargar raylib desde GitHub
#      con FetchContent.
# -----------------------------------------------------------------------------

if(USE_EXTERNAL_RAYLIB)
  # Modo 1: raylib instalada en el sistema (pkg-config / find_package)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(RAYLIB QUIET raylib)
  endif()

  if(NOT RAYLIB_FOUND)
    find_package(raylib QUIET)
  endif()

  if(NOT RAYLIB_FOUND AND NOT raylib_FOUND)
    message(FATAL_ERROR
      "USE_EXTERNAL_RAYLIB está a ON pero no se ha encontrado raylib.\n"
      "Instala raylib en el sistema o desactiva USE_EXTERNAL_RAYLIB para "
      "usar la copia local / descargarla desde código fuente.")
  endif()

elseif(EXISTS "${PROJECT_SOURCE_DIR}/raylib/CMakeLists.txt")
  # Modo 2: usar la raylib incluida en el propio repositorio (subcarpeta raylib/)
  message(STATUS "Usando raylib incluida en el repositorio (subcarpeta raylib/).")

  # Desactivar ejemplos/juegos y actualizaciones online en el subproyecto
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  set(OPTION_CHECK_UPDATE OFF CACHE BOOL "" FORCE)

  # Decidir si la librería será estática o compartida
  if(PREFER_RAYLIB_STATIC)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
  endif()

  # Añadir raylib como subdirectorio CMake
  add_subdirectory("${PROJECT_SOURCE_DIR}/raylib" raylib_build)

else()
  # Modo 3: no hay raylib externa ni local → usar FetchContent (GitHub)
  message(STATUS "Descargando raylib con FetchContent (no se ha encontrado raylib local).")
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

# -----------------------------------------------------------------------------
# Definición del ejecutable principal
#
# Creamos el binario "roguebot" a partir de las fuentes y lo enlazamos
# con raylib. Además configuramos includes, constantes de compilación,
# y librerías extra según plataforma.
# -----------------------------------------------------------------------------

# Crear el ejecutable a partir de todos los .cpp recogidos
add_executable(${PROJECT_NAME} ${ROGUEBOT_SOURCES})

# Vincular la librería Intl (Gettext) al ejecutable
if(ENABLE_I18N AND Intl_FOUND)
  target_link_libraries(${PROJECT_NAME} PRIVATE Intl::Intl)
endif()

# Forzar enlazado estático en Windows (MinGW)
if(WIN32 AND MINGW)
  target_link_options(${PROJECT_NAME} PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
  )
endif()

# Añadir los directorios de cabeceras (.hpp) al target
target_include_directories(${PROJECT_NAME}
  PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

# Inyectar la ruta de assets en el código como RB_ASSET_ROOT.
# En C++ se verá como una cadena, por ejemplo "assets".
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  RB_ASSET_ROOT="${ASSET_ROOT}"
  RB_ENABLE_I18N=$<BOOL:${ENABLE_I18N}>
)

# Recurso de icono para el ejecutable en Windows (.exe).
# El archivo .rc se compila con windres y embebe el icono .ico en el binario.
if(WIN32)
  set(ROGUEBOT_RC "${PROJECT_SOURCE_DIR}/packaging/windows/roguebot.rc")
  target_sources(${PROJECT_NAME} PRIVATE ${ROGUEBOT_RC})
endif()

# Enlazar con raylib según el modo seleccionado
if(USE_EXTERNAL_RAYLIB)
  # Si find_package(raylib) ha proporcionado un target "raylib", usamos ese.
  if(TARGET raylib)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
  else()
    # Si venimos de pkg-config, usamos las rutas y librerías que nos da.
    target_include_directories(${PROJECT_NAME} PRIVATE ${RAYLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYLIB_LIBRARIES})
  endif()
else()
  # En el modo FetchContent tenemos el target "raylib" del propio proyecto.
  target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

# En Linux, si raylib se construye estática y no es externa,
# hay que enlazar manualmente varias librerías del sistema
# (X11, GL, pthread, etc.).
if(UNIX AND NOT APPLE AND PREFER_RAYLIB_STATIC AND NOT USE_EXTERNAL_RAYLIB)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    m pthread dl rt
    X11 Xrandr Xi Xxf86vm Xinerama Xcursor GL X11)
endif()

# En Windows, si raylib es estática y no externa,
# hay que añadir las libs del sistema que raylib necesita.
if(WIN32 AND PREFER_RAYLIB_STATIC AND NOT USE_EXTERNAL_RAYLIB)
  target_link_libraries(${PROJECT_NAME} PRIVATE winmm gdi32 opengl32)
endif()

# -----------------------------------------------------------------------------
# Reglas de instalación
#
# Aquí definimos qué archivos se instalan y dónde.
# CPack (al generar paquetes .zip, .deb, etc.) utiliza estas reglas
# como referencia para estructurar el contenido del paquete.
#
#   - En Windows:
#       roguebot.exe
#       assets/...
#
#   - En Linux:
#       bin/roguebot
#       share/roguebot/assets/...
#       share/applications/roguebot.desktop
#       share/icons/hicolor/256x256/apps/roguebot.png
# -----------------------------------------------------------------------------

include(GNUInstallDirs)

if(WIN32)
  # En Windows queremos un ZIP “portable”: el exe en la raíz del paquete
  install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .)

  # Copiar toda la carpeta assets/ a la raíz del paquete (assets/...)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
    DESTINATION assets
    PATTERN ".*" EXCLUDE)

else()
  # En Linux (y otros UNIX), seguimos el esquema estándar de FHS:
  #   /usr/bin, /usr/share, etc.

  # Ejecutable en bin/ (normalmente /usr/local/bin)
  install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  # Assets en share/roguebot/assets (p.ej. /usr/local/share/roguebot/assets)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/assets
    PATTERN ".*" EXCLUDE)

  # Fichero .desktop para integrarlo en el menú de escritorio (solo no-Windows)
  install(FILES ${PROJECT_SOURCE_DIR}/packaging/roguebot.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

  # Icono de 256x256 para menús/lanzadores en escritorio (solo no-Windows)
  install(FILES ${PROJECT_SOURCE_DIR}/packaging/icons/roguebot.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps)
endif()

# -----------------------------------------------------------------------------
# Configuración de empaquetado (CPack)
#
# CPack genera paquetes a partir de las reglas de instalación anteriores.
#
#   - En Windows: ZIP portable + instalador NSIS (.exe)
#   - En Linux:   paquete DEB + tarball TGZ
#
# Aquí definimos metadatos del paquete (nombre, versión, contacto, etc.)
# y opciones específicas de cada generador.
# -----------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Roguebot")
set(CPACK_PACKAGE_CONTACT "Roguebot <info@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

if(WIN32)
  # En Windows queremos:
  #   - ZIP (portable)
  #   - NSIS (instalador gráfico .exe)
  set(CPACK_GENERATOR "ZIP;NSIS")

  # Carpeta por defecto dentro de Archivos de programa (ej. C:\Program Files\RogueBot)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "RogueBot")

  # Nombre que aparece en el instalador y en "Agregar o quitar programas"
  set(CPACK_NSIS_DISPLAY_NAME "RogueBot")
  set(CPACK_NSIS_PACKAGE_NAME "RogueBot")
  set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")

  # (Opcional) Icono del asistente de instalación, si se desea usar un .ico
  # set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/packaging/icons/roguebot.ico")

  # Indicar qué ejecutable se usará como icono instalado en el Menú Inicio
  set(CPACK_NSIS_INSTALLED_ICON_NAME "roguebot.exe")

  # Crear entrada en el Menú Inicio que apunta a roguebot.exe
  set(CPACK_NSIS_MENU_LINKS "roguebot.exe" "RogueBot")

  # Crear un acceso directo en el Escritorio al instalar
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$DESKTOP\\\\RogueBot.lnk' '$INSTDIR\\\\roguebot.exe'")

  # Borrar el acceso del Escritorio al desinstalar
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$DESKTOP\\\\RogueBot.lnk'")

else()
  # En Linux generamos:
  #   - Paquete .deb (para sistemas tipo Debian/Ubuntu)
  #   - Tarball .tar.gz (portátil para otros sistemas)
  set(CPACK_GENERATOR "DEB;TGZ")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Roguebot")
  set(CPACK_DEBIAN_PACKAGE_SECTION "games")

  # Si enlazamos dinámicamente contra una raylib externa, podemos declarar
  # la dependencia del paquete DEB (ajustar versión según el sistema).
  if(USE_EXTERNAL_RAYLIB AND NOT PREFER_RAYLIB_STATIC)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libraylib5 (>= 5.0)")
  endif()
endif()

# -----------------------------------------------------------------------------
# Tests (CTest)
# -----------------------------------------------------------------------------
enable_testing()
include(CTest)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Incluir CPack y activar los targets "package" / "package_source"
include(CPack)
