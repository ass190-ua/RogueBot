#define BOOST_TEST_MODULE rb_test_locales_exist
#include <boost/test/unit_test.hpp>

#include <filesystem>

BOOST_AUTO_TEST_SUITE(Locales_Assets)

BOOST_AUTO_TEST_CASE(mo_files_exist_and_not_empty) {
  namespace fs = std::filesystem;

  const fs::path base = fs::path("assets") / "locales";
  const fs::path es = base / "es_ES" / "LC_MESSAGES" / "roguebot.mo";
  const fs::path en = base / "en_GB" / "LC_MESSAGES" / "roguebot.mo";

  BOOST_REQUIRE_MESSAGE(fs::exists(es), "No existe: " << es.string());
  BOOST_REQUIRE_MESSAGE(fs::exists(en), "No existe: " << en.string());

  BOOST_TEST(fs::file_size(es) > 0);
  BOOST_TEST(fs::file_size(en) > 0);
}

BOOST_AUTO_TEST_SUITE_END()
