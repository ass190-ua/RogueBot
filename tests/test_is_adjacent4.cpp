#include "GameUtils.hpp"
#include <iostream>

static int expect(const char *name, bool got, bool expected) {
  if (got != expected) {
    std::cerr << "[FAIL] " << name
              << " expected=" << (expected ? "true" : "false")
              << " got=" << (got ? "true" : "false") << "\n";
    return 1;
  }
  return 0;
}

int main() {
  int fails = 0;

  fails += expect("right", isAdjacent4(0, 0, 1, 0), true);
  fails += expect("left", isAdjacent4(0, 0, -1, 0), true);
  fails += expect("down", isAdjacent4(0, 0, 0, 1), true);
  fails += expect("up", isAdjacent4(0, 0, 0, -1), true);

  fails += expect("diag1", isAdjacent4(0, 0, 1, 1), false);
  fails += expect("diag2", isAdjacent4(0, 0, -1, 1), false);

  fails += expect("same", isAdjacent4(2, 3, 2, 3), false);

  fails += expect("far_x", isAdjacent4(0, 0, 2, 0), false);
  fails += expect("far_y", isAdjacent4(0, 0, 0, 2), false);
  fails += expect("far_xy", isAdjacent4(0, 0, 2, 1), false);

  if (fails == 0) {
    std::cout << "[OK] isAdjacent4: todas las comprobaciones pasan.\n";
    return 0;
  }
  return 1;
}
