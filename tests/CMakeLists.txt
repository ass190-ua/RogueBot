
## Primer test: Comprobar que los archivos de localización existen
add_executable(rb_test_locales_exist
  test_locales_exist.cpp
)

add_test(NAME locales_exist COMMAND rb_test_locales_exist)

set_tests_properties(locales_exist PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


## Segundo test: Comprobar que assetPath funciona correctamente
add_executable(rb_test_assetpath
  test_assetpath.cpp
)

target_include_directories(rb_test_assetpath PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

target_compile_definitions(rb_test_assetpath PRIVATE RB_ASSET_ROOT="${ASSET_ROOT}")

add_test(NAME assetpath_exists COMMAND rb_test_assetpath)

set_tests_properties(assetpath_exists PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


## Tercer test: Comprobar función dominantAxis
add_executable(rb_test_dominant_axis
  test_dominant_axis.cpp
  ${PROJECT_SOURCE_DIR}/src/core/GameUtils.cpp
)

target_include_directories(rb_test_dominant_axis PRIVATE ${ROGUEBOT_INCLUDE_DIRS})

add_test(NAME dominant_axis COMMAND rb_test_dominant_axis)
set_tests_properties(dominant_axis PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)