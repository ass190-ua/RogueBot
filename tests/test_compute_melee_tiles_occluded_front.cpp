#include "GameUtils.hpp"
#include "Map.hpp"
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
  const IVec2 center{5, 5};

  {
    Map map;
    map.generateBossArena(10, 10);
    map.setTile(6, 5, WALL);

    auto got = computeMeleeTilesOccluded(center, {1, 0}, 3, true, map);
    std::vector<IVec2> exp{};
    fails += assertTiles("occluded_wall_immediate", got, exp);
  }

  {
    Map map;
    map.generateBossArena(10, 10);
    map.setTile(7, 5, WALL);

    auto got = computeMeleeTilesOccluded(center, {1, 0}, 3, true, map);
    std::vector<IVec2> exp{{6, 5}};
    fails += assertTiles("occluded_wall_at_2", got, exp);
  }

  if (fails == 0) {
    std::cout << "[OK] computeMeleeTilesOccluded (frontal)\n";
    return 0;
  }
  return 1;
}