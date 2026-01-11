#define BOOST_TEST_MODULE rb_test_compute_melee_tiles_negative_range
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"

BOOST_AUTO_TEST_SUITE(compute_melee_tiles_negative_range)

BOOST_AUTO_TEST_CASE(compute_melee_tiles_negative_range)
{
  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {1, 0}, -1, true);
    BOOST_REQUIRE_MESSAGE(got.empty(),
                          "neg_range_front: esperado vector vacío, obtenido tamaño=" << got.size());
  }

  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {0, 1}, -5, false);
    BOOST_REQUIRE_MESSAGE(got.empty(),
                          "neg_range_cross: esperado vector vacío, obtenido tamaño=" << got.size());
  }
}

BOOST_AUTO_TEST_SUITE_END()
