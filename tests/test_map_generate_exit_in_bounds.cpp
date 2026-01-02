#include <cstdlib>
#include <iostream>

#include "core/Map.hpp"

int main() {
  Map map;

  struct Case {
    int W;
    int H;
    unsigned seed;
  };

  const Case cases[] = {
      {40, 25, 1u}, {20, 15, 42u}, {60, 30, 12345u}, {10, 10, 0u}};

  for (const auto &c : cases) {
    map.generate(c.W, c.H, c.seed);

    auto [ex, ey] = map.findExitTile();

    if (ex < 0 || ex >= c.W || ey < 0 || ey >= c.H) {
      std::cerr << "[FAIL] findExitTile fuera de rango tras generate(" << c.W
                << "," << c.H << "," << c.seed << "). "
                << "Exit=(" << ex << "," << ey << "), "
                << "Rangos: x[0," << (c.W - 1) << "], y[0," << (c.H - 1)
                << "]\n";
      return EXIT_FAILURE;
    }
  }

  std::cout << "[OK] findExitTile devuelve coordenadas válidas (in-bounds)\n";
  return EXIT_SUCCESS;
}
