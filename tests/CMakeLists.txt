
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