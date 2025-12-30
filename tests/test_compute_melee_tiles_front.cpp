#include "GameUtils.hpp"
#include <iostream>
#include <vector>

static bool eq(const IVec2 &a, const IVec2 &b) {
  return a.x == b.x && a.y == b.y;
}

static int assertTiles(const char *name, const std::vector<IVec2> &got,
                       const std::vector<IVec2> &exp) {
  if (got.size() != exp.size()) {
    std::cerr << "[FAIL] " << name << " tamaño incorrecto\n";
    std::cerr << "  esperado: " << exp.size() << "  obtenido: " << got.size()
              << "\n";
    return 1;
  }
  for (size_t i = 0; i < exp.size(); ++i) {
    if (!eq(got[i], exp[i])) {
      std::cerr << "[FAIL] " << name << " mismatch en i=" << i << "\n";
      std::cerr << "  esperado: (" << exp[i].x << "," << exp[i].y << ")\n";
      std::cerr << "  obtenido: (" << got[i].x << "," << got[i].y << ")\n";
      return 1;
    }
  }
  return 0;
}

int main() {
  int fails = 0;

  {
    IVec2 center{10, 10};
    auto got = computeMeleeTiles(center, {5, 1}, 3, true);
    std::vector<IVec2> exp{{11, 10}, {12, 10}, {13, 10}};
    fails += assertTiles("front_right", got, exp);
  }

  {
    IVec2 center{2, 2};
    auto got = computeMeleeTiles(center, {0, 0}, 2, true);
    std::vector<IVec2> exp{{2, 3}, {2, 4}};
    fails += assertTiles("front_fallback_down", got, exp);
  }

  if (fails == 0) {
    std::cout << "[OK] computeMeleeTiles (frontal)\n";
    return 0;
  }
  return 1;
}