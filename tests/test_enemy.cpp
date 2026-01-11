#define BOOST_TEST_MODULE test_enemy
#include <boost/test/unit_test.hpp>

#include "core/Enemy.hpp"
#include "core/Map.hpp"

BOOST_AUTO_TEST_CASE(enemy_in_chase_range_basic) {
  Enemy e(5, 5, Enemy::Melee);

  BOOST_CHECK(e.inChaseRange(6, 5, 32, 64));
  BOOST_CHECK(e.inChaseRange(7, 5, 32, 64));

  BOOST_CHECK(!e.inChaseRange(8, 5, 32, 64));
}

BOOST_AUTO_TEST_CASE(enemy_step_chase_moves_horizontally_when_clear) {
  Map map;
  map.generateTutorialMap(20, 20);

  Enemy e(5, 5, Enemy::Melee);
  e.stepChase(8, 5, map);

  BOOST_CHECK_EQUAL(e.getX(), 6);
  BOOST_CHECK_EQUAL(e.getY(), 5);
}

BOOST_AUTO_TEST_CASE(enemy_step_chase_slides_when_blocked) {
  Map map;
  map.generateTutorialMap(20, 20);

  map.setTile(6, 5, WALL);

  Enemy e(5, 5, Enemy::Melee);
  e.stepChase(8, 7, map);

  BOOST_CHECK_EQUAL(e.getX(), 5);
  BOOST_CHECK_EQUAL(e.getY(), 6);
}

BOOST_AUTO_TEST_CASE(enemy_animation_tilt_decays) {
  Enemy e(0, 0, Enemy::Melee);

  e.addTilt(30.0f);

  for (int i = 0; i < 10; ++i) {
    e.updateAnimation(0.016f);
  }

  float t1 = std::abs(e.getTilt());
  BOOST_CHECK(t1 > 0.0f);

  for (int i = 0; i < 120; ++i) {
    e.updateAnimation(0.016f);
  }

  float t2 = std::abs(e.getTilt());
  BOOST_CHECK(t2 < t1);
}
