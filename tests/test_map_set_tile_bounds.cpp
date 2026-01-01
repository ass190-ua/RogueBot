#include "Map.hpp"
#include <iostream>

static int fail(const char* msg) {
  std::cerr << "[FAIL] " << msg << "\n";
  return 1;
}

int main() {
  Map m;
  m.generateBossArena(10, 10);

  // Dentro de rango: debe cambiar el tile
  m.setTile(5, 5, WALL);
  if (m.at(5, 5) != WALL) return fail("setTile dentro de rango no cambió el tile a WALL");

  m.setTile(5, 5, FLOOR);
  if (m.at(5, 5) != FLOOR) return fail("setTile dentro de rango no cambió el tile a FLOOR");

  // Fuera de rango: no debe romper ni cambiar tiles válidos
  // Guardamos un tile conocido y comprobamos que sigue igual tras llamadas fuera de rango
  Tile before = m.at(2, 2);
  m.setTile(-1, 0, WALL);
  m.setTile(0, -1, WALL);
  m.setTile(10, 0, WALL);
  m.setTile(0, 10, WALL);

  if (m.at(2, 2) != before) return fail("setTile fuera de rango ha modificado un tile válido");

  std::cout << "[OK] Map::setTile respeta límites (dentro/fuera de rango)\n";
  return 0;
}
