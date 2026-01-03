#include "GameUtils.hpp"
#include <iostream>
#include <vector>

int main() {
  int fails = 0;

  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {1, 0}, 0, true);
    if (!got.empty()) {
      std::cerr
          << "[FAIL] range_zero_front: esperado vector vacío, obtenido tamaño="
          << got.size() << "\n";
      fails++;
    }
  }

  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {0, 1}, 0, false);
    if (!got.empty()) {
      std::cerr
          << "[FAIL] range_zero_cross: esperado vector vacío, obtenido tamaño="
          << got.size() << "\n";
      fails++;
    }
  }

  if (fails == 0) {
    std::cout << "[OK] computeMeleeTiles: rango 0 devuelve vacío.\n";
    return 0;
  }
  return 1;
}
