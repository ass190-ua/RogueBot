
# Test: Comprobar que los archivos de localización existen
add_executable(rb_test_locales_exist
  test_locales_exist.cpp
)

add_test(NAME locales_exist COMMAND rb_test_locales_exist)

set_tests_properties(locales_exist PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: Comprobar que assetPath funciona correctamente
add_executable(rb_test_assetpath
  test_assetpath.cpp
)

target_include_directories(rb_test_assetpath PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

target_compile_definitions(rb_test_assetpath PRIVATE RB_ASSET_ROOT="${ASSET_ROOT}")

add_test(NAME assetpath_exists COMMAND rb_test_assetpath)

set_tests_properties(assetpath_exists PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: Comprobar función dominantAxis
add_executable(rb_test_dominant_axis
  test_dominant_axis.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_dominant_axis PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_dominant_axis PRIVATE raylib)
endif()

add_test(NAME dominant_axis COMMAND rb_test_dominant_axis)
set_tests_properties(dominant_axis PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: Comprobar función isAdjacent4
add_executable(rb_test_is_adjacent4
  test_is_adjacent4.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_is_adjacent4 PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_is_adjacent4 PRIVATE raylib)
endif()

add_test(NAME is_adjacent4 COMMAND rb_test_is_adjacent4)
set_tests_properties(is_adjacent4 PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


## Text: Comprobar traducciones con gettext
find_library(RB_INTL_LIB intl)

add_executable(rb_test_gettext_translation
  test_gettext_translation.cpp
)

if(RB_INTL_LIB)
  target_link_libraries(rb_test_gettext_translation PRIVATE ${RB_INTL_LIB})
endif()

add_test(NAME gettext_translation COMMAND rb_test_gettext_translation)

set_tests_properties(gettext_translation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  ENVIRONMENT "LANG=es_ES.UTF-8;LC_ALL=es_ES.UTF-8;LANGUAGE=es"
)

target_include_directories(rb_test_gettext_translation PRIVATE ${ROGUEBOT_INCLUDE_DIRS})


# Test: Comprobar función computeMeleeTiles con frontOnly = true
add_executable(rb_test_compute_melee_front
  test_compute_melee_tiles_front.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_compute_melee_front PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_front PRIVATE raylib)
endif()

add_test(NAME compute_melee_front COMMAND rb_test_compute_melee_front)


# Test: Comprobar función computeMeleeTiles con frontOnly = false
add_executable(rb_test_compute_melee_cross
  test_compute_melee_tiles_cross.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_compute_melee_cross PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_cross PRIVATE raylib)
endif()

add_test(NAME compute_melee_cross COMMAND rb_test_compute_melee_cross)


# Test: Comprobar función computeMeleeTilesOccluded con frontOnly = true
add_executable(rb_test_compute_melee_occluded_front
  ${CMAKE_CURRENT_SOURCE_DIR}/test_compute_melee_tiles_occluded_front.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_compute_melee_occluded_front PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_occluded_front PRIVATE raylib)
endif()

add_test(NAME compute_melee_occluded_front COMMAND rb_test_compute_melee_occluded_front)


# Test
add_executable(rb_test_facing_from_dir
  ${CMAKE_CURRENT_SOURCE_DIR}/test_facing_from_dir.cpp
)

target_include_directories(rb_test_facing_from_dir PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_facing_from_dir PRIVATE raylib)
endif()

add_test(NAME facing_from_dir COMMAND rb_test_facing_from_dir)


# Test: Comprobar función PerformMeleeHit
add_executable(rb_test_perform_melee_hit
  ${CMAKE_CURRENT_SOURCE_DIR}/test_perform_melee_hit.cpp
)

target_include_directories(rb_test_perform_melee_hit PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_perform_melee_hit PRIVATE raylib)
endif()

add_test(NAME perform_melee_hit COMMAND rb_test_perform_melee_hit)

# Test: Map determinista con misma seed
add_executable(rb_test_map_deterministic_seed
  test_map_deterministic_seed.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_deterministic_seed PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_deterministic_seed PRIVATE raylib)
endif()

add_test(NAME map_deterministic_seed COMMAND rb_test_map_deterministic_seed)

set_tests_properties(map_deterministic_seed PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: findExitTile devuelve una celda válida
add_executable(rb_test_find_exit_tile
  test_find_exit_tile.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_find_exit_tile PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_find_exit_tile PRIVATE raylib)
endif()

add_test(NAME find_exit_tile COMMAND rb_test_find_exit_tile)

set_tests_properties(find_exit_tile PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: Map::isWalkable (límites + WALL/FLOOR)
add_executable(rb_test_map_is_walkable
  test_map_is_walkable.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_is_walkable PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_is_walkable PRIVATE raylib)
endif()

add_test(NAME map_is_walkable COMMAND rb_test_map_is_walkable)

set_tests_properties(map_is_walkable PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: generateBossArena (borde WALL e interior FLOOR)
add_executable(rb_test_map_boss_arena_structure
  test_map_boss_arena_structure.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_boss_arena_structure PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_boss_arena_structure PRIVATE raylib)
endif()

add_test(NAME map_boss_arena_structure COMMAND rb_test_map_boss_arena_structure)

set_tests_properties(map_boss_arena_structure PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


# Test: Map::generate coloca EXIT
add_executable(rb_test_map_generate_places_exit
  test_map_generate_places_exit.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_generate_places_exit PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_generate_places_exit PRIVATE raylib)
endif()

add_test(NAME map_generate_places_exit COMMAND rb_test_map_generate_places_exit)

set_tests_properties(map_generate_places_exit PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
