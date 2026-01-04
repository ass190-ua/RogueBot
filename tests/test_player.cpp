#define BOOST_TEST_MODULE test_player
#include <boost/test/unit_test.hpp>

#include "core/Player.hpp"

BOOST_AUTO_TEST_CASE(player_grid_position_getters) {
  Player p;
  p.setGridPos(3, 7);
  BOOST_CHECK_EQUAL(p.getX(), 3);
  BOOST_CHECK_EQUAL(p.getY(), 7);
}

BOOST_AUTO_TEST_CASE(player_direction_from_delta) {
  Player p;

  p.setDirectionFromDelta(1, 0);
  BOOST_CHECK(p.getDirection() == Direction::Right);

  p.setDirectionFromDelta(-1, 0);
  BOOST_CHECK(p.getDirection() == Direction::Left);

  p.setDirectionFromDelta(0, 1);
  BOOST_CHECK(p.getDirection() == Direction::Down);

  p.setDirectionFromDelta(0, -1);
  BOOST_CHECK(p.getDirection() == Direction::Up);
}

BOOST_AUTO_TEST_CASE(player_update_idle_resets_walk_state) {
  Player p;

  // Simulamos que se mueve un poquito
  p.update(0.2f, true);

  // y luego se para -> debe resetear
  p.update(0.01f, false);

  BOOST_CHECK_EQUAL(p.getWalkIndex(), 0);
  BOOST_CHECK_SMALL(p.getAnimTimer(), 1e-6f);
}

BOOST_AUTO_TEST_CASE(player_update_toggles_walk_index) {
  Player p;

  // animInterval por defecto: 0.12
  p.update(0.13f, true); // debería alternar a 1
  BOOST_CHECK_EQUAL(p.getWalkIndex(), 1);

  p.update(0.13f, true); // debería volver a 0
  BOOST_CHECK_EQUAL(p.getWalkIndex(), 0);
}
