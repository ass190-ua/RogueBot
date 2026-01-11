#define BOOST_TEST_MODULE rb_test_assetpath
#include <boost/test/unit_test.hpp>

#include "AssetPath.hpp"
#include <filesystem>

BOOST_AUTO_TEST_SUITE(assetpath)

BOOST_AUTO_TEST_CASE(assetpath_exists)
{
    namespace fs = std::filesystem;

    const std::string rel = "locales/es_ES/LC_MESSAGES/roguebot.mo";
    const std::string abs = assetPath(rel);

    BOOST_REQUIRE_MESSAGE(fs::exists(abs),
                          "assetPath no devuelve un fichero existente\n  rel: " << rel << "\n  abs: " << abs);

    BOOST_REQUIRE_MESSAGE(fs::file_size(abs) != 0,
                          "El fichero existe pero está vacío\n  abs: " << abs);
}

BOOST_AUTO_TEST_SUITE_END()
