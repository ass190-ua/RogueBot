#include <cstdlib>
#include <iostream>

#include "core/Map.hpp"

int main() {
  Map map;

  const int W = 40;
  const int H = 25;
  const unsigned seed = 12345u;

  map.generate(W, H, seed);

  if (map.width() != W) {
    std::cerr << "[FAIL] width() incorrecto. Esperado=" << W
              << " Obtenido=" << map.width() << "\n";
    return EXIT_FAILURE;
  }

  if (map.height() != H) {
    std::cerr << "[FAIL] height() incorrecto. Esperado=" << H
              << " Obtenido=" << map.height() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "[OK] Map::generate fija correctamente width/height\n";
  return EXIT_SUCCESS;
}
