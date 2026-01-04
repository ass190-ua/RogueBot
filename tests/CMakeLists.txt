cmake_policy(SET CMP0167 OLD)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

function(rb_link_boost_test target)
  target_link_libraries(${target} PRIVATE Boost::unit_test_framework)
  # Ayuda en muchas distros cuando Boost.Test está como librería separada
  target_compile_definitions(${target} PRIVATE BOOST_TEST_DYN_LINK)
endfunction()


# Test: Comprobar que los archivos de localización existen
add_executable(rb_test_locales_exist
  test_locales_exist.cpp
)
rb_link_boost_test(rb_test_locales_exist)

add_test(NAME locales_exist COMMAND rb_test_locales_exist)

set_tests_properties(locales_exist PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "assets"
)





# Test: Comprobar que assetPath funciona correctamente
add_executable(rb_test_assetpath
  test_assetpath.cpp
)

rb_link_boost_test(rb_test_assetpath)

target_include_directories(rb_test_assetpath PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

target_compile_definitions(rb_test_assetpath PRIVATE RB_ASSET_ROOT="${ASSET_ROOT}")

add_test(NAME assetpath_exists COMMAND rb_test_assetpath)

set_tests_properties(assetpath_exists PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "assets"
)




# Test: Comprobar función dominantAxis
add_executable(rb_test_dominant_axis
  test_dominant_axis.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

rb_link_boost_test(rb_test_dominant_axis)

target_include_directories(rb_test_dominant_axis PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_dominant_axis PRIVATE raylib)
endif()

add_test(NAME dominant_axis COMMAND rb_test_dominant_axis)
set_tests_properties(dominant_axis PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
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
  LABELS "utils"
)




## Text: Comprobar traducciones con gettext
find_library(RB_INTL_LIB intl)

add_executable(rb_test_gettext_translation
  test_gettext_translation.cpp
)

rb_link_boost_test(rb_test_gettext_translation)


if(RB_INTL_LIB)
  target_link_libraries(rb_test_gettext_translation PRIVATE ${RB_INTL_LIB})
endif()

add_test(NAME gettext_translation COMMAND rb_test_gettext_translation)

set_tests_properties(gettext_translation PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  ENVIRONMENT "LANG=es_ES.UTF-8;LC_ALL=es_ES.UTF-8;LANGUAGE=es"
)
set_property(TEST gettext_translation PROPERTY LABELS i18n)
target_include_directories(rb_test_gettext_translation PRIVATE ${ROGUEBOT_INCLUDE_DIRS})




# Test: Comprobar función computeMeleeTiles con frontOnly = true
add_executable(rb_test_compute_melee_front
  test_compute_melee_tiles_front.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)
rb_link_boost_test(rb_test_compute_melee_front)

target_include_directories(rb_test_compute_melee_front PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_front PRIVATE raylib)
endif()

add_test(NAME compute_melee_front COMMAND rb_test_compute_melee_front)

set_tests_properties(compute_melee_front PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)




# Test: Comprobar función computeMeleeTiles con frontOnly = false
add_executable(rb_test_compute_melee_cross
  test_compute_melee_tiles_cross.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

rb_link_boost_test(rb_test_compute_melee_cross)

target_include_directories(rb_test_compute_melee_cross PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_cross PRIVATE raylib)
endif()

add_test(NAME compute_melee_cross COMMAND rb_test_compute_melee_cross)

set_tests_properties(compute_melee_cross PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)




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

set_tests_properties(compute_melee_occluded_front PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)




# Test
add_executable(rb_test_facing_from_dir
  ${CMAKE_CURRENT_SOURCE_DIR}/test_facing_from_dir.cpp
)

target_include_directories(rb_test_facing_from_dir PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_facing_from_dir PRIVATE raylib)
endif()

add_test(NAME facing_from_dir COMMAND rb_test_facing_from_dir)

set_tests_properties(facing_from_dir PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)



# Test: Comprobar función PerformMeleeHit
add_executable(rb_test_perform_melee_hit
  ${CMAKE_CURRENT_SOURCE_DIR}/test_perform_melee_hit.cpp
)

rb_link_boost_test(rb_test_perform_melee_hit)

target_include_directories(rb_test_perform_melee_hit PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_perform_melee_hit PRIVATE raylib)
endif()

add_test(NAME perform_melee_hit COMMAND rb_test_perform_melee_hit)

set_tests_properties(perform_melee_hit PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)



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
  LABELS "map"
)


# Test: findExitTile devuelve una celda válida
add_executable(rb_test_find_exit_tile
  test_find_exit_tile.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

rb_link_boost_test(rb_test_find_exit_tile)

target_include_directories(rb_test_find_exit_tile PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_find_exit_tile PRIVATE raylib)
endif()

add_test(NAME find_exit_tile COMMAND rb_test_find_exit_tile)

set_tests_properties(find_exit_tile PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Map::isWalkable (límites + WALL/FLOOR)
add_executable(rb_test_map_is_walkable
  test_map_is_walkable.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)
rb_link_boost_test(rb_test_map_is_walkable)

target_include_directories(rb_test_map_is_walkable PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_is_walkable PRIVATE raylib)
endif()

add_test(NAME map_is_walkable COMMAND rb_test_map_is_walkable)

set_tests_properties(map_is_walkable PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
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
  LABELS "map"
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
  LABELS "map"
)


# Test: Map::setTile respeta límites
add_executable(rb_test_map_set_tile_bounds
  test_map_set_tile_bounds.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_set_tile_bounds PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_set_tile_bounds PRIVATE raylib)
endif()

add_test(NAME map_set_tile_bounds COMMAND rb_test_map_set_tile_bounds)

set_tests_properties(map_set_tile_bounds PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Comprobar que Map::generate fija correctamente width/height
add_executable(rb_test_map_generate_sets_dimensions
  test_map_generate_sets_dimensions.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

rb_link_boost_test(rb_test_map_generate_sets_dimensions)

target_include_directories(rb_test_map_generate_sets_dimensions PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_generate_sets_dimensions PRIVATE raylib)
endif()

add_test(NAME map_generate_sets_dimensions COMMAND rb_test_map_generate_sets_dimensions)

set_tests_properties(map_generate_sets_dimensions PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Comprobar que findExitTile siempre devuelve coordenadas válidas (in-bounds)
add_executable(rb_test_map_generate_exit_in_bounds
  test_map_generate_exit_in_bounds.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_generate_exit_in_bounds PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_generate_exit_in_bounds PRIVATE raylib)
endif()

add_test(NAME map_generate_exit_in_bounds COMMAND rb_test_map_generate_exit_in_bounds)

set_tests_properties(map_generate_exit_in_bounds PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Comprobar que findExitTile apunta a un tile EXIT
add_executable(rb_test_map_exit_tile_is_exit
  test_map_exit_tile_is_exit.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_exit_tile_is_exit PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_exit_tile_is_exit PRIVATE raylib)
endif()

add_test(NAME map_exit_tile_is_exit COMMAND rb_test_map_exit_tile_is_exit)

set_tests_properties(map_exit_tile_is_exit PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Comprobar que Map::generate crea tiles FLOOR suficientes
add_executable(rb_test_map_generate_creates_some_floor
  test_map_generate_creates_some_floor.cpp
  ${PROJECT_SOURCE_DIR}/src/core/Map.cpp
)

target_include_directories(rb_test_map_generate_creates_some_floor PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_map_generate_creates_some_floor PRIVATE raylib)
endif()

add_test(NAME map_generate_creates_some_floor COMMAND rb_test_map_generate_creates_some_floor)

set_tests_properties(map_generate_creates_some_floor PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "map"
)


# Test: Fijar comportamiento de dominantAxis en empate abs(dx)==abs(dy)
add_executable(rb_test_dominant_axis_tie_behavior
  test_dominant_axis_tie_behavior.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_dominant_axis_tie_behavior PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_dominant_axis_tie_behavior PRIVATE raylib)
endif()

add_test(NAME dominant_axis_tie_behavior COMMAND rb_test_dominant_axis_tie_behavior)

set_tests_properties(dominant_axis_tie_behavior PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)


# Test: computeMeleeTiles con rango 0 devuelve vacío
add_executable(rb_test_compute_melee_tiles_range_zero
  test_compute_melee_tiles_range_zero.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_compute_melee_tiles_range_zero PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_tiles_range_zero PRIVATE raylib)
endif()

add_test(NAME compute_melee_tiles_range_zero COMMAND rb_test_compute_melee_tiles_range_zero)

set_tests_properties(compute_melee_tiles_range_zero PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)


# Test: computeMeleeTiles maneja rango negativo sin romper
add_executable(rb_test_compute_melee_tiles_negative_range
  test_compute_melee_tiles_negative_range.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_compute_melee_tiles_negative_range PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_compute_melee_tiles_negative_range PRIVATE raylib)
endif()

add_test(NAME compute_melee_tiles_negative_range COMMAND rb_test_compute_melee_tiles_negative_range)

set_tests_properties(compute_melee_tiles_negative_range PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)


# Test: facingFromDir acepta vectores no unitarios
add_executable(rb_test_facing_from_dir_non_unit_vectors
  ${CMAKE_CURRENT_SOURCE_DIR}/test_facing_from_dir_non_unit_vectors.cpp
)

target_include_directories(rb_test_facing_from_dir_non_unit_vectors PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

if(TARGET raylib)
  target_link_libraries(rb_test_facing_from_dir_non_unit_vectors PRIVATE raylib)
endif()

add_test(NAME facing_from_dir_non_unit_vectors COMMAND rb_test_facing_from_dir_non_unit_vectors)

set_tests_properties(facing_from_dir_non_unit_vectors PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "utils"
)


# Test: Comprobar que existen catálogos .mo en es_ES y en_GB
add_executable(rb_test_i18n_mo_files_exist_en_es
  test_i18n_mo_files_exist_en_es.cpp
)
rb_link_boost_test(rb_test_i18n_mo_files_exist_en_es)

add_test(NAME i18n_mo_files_exist_en_es COMMAND rb_test_i18n_mo_files_exist_en_es)

set_tests_properties(i18n_mo_files_exist_en_es PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "i18n"
)


# Test: Comprobar traducción con gettext en es_ES usando una clave real del .mo
add_executable(rb_test_gettext_translation_es_es
  test_gettext_translation_es_es.cpp
)

rb_link_boost_test(rb_test_gettext_translation_es_es)


if(RB_INTL_LIB)
  target_link_libraries(rb_test_gettext_translation_es_es PRIVATE ${RB_INTL_LIB})
endif()

target_include_directories(rb_test_gettext_translation_es_es PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

add_test(NAME gettext_translation_es_es COMMAND rb_test_gettext_translation_es_es)

set_tests_properties(gettext_translation_es_es PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  ENVIRONMENT "LANG=es_ES.UTF-8;LC_ALL=es_ES.UTF-8;LC_MESSAGES=es_ES.UTF-8;LANGUAGE=es_ES"
)
set_property(TEST gettext_translation_es_es PROPERTY LABELS i18n)


# Test: assetPath resuelve correctamente un recurso existente conocido
add_executable(rb_test_assetpath_known_file_exists
  test_assetpath_known_file_exists.cpp
)
rb_link_boost_test(rb_test_assetpath_known_file_exists)


target_include_directories(rb_test_assetpath_known_file_exists PRIVATE ${ROGUEBOT_INCLUDE_DIRS})
target_compile_definitions(rb_test_assetpath_known_file_exists PRIVATE RB_ASSET_ROOT="${ASSET_ROOT}")

add_test(NAME assetpath_known_file_exists COMMAND rb_test_assetpath_known_file_exists)

set_tests_properties(assetpath_known_file_exists PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  LABELS "assets"
)
