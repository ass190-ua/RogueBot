#pragma once
#include <algorithm>

namespace rb {
inline float EaseOutCubic(float x) {
  x = std::clamp(x, 0.0f, 1.0f);
  float a = 1.0f - x;
  return 1.0f - a * a * a;
}
} // namespace rb
