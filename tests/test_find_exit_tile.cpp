#define BOOST_TEST_MODULE rb_test_find_exit_tile
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(find_exit_tile)

BOOST_AUTO_TEST_CASE(find_exit_tile_returns_valid_cell)
{
  const int W = 40;
  const int H = 25;

  Map m;
  m.generate(W, H, 777);

  auto [ex, ey] = m.findExitTile();

  BOOST_REQUIRE_MESSAGE(!(ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()),
                        "findExitTile devuelve coordenadas fuera de rango: " << ex << "," << ey);

  Tile t = m.at(ex, ey);
  BOOST_REQUIRE_MESSAGE(t != WALL,
                        "findExitTile apunta a un muro en: " << ex << "," << ey);
}

BOOST_AUTO_TEST_SUITE_END()
