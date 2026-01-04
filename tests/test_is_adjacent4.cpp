#define BOOST_TEST_MODULE rb_test_is_adjacent4
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"

BOOST_AUTO_TEST_SUITE(is_adjacent4)

BOOST_AUTO_TEST_CASE(is_adjacent4_cases)
{
  auto expect = [](const char* name, bool got, bool expected) {
    BOOST_REQUIRE_MESSAGE(got == expected,
                          name << " expected=" << (expected ? "true" : "false")
                               << " got=" << (got ? "true" : "false"));
  };

  expect("right", isAdjacent4(0, 0, 1, 0), true);
  expect("left", isAdjacent4(0, 0, -1, 0), true);
  expect("down", isAdjacent4(0, 0, 0, 1), true);
  expect("up", isAdjacent4(0, 0, 0, -1), true);

  expect("diag1", isAdjacent4(0, 0, 1, 1), false);
  expect("diag2", isAdjacent4(0, 0, -1, 1), false);

  expect("same", isAdjacent4(2, 3, 2, 3), false);

  expect("far_x", isAdjacent4(0, 0, 2, 0), false);
  expect("far_y", isAdjacent4(0, 0, 0, 2), false);
  expect("far_xy", isAdjacent4(0, 0, 2, 1), false);
}

BOOST_AUTO_TEST_SUITE_END()
