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
    auto got = computeMeleeTiles(center, {7, 1}, 2, false);

    std::vector<IVec2> exp{
        {11, 10}, {9, 10}, {10, 11}, {10, 9},
        {12, 10}, {8, 10}, {10, 12}, {10, 8}
    };

    fails += assertTiles("cross_range2", got, exp);
  }

  {
    IVec2 center{0, 0};
    auto got = computeMeleeTiles(center, {1, 0}, 1, false);

    std::vector<IVec2> exp{{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    fails += assertTiles("cross_range1", got, exp);
  }

  if (fails == 0) {
    std::cout << "[OK] computeMeleeTiles (cruz)\n";
    return 0;
  }
  return 1;
}