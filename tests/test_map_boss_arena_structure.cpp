#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  const int W = 12;
  const int H = 12;

  Map m;
  m.generateBossArena(W, H);

  if (m.width() != W || m.height() != H) {
    return fail("generateBossArena no respeta dimensiones");
  }

  // Esquina: debería ser WALL
  if (m.at(0, 0) != WALL) return fail("esquina (0,0) debería ser WALL");

  // Zona interior a partir de padding=2: debería ser FLOOR
  if (m.at(2, 2) != FLOOR) return fail("interior (2,2) debería ser FLOOR");

  // Centro del mapa: debería ser FLOOR
  if (m.at(W/2, H/2) != FLOOR) return fail("centro debería ser FLOOR");

  std::cout << "[OK] generateBossArena crea borde WALL e interior FLOOR\n";
  return 0;
}
