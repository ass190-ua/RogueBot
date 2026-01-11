#define BOOST_TEST_MODULE rb_test_dominant_axis
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"

static bool eq(const IVec2& a, const IVec2& b) {
  return a.x == b.x && a.y == b.y;
}

BOOST_AUTO_TEST_SUITE(dominant_axis)

BOOST_AUTO_TEST_CASE(dominant_axis_cases)
{
  auto check = [](const char* name, IVec2 in, IVec2 expected) {
    IVec2 out = dominantAxis(in);
    BOOST_REQUIRE_MESSAGE(eq(out, expected),
                          name << " in=(" << in.x << "," << in.y << ")"
                               << " expected=(" << expected.x << "," << expected.y << ")"
                               << " got=(" << out.x << "," << out.y << ")");
  };

  check("horizontal_pos", {5, 2}, {1, 0});
  check("horizontal_neg", {-5, 2}, {-1, 0});
  check("vertical_pos", {1, 7}, {0, 1});
  check("vertical_neg", {1, -7}, {0, -1});

  check("tie_pos", {3, 3}, {1, 0});
  check("tie_left", {-3, 3}, {-1, 0});

  check("pure_up", {0, -2}, {0, -1});
  check("pure_down", {0, 2}, {0, 1});

  check("zero_vector", {0, 0}, {1, 0});
}

BOOST_AUTO_TEST_SUITE_END()
