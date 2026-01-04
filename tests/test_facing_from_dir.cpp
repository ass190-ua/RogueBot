#define BOOST_TEST_MODULE rb_test_facing_from_dir
#include <boost/test/unit_test.hpp>

#include "Attack.hpp"

static bool eq(const IVec2& a, const IVec2& b) {
  return a.x == b.x && a.y == b.y;
}

BOOST_AUTO_TEST_SUITE(facing_from_dir)

BOOST_AUTO_TEST_CASE(facing_from_dir_cases)
{
  auto check = [](const char* name, Vector2 in, IVec2 expected) {
    IVec2 got = facingFromDir(in);
    BOOST_REQUIRE_MESSAGE(eq(got, expected),
                          name << " in=(" << in.x << "," << in.y << ")"
                               << " expected=(" << expected.x << "," << expected.y << ")"
                               << " got=(" << got.x << "," << got.y << ")");
  };

  check("x_dom_pos", {5.f, 2.f}, {1, 0});
  check("x_dom_neg", {-5.f, 2.f}, {-1, 0});

  check("y_dom_down", {1.f, 7.f}, {0, 1});
  check("y_dom_up", {1.f, -7.f}, {0, -1});

  check("tie_pos", {3.f, 3.f}, {1, 0});
  check("tie_left", {-3.f, 3.f}, {-1, 0});
}

BOOST_AUTO_TEST_SUITE_END()
