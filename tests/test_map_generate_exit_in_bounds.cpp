#define BOOST_TEST_MODULE rb_test_map_generate_exit_in_bounds
#include <boost/test/unit_test.hpp>

#include "core/Map.hpp"

BOOST_AUTO_TEST_SUITE(map_generate_exit_in_bounds)

BOOST_AUTO_TEST_CASE(map_generate_exit_in_bounds)
{
  Map map;

  struct Case {
    int W;
    int H;
    unsigned seed;
  };

  const Case cases[] = {
      {40, 25, 1u}, {20, 15, 42u}, {60, 30, 12345u}, {10, 10, 0u}};

  for (const auto& c : cases) {
    map.generate(c.W, c.H, c.seed);

    auto [ex, ey] = map.findExitTile();

    BOOST_REQUIRE_MESSAGE(!(ex < 0 || ex >= c.W || ey < 0 || ey >= c.H),
                          "findExitTile fuera de rango tras generate(" << c.W
                          << "," << c.H << "," << c.seed << "). "
                          << "Exit=(" << ex << "," << ey << "), "
                          << "Rangos: x[0," << (c.W - 1) << "], y[0," << (c.H - 1)
                          << "]");
  }
}

BOOST_AUTO_TEST_SUITE_END()
