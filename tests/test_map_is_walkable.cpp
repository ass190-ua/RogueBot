#define BOOST_TEST_MODULE rb_test_map_is_walkable
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

struct BossArenaFixture {
  Map m;

  BossArenaFixture() {
    // Arrange (setup compartido)
    m.generateBossArena(10, 10);
  }
};

BOOST_FIXTURE_TEST_SUITE(Map_IsWalkable, BossArenaFixture)

BOOST_AUTO_TEST_CASE(out_of_bounds_is_false) {
  // Act + Assert
  BOOST_TEST(!m.isWalkable(-1, 0));
  BOOST_TEST(!m.isWalkable(0, -1));
  BOOST_TEST(!m.isWalkable(10, 0));
  BOOST_TEST(!m.isWalkable(0, 10));
}

BOOST_AUTO_TEST_CASE(wall_is_not_walkable) {
  BOOST_TEST(!m.isWalkable(0, 0));
}

BOOST_AUTO_TEST_CASE(floor_is_walkable) {
  BOOST_TEST(m.isWalkable(2, 2));
}

BOOST_AUTO_TEST_SUITE_END()
