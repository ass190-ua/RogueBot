#define BOOST_TEST_MODULE rb_test_map_generate_creates_some_floor
#include <boost/test/unit_test.hpp>

#include "Map.hpp"

BOOST_AUTO_TEST_SUITE(map_generate_creates_some_floor)

BOOST_AUTO_TEST_CASE(map_generate_creates_some_floor)
{
  const int W = 60;
  const int H = 35;
  const unsigned seed = 123;

  Map m;
  m.generate(W, H, seed);

  int floorCount = 0;

  for (int y = 0; y < m.height(); ++y) {
    for (int x = 0; x < m.width(); ++x) {
      const Tile t = m.at(x, y);
      if (t == FLOOR)
        floorCount++;
    }
  }

  const int MIN_FLOOR = 10;

  BOOST_REQUIRE_MESSAGE(floorCount >= MIN_FLOOR,
                        "FLOOR insuficiente: " << floorCount << " (min=" << MIN_FLOOR << ")");
}

BOOST_AUTO_TEST_SUITE_END()
