#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  const int W = 60;
  const int H = 35;
  const unsigned seed = 123; // determinista

  Map m;
  m.generate(W, H, seed);

  auto [ex, ey] = m.findExitTile();

  if (ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()) {
    std::cerr << "[FAIL] findExitTile fuera de rango: " << ex << "," << ey << "\n";
    return 1;
  }

  if (m.at(ex, ey) != EXIT) {
    std::cerr << "[FAIL] La celda devuelta por findExitTile no es EXIT\n";
    std::cerr << "  pos: (" << ex << "," << ey << ")  tile=" << int(m.at(ex, ey)) << "\n";
    return 1;
  }

  std::cout << "[OK] Map::generate coloca EXIT y findExitTile lo localiza\n";
  return 0;
}
