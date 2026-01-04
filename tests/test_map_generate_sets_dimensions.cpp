#define BOOST_TEST_MODULE rb_test_map_generate_sets_dimensions
#include <boost/test/unit_test.hpp>

#include "core/Map.hpp"

BOOST_AUTO_TEST_SUITE(map_generate_sets_dimensions)

BOOST_AUTO_TEST_CASE(map_generate_sets_dimensions)
{
  Map map;

  const int W = 40;
  const int H = 25;
  const unsigned seed = 12345u;

  map.generate(W, H, seed);

  BOOST_REQUIRE_MESSAGE(map.width() == W,
                        "width() incorrecto. Esperado=" << W << " Obtenido=" << map.width());

  BOOST_REQUIRE_MESSAGE(map.height() == H,
                        "height() incorrecto. Esperado=" << H << " Obtenido=" << map.height());
}

BOOST_AUTO_TEST_SUITE_END()
