#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  Map m;
  m.generateBossArena(10, 10);

  // Fuera de rango
  if (m.isWalkable(-1, 0)) return fail("isWalkable(-1,0) debería ser false");
  if (m.isWalkable(0, -1)) return fail("isWalkable(0,-1) debería ser false");
  if (m.isWalkable(10, 0)) return fail("isWalkable(10,0) debería ser false");
  if (m.isWalkable(0, 10)) return fail("isWalkable(0,10) debería ser false");

  // En boss arena hay padding=2 de muros, el borde (0,0) debe ser WALL => no walkable
  if (m.isWalkable(0, 0)) return fail("isWalkable(0,0) debería ser false (WALL)");

  // Interior (2,2) debería ser FLOOR => walkable
  if (!m.isWalkable(2, 2)) return fail("isWalkable(2,2) debería ser true (FLOOR)");

  std::cout << "[OK] Map::isWalkable (límites + WALL/FLOOR)\n";
  return 0;
}
