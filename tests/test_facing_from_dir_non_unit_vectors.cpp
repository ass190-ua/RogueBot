#include "Attack.hpp"
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

  fails += check("non_unit_right", {2.f, 0.f}, {1, 0});
  fails += check("non_unit_left", {-3.f, 0.f}, {-1, 0});
  fails += check("non_unit_down", {0.f, 5.f}, {0, 1});
  fails += check("non_unit_up", {0.f, -7.f}, {0, -1});

  fails += check("x_dominant_big_pos", {10.f, 1.f}, {1, 0});
  fails += check("x_dominant_big_neg", {-10.f, 2.f}, {-1, 0});
  fails += check("y_dominant_big_down", {1.f, 10.f}, {0, 1});
  fails += check("y_dominant_big_up", {2.f, -10.f}, {0, -1});

  if (fails == 0) {
    std::cout << "[OK] facingFromDir: acepta vectores no unitarios.\n";
    return 0;
  }
  return 1;
}
