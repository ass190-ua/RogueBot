#define BOOST_TEST_MODULE rb_test_map_set_tile_bounds
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_set_tile_bounds)

BOOST_AUTO_TEST_CASE(map_set_tile_bounds)
{
  Map m;
  m.generateBossArena(10, 10);

  // Dentro de rango: debe cambiar el tile
  m.setTile(5, 5, WALL);
  BOOST_REQUIRE_MESSAGE(m.at(5, 5) == WALL,
                        "setTile dentro de rango no cambió el tile a WALL");

  m.setTile(5, 5, FLOOR);
  BOOST_REQUIRE_MESSAGE(m.at(5, 5) == FLOOR,
                        "setTile dentro de rango no cambió el tile a FLOOR");

  // Fuera de rango: no debe romper ni cambiar tiles válidos
  Tile before = m.at(2, 2);
  m.setTile(-1, 0, WALL);
  m.setTile(0, -1, WALL);
  m.setTile(10, 0, WALL);
  m.setTile(0, 10, WALL);

  BOOST_REQUIRE_MESSAGE(m.at(2, 2) == before,
                        "setTile fuera de rango ha modificado un tile válido");
}

BOOST_AUTO_TEST_SUITE_END()
