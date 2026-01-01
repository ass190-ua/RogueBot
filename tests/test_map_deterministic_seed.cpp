#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  const int W = 40;
  const int H = 25;
  const unsigned seed = 12345;

  Map a;
  Map b;
  a.generate(W, H, seed);
  b.generate(W, H, seed);

  if (a.width() != b.width() || a.height() != b.height()) {
    return fail("dimensiones distintas con misma seed");
  }

  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      if (a.at(x, y) != b.at(x, y)) {
        std::cerr << "[FAIL] tile distinto en (" << x << "," << y << ")\n";
        std::cerr << "  a=" << int(a.at(x,y)) << " b=" << int(b.at(x,y)) << "\n";
        return 1;
      }
    }
  }

  std::cout << "[OK] Map::generate es determinista con la misma seed\n";
  return 0;
}
