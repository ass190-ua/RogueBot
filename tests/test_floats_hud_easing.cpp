#define BOOST_TEST_NO_COLOR
#define BOOST_TEST_MODULE floats_hud_easing
#include <boost/test/unit_test.hpp>

#include "core/Easing.hpp"

BOOST_AUTO_TEST_CASE(ease_out_cubic_valores_limite) {
  BOOST_CHECK_SMALL(rb::EaseOutCubic(0.0f), 1e-6f);
  BOOST_CHECK_CLOSE(rb::EaseOutCubic(1.0f), 1.0f, 1e-4f);
}

BOOST_AUTO_TEST_CASE(ease_out_cubic_monotona_en_rango) {
  float f25 = rb::EaseOutCubic(0.25f);
  float f50 = rb::EaseOutCubic(0.50f);
  float f75 = rb::EaseOutCubic(0.75f);

  BOOST_TEST(f25 >= 0.0f);
  BOOST_TEST(f25 <= 1.0f);
  BOOST_TEST(f50 >= 0.0f);
  BOOST_TEST(f50 <= 1.0f);
  BOOST_TEST(f75 >= 0.0f);
  BOOST_TEST(f75 <= 1.0f);

  BOOST_TEST(f25 < f50);
  BOOST_TEST(f50 < f75);
}
