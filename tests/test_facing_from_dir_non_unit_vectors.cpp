#define BOOST_TEST_MODULE rb_test_facing_from_dir_non_unit_vectors
#include <boost/test/unit_test.hpp>

#include "Attack.hpp"

static bool eq(const IVec2& a, const IVec2& b) {
  return a.x == b.x && a.y == b.y;
}

BOOST_AUTO_TEST_SUITE(facing_from_dir_non_unit_vectors)

BOOST_AUTO_TEST_CASE(facing_from_dir_non_unit_vectors)
{
  auto check = [](const char* name, Vector2 in, IVec2 expected) {
    IVec2 got = facingFromDir(in);
    BOOST_REQUIRE_MESSAGE(eq(got, expected),
                          name << " in=(" << in.x << "," << in.y << ")"
                               << " expected=(" << expected.x << "," << expected.y << ")"
                               << " got=(" << got.x << "," << got.y << ")");
  };

  check("non_unit_right", {2.f, 0.f}, {1, 0});
  check("non_unit_left", {-3.f, 0.f}, {-1, 0});
  check("non_unit_down", {0.f, 5.f}, {0, 1});
  check("non_unit_up", {0.f, -7.f}, {0, -1});

  check("x_dominant_big_pos", {10.f, 1.f}, {1, 0});
  check("x_dominant_big_neg", {-10.f, 2.f}, {-1, 0});
  check("y_dominant_big_down", {1.f, 10.f}, {0, 1});
  check("y_dominant_big_up", {2.f, -10.f}, {0, -1});
}

BOOST_AUTO_TEST_SUITE_END()
