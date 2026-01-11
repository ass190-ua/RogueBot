#define BOOST_TEST_MODULE rb_test_compute_melee_tiles_front
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"
#include <vector>
#include <cstddef>

static void assertTilesEq(const std::vector<IVec2>& got, const std::vector<IVec2>& exp) {
  BOOST_REQUIRE_EQUAL(got.size(), exp.size());
  for (std::size_t i = 0; i < exp.size(); ++i) {
    BOOST_TEST(got[i].x == exp[i].x);
    BOOST_TEST(got[i].y == exp[i].y);
  }
}

BOOST_AUTO_TEST_SUITE(GameUtils_ComputeMeleeTiles_Front)

BOOST_AUTO_TEST_CASE(front_right) {
  // Arrange
  IVec2 center{10, 10};
  std::vector<IVec2> exp{{11, 10}, {12, 10}, {13, 10}};

  // Act
  auto got = computeMeleeTiles(center, {5, 1}, 3, true);

  // Assert
  assertTilesEq(got, exp);
}

BOOST_AUTO_TEST_CASE(front_fallback_down) {
  // Arrange
  IVec2 center{2, 2};
  std::vector<IVec2> exp{{2, 3}, {2, 4}};

  // Act
  auto got = computeMeleeTiles(center, {0, 0}, 2, true);

  // Assert
  assertTilesEq(got, exp);
}

BOOST_AUTO_TEST_SUITE_END()
