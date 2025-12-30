#include "Attack.hpp"
#include <cmath>
#include <iostream>

static bool eq(const IVec2 &a, const IVec2 &b) {
  return a.x == b.x && a.y == b.y;
}

static int check(const char *name, Vector2 in, IVec2 expected) {
  IVec2 got = facingFromDir(in);
  if (!eq(got, expected)) {
    std::cerr << "[FAIL] " << name << " in=(" << in.x << "," << in.y << ")"
              << " expected=(" << expected.x << "," << expected.y << ")"
              << " got=(" << got.x << "," << got.y << ")\n";
    return 1;
  }
  return 0;
}

int main() {
  int fails = 0;

  fails += check("x_dom_pos", {5.f, 2.f}, {1, 0});
  fails += check("x_dom_neg", {-5.f, 2.f}, {-1, 0});

  fails += check("y_dom_down", {1.f, 7.f}, {0, 1});
  fails += check("y_dom_up", {1.f, -7.f}, {0, -1});

  fails += check("tie_pos", {3.f, 3.f}, {1, 0});
  fails += check("tie_left", {-3.f, 3.f}, {-1, 0});

  if (fails == 0) {
    std::cout << "[OK] facingFromDir: todas las comprobaciones pasan.\n";
    return 0;
  }
  return 1;
}