// Evita que Boost.Test use setcolor (choca con macros de raylib)
#define BOOST_TEST_NO_COLOR
#define BOOST_TEST_MODULE RogueBot_FloatsHUD
#include <boost/test/included/unit_test.hpp>

// raylib define macros tipo BLACK/RED/GREEN/YELLOW que chocan con Boost,
// así que los anulamos DESPUÉS de Boost y ANTES de incluir HUD.
#ifdef BLACK
#undef BLACK
#endif
#ifdef RED
#undef RED
#endif
#ifdef GREEN
#undef GREEN
#endif
#ifdef YELLOW
#undef YELLOW
#endif
#ifdef BLUE
#undef BLUE
#endif
#ifdef MAGENTA
#undef MAGENTA
#endif
#ifdef WHITE
#undef WHITE
#endif

#include "systems/HUD.hpp"

BOOST_AUTO_TEST_SUITE(floats_hud)

BOOST_AUTO_TEST_CASE(ease_out_cubic_valores_limite) {
  // f(0) = 0
  BOOST_CHECK_SMALL(HUD::EaseOutCubic(0.0f), 1e-6f);

  // f(1) = 1 (tolerancia en porcentaje)
  BOOST_CHECK_CLOSE(HUD::EaseOutCubic(1.0f), 1.0f, 1e-4f);
}

BOOST_AUTO_TEST_CASE(ease_out_cubic_monotona_en_rango) {
  const float f25 = HUD::EaseOutCubic(0.25f);
  const float f50 = HUD::EaseOutCubic(0.50f);
  const float f75 = HUD::EaseOutCubic(0.75f);

  // Debe ser creciente en [0, 1]
  BOOST_TEST(f25 < f50);
  BOOST_TEST(f50 < f75);

  // Debe mantenerse en [0, 1]
  BOOST_TEST(f25 >= 0.0f);
  BOOST_TEST(f25 <= 1.0f);

  BOOST_TEST(f50 >= 0.0f);
  BOOST_TEST(f50 <= 1.0f);

  BOOST_TEST(f75 >= 0.0f);
  BOOST_TEST(f75 <= 1.0f);
}

BOOST_AUTO_TEST_SUITE_END()
