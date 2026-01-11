#define BOOST_TEST_MODULE rb_test_map_exit_tile_is_exit
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_exit_tile_is_exit)

BOOST_AUTO_TEST_CASE(map_exit_tile_is_exit)
{
  Map m;

  struct Case {
    int W;
    int H;
    unsigned seed;
  };
  const Case cases[] = {
      {40, 25, 1u}, {20, 15, 42u}, {60, 30, 12345u}, {10, 10, 0u}};

  for (const auto& c : cases) {
    m.generate(c.W, c.H, c.seed);

    auto [ex, ey] = m.findExitTile();

    BOOST_REQUIRE_MESSAGE(!(ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()),
                          "findExitTile fuera de rango: " << ex << "," << ey
                          << " en generate(" << c.W << "," << c.H << "," << c.seed << ")");

    BOOST_REQUIRE_MESSAGE(m.at(ex, ey) == EXIT,
                          "La celda devuelta por findExitTile no es EXIT\n"
                          << "  pos: (" << ex << "," << ey
                          << ")  tile=" << int(m.at(ex, ey)));
  }
}

BOOST_AUTO_TEST_SUITE_END()
