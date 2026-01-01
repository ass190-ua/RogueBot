#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  const int W = 40;
  const int H = 25;

  Map m;
  m.generate(W, H, 777);

  auto [ex, ey] = m.findExitTile();

  if (ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()) {
    std::cerr << "[FAIL] findExitTile devuelve coordenadas fuera de rango: "
              << ex << "," << ey << "\n";
    return 1;
  }

  Tile t = m.at(ex, ey);
  if (t == WALL) {
    std::cerr << "[FAIL] findExitTile apunta a un muro en: "
              << ex << "," << ey << "\n";
    return 1;
  }

  std::cout << "[OK] findExitTile devuelve una celda válida (" << ex << "," << ey << ")\n";
  return 0;
}
