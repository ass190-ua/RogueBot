#define BOOST_TEST_MODULE rb_test_map_generate_places_exit
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_generate_places_exit)

BOOST_AUTO_TEST_CASE(map_generate_places_exit)
{
  const int W = 60;
  const int H = 35;
  const unsigned seed = 123; // determinista

  Map m;
  m.generate(W, H, seed);

  auto [ex, ey] = m.findExitTile();

  BOOST_REQUIRE_MESSAGE(!(ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()),
                        "findExitTile fuera de rango: " << ex << "," << ey);

  BOOST_REQUIRE_MESSAGE(m.at(ex, ey) == EXIT,
                        "La celda devuelta por findExitTile no es EXIT\n"
                        << "  pos: (" << ex << "," << ey << ")  tile=" << int(m.at(ex, ey)));
}

BOOST_AUTO_TEST_SUITE_END()
