#define BOOST_TEST_MODULE rb_test_map_boss_arena_structure
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_boss_arena_structure)

BOOST_AUTO_TEST_CASE(map_boss_arena_structure)
{
  const int W = 12;
  const int H = 12;

  Map m;
  m.generateBossArena(W, H);

  BOOST_REQUIRE_MESSAGE(m.width() == W && m.height() == H,
                        "generateBossArena no respeta dimensiones");

  // Esquina: debería ser WALL
  BOOST_REQUIRE_MESSAGE(m.at(0, 0) == WALL,
                        "esquina (0,0) debería ser WALL");

  // Zona interior a partir de padding=2: debería ser FLOOR
  BOOST_REQUIRE_MESSAGE(m.at(2, 2) == FLOOR,
                        "interior (2,2) debería ser FLOOR");

  // Centro del mapa: debería ser FLOOR
  BOOST_REQUIRE_MESSAGE(m.at(W/2, H/2) == FLOOR,
                        "centro debería ser FLOOR");
}

BOOST_AUTO_TEST_SUITE_END()
