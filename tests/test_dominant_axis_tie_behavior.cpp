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

  fails += check("tie_q1", {3, 3}, {1, 0});
  fails += check("tie_q2", {-3, 3}, {-1, 0});
  fails += check("tie_q3", {-3, -3}, {-1, 0});
  fails += check("tie_q4", {3, -3}, {1, 0});

  if (fails == 0) {
    std::cout << "[OK] dominantAxis: empate abs(dx)==abs(dy) definido.\n";
    return 0;
  }
  return 1;
}
