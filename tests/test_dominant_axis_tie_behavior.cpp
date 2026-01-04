#define BOOST_TEST_MODULE rb_test_dominant_axis_tie_behavior
#include <boost/test/unit_test.hpp>

#include "GameUtils.hpp"

static bool eq(const IVec2& a, const IVec2& b) {
  return a.x == b.x && a.y == b.y;
}

BOOST_AUTO_TEST_SUITE(dominant_axis_tie_behavior)

BOOST_AUTO_TEST_CASE(dominant_axis_tie_behavior)
{
  auto check = [](const char* name, IVec2 in, IVec2 expected) {
    IVec2 out = dominantAxis(in);
    BOOST_REQUIRE_MESSAGE(eq(out, expected),
                          name << " in=(" << in.x << "," << in.y << ")"
                               << " expected=(" << expected.x << "," << expected.y << ")"
                               << " got=(" << out.x << "," << out.y << ")");
  };

  check("tie_q1", {3, 3}, {1, 0});
  check("tie_q2", {-3, 3}, {-1, 0});
  check("tie_q3", {-3, -3}, {-1, 0});
  check("tie_q4", {3, -3}, {1, 0});
}

BOOST_AUTO_TEST_SUITE_END()
