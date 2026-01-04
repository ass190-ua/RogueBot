#define BOOST_TEST_MODULE rb_test_compute_melee_occluded_front
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"
#include "Map.hpp"
#include <vector>

static bool eq(const IVec2& a, const IVec2& b) {
  return a.x == b.x && a.y == b.y;
}

static void assertTiles(const char* name,
                        const std::vector<IVec2>& got,
                        const std::vector<IVec2>& exp) {
  BOOST_REQUIRE_MESSAGE(got.size() == exp.size(),
                        name << " tamaño incorrecto\n"
                             << "  esperado: " << exp.size()
                             << "  obtenido: " << got.size());

  for (size_t i = 0; i < exp.size(); ++i) {
    BOOST_REQUIRE_MESSAGE(eq(got[i], exp[i]),
                          name << " mismatch en i=" << i << "\n"
                               << "  esperado: (" << exp[i].x << "," << exp[i].y << ")\n"
                               << "  obtenido: (" << got[i].x << "," << got[i].y << ")");
  }
}

BOOST_AUTO_TEST_SUITE(compute_melee_occluded_front)

BOOST_AUTO_TEST_CASE(compute_melee_occluded_front)
{
  const IVec2 center{5, 5};

  {
    Map map;
    map.generateBossArena(10, 10);
    map.setTile(6, 5, WALL);

    auto got = computeMeleeTilesOccluded(center, {1, 0}, 3, true, map);
    std::vector<IVec2> exp{};
    assertTiles("occluded_wall_immediate", got, exp);
  }

  {
    Map map;
    map.generateBossArena(10, 10);
    map.setTile(7, 5, WALL);

    auto got = computeMeleeTilesOccluded(center, {1, 0}, 3, true, map);
    std::vector<IVec2> exp{{6, 5}};
    assertTiles("occluded_wall_at_2", got, exp);
  }
}

BOOST_AUTO_TEST_SUITE_END()
