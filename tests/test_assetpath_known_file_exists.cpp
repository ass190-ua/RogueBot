#define BOOST_TEST_MODULE rb_test_assetpath_known_file_exists
#include <boost/test/unit_test.hpp>

#include <string>
#include <sys/stat.h>

#if __has_include("AssetPath.hpp")
#include "AssetPath.hpp"
#elif __has_include("assetPath.hpp")
#include "assetPath.hpp"
#elif __has_include("Assets.hpp")
#include "Assets.hpp"
#elif __has_include("assets/AssetPath.hpp")
#include "assets/AssetPath.hpp"
#else
#error "No se encuentra el header de assetPath (por ejemplo AssetPath.hpp)."
#endif

static bool fileExists(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

BOOST_AUTO_TEST_SUITE(assetpath_known_file_exists)

BOOST_AUTO_TEST_CASE(assetpath_known_file_exists)
{
  const std::string rel = "locales/es_ES/LC_MESSAGES/roguebot.mo";
  const std::string abs = assetPath(rel);

  BOOST_REQUIRE_MESSAGE(!abs.empty(),
                        "assetPath devolvió string vacío para: " << rel);

  BOOST_REQUIRE_MESSAGE(fileExists(abs),
                        "assetPath no resuelve a un fichero existente\n  rel: " << rel << "\n  abs: " << abs);
}

BOOST_AUTO_TEST_SUITE_END()
