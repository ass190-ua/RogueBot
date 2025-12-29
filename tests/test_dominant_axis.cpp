#include "GameUtils.hpp"
#include <iostream>

static bool eq(const IVec2 &a, const IVec2 &b) {
  return a.x == b.x && a.y == b.y;
}

static int check(const char *name, IVec2 in, IVec2 expected) {
  IVec2 out = dominantAxis(in);
  if (!eq(out, expected)) {
    std::cerr << "[FAIL] " << name << " in=(" << in.x << "," << in.y << ")"
              << " expected=(" << expected.x << "," << expected.y << ")"
              << " got=(" << out.x << "," << out.y << ")\n";
    return 1;
  }
  return 0;
}

int main() {
  int fails = 0;

  fails += check("horizontal_pos", {5, 2}, {1, 0});
  fails += check("horizontal_neg", {-5, 2}, {-1, 0});
  fails += check("vertical_pos", {1, 7}, {0, 1});
  fails += check("vertical_neg", {1, -7}, {0, -1});

  fails += check("tie_pos", {3, 3}, {1, 0});
  fails += check("tie_left", {-3, 3}, {-1, 0});

  fails += check("pure_up", {0, -2}, {0, -1});
  fails += check("pure_down", {0, 2}, {0, 1});

  fails += check("zero_vector", {0, 0}, {1, 0});

  if (fails == 0) {
    std::cout << "[OK] dominantAxis: todas las comprobaciones pasan.\n";
    return 0;
  }
  return 1;
}
