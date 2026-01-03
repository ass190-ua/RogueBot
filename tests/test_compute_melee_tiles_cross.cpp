#define BOOST_TEST_MODULE rb_test_compute_melee_tiles_cross
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

BOOST_AUTO_TEST_SUITE(GameUtils_ComputeMeleeTiles_Cross)

BOOST_AUTO_TEST_CASE(cross_range2) {
  // Arrange
  IVec2 center{10, 10};
  std::vector<IVec2> exp{
      {11, 10}, {9, 10}, {10, 11}, {10, 9},
      {12, 10}, {8, 10}, {10, 12}, {10, 8}
  };

  // Act
  auto got = computeMeleeTiles(center, {7, 1}, 2, false);

  // Assert
  assertTilesEq(got, exp);
}

BOOST_AUTO_TEST_CASE(cross_range1) {
  // Arrange
  IVec2 center{0, 0};
  std::vector<IVec2> exp{{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  // Act
  auto got = computeMeleeTiles(center, {1, 0}, 1, false);

  // Assert
  assertTilesEq(got, exp);
}

BOOST_AUTO_TEST_SUITE_END()
