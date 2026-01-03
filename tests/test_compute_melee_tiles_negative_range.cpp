#include "GameUtils.hpp"
#include <iostream>
#include <vector>

int main() {
  int fails = 0;

  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {1, 0}, -1, true);
    if (!got.empty()) {
      std::cerr
          << "[FAIL] neg_range_front: esperado vector vacío, obtenido tamaño="
          << got.size() << "\n";
      fails++;
    }
  }

  {
    IVec2 center{10, 10};

    auto got = computeMeleeTiles(center, {0, 1}, -5, false);
    if (!got.empty()) {
      std::cerr
          << "[FAIL] neg_range_cross: esperado vector vacío, obtenido tamaño="
          << got.size() << "\n";
      fails++;
    }
  }

  if (fails == 0) {
    std::cout << "[OK] computeMeleeTiles: rango negativo devuelve vacío sin "
                 "romper.\n";
    return 0;
  }
  return 1;
}
