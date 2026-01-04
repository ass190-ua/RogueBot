#define BOOST_TEST_MODULE rb_test_map_deterministic_seed
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_deterministic_seed)

BOOST_AUTO_TEST_CASE(map_deterministic_seed)
{
  const int W = 40;
  const int H = 25;
  const unsigned seed = 12345;

  Map a;
  Map b;
  a.generate(W, H, seed);
  b.generate(W, H, seed);

  BOOST_REQUIRE_MESSAGE(a.width() == b.width() && a.height() == b.height(),
                        "dimensiones distintas con misma seed");

  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      BOOST_REQUIRE_MESSAGE(a.at(x, y) == b.at(x, y),
                            "tile distinto en (" << x << "," << y << ")\n"
                            << "  a=" << int(a.at(x,y)) << " b=" << int(b.at(x,y)));
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
