#define BOOST_TEST_MODULE rb_test_i18n_mo_files_exist_en_es
#include <boost/test/unit_test.hpp>

#include <filesystem>

BOOST_AUTO_TEST_SUITE(I18n_MoFiles)

BOOST_AUTO_TEST_CASE(mo_files_exist) {
  namespace fs = std::filesystem;

  const fs::path enMo = "assets/locales/en_GB/LC_MESSAGES/roguebot.mo";
  const fs::path esMo = "assets/locales/es_ES/LC_MESSAGES/roguebot.mo";

  BOOST_TEST(fs::exists(enMo));
  BOOST_TEST(fs::exists(esMo));
}

BOOST_AUTO_TEST_SUITE_END()
