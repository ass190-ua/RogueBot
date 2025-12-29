add_executable(rb_test_locales_exist
  test_locales_exist.cpp
)

add_test(NAME locales_exist COMMAND rb_test_locales_exist)

# Para que "assets/..." exista, ejecutamos el test desde la raíz del repo
set_tests_properties(locales_exist PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
