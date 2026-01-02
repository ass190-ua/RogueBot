#include <iostream>

#include "Map.hpp"

int main() {
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

  if (floorCount < MIN_FLOOR) {
    std::cerr << "[FAIL] FLOOR insuficiente: " << floorCount
              << " (min=" << MIN_FLOOR << ")\n";
    return 1;
  }

  std::cout << "[OK] generate crea FLOOR suficiente: " << floorCount << "\n";
  return 0;
}
