#define BOOST_TEST_MODULE rb_test_perform_melee_hit
#include <boost/test/unit_test.hpp>

#include "Attack.hpp"
#include <vector>

// Enemigo dummy compatible con PerformMeleeHit (necesita pos, hp, alive)
struct DummyEnemy {
    Vector2 pos{};
    int hp = 1;
    bool alive = true;
};

static Vector2 tileCenter(int tx, int ty) {
    // Centro de la celda (para evitar bordes)
    return Vector2{ (tx + 0.5f) * TILE, (ty + 0.5f) * TILE };
}

static void reset(std::vector<DummyEnemy>& enemies) {
    for (auto& e : enemies) {
        e.hp = 1;
        e.alive = true;
    }
}

BOOST_AUTO_TEST_SUITE(perform_melee_hit)

BOOST_AUTO_TEST_CASE(perform_melee_hit)
{
    // Jugador en tile (5,5)
    const Vector2 playerPos = tileCenter(5,5);

    // Enemigos:
    // - delante:  (6,5)
    // - detrás:   (4,5)
    // - diagonal: (6,6)
    // - lejos:    (8,5)
    std::vector<DummyEnemy> enemies{
        { tileCenter(6,5), 1, true },
        { tileCenter(4,5), 1, true },
        { tileCenter(6,6), 1, true },
        { tileCenter(8,5), 1, true },
    };

    AttackSystem a;
    a.current.damage = 1;

    // Caso 1: Front, range=2, mirando a la derecha
    a.mode = AttackSystem::Front;
    a.current.rangeTiles = 2;
    a.lastDir = Vector2{1.f, 0.f};

    const int hits1 = PerformMeleeHit(a, playerPos, enemies);
    BOOST_REQUIRE_MESSAGE(hits1 == 1, "hits front esperado=1 obtenido=" << hits1);

    BOOST_REQUIRE_MESSAGE(enemies[0].alive == false, "front_front expected alive=false got alive=true");
    BOOST_REQUIRE_MESSAGE(enemies[1].alive == true,  "front_back expected alive=true got alive=false");
    BOOST_REQUIRE_MESSAGE(enemies[2].alive == true,  "front_diag expected alive=true got alive=false");
    BOOST_REQUIRE_MESSAGE(enemies[3].alive == true,  "front_far expected alive=true got alive=false");

    // Caso 2: Cross, range=1
    reset(enemies);

    a.mode = AttackSystem::Cross;
    a.current.rangeTiles = 1;

    const int hits2 = PerformMeleeHit(a, playerPos, enemies);
    BOOST_REQUIRE_MESSAGE(hits2 == 2, "hits cross esperado=2 obtenido=" << hits2);

    BOOST_REQUIRE_MESSAGE(enemies[0].alive == false, "cross_front expected alive=false got alive=true");
    BOOST_REQUIRE_MESSAGE(enemies[1].alive == false, "cross_back expected alive=false got alive=true");
    BOOST_REQUIRE_MESSAGE(enemies[2].alive == true,  "cross_diag expected alive=true got alive=false");
    BOOST_REQUIRE_MESSAGE(enemies[3].alive == true,  "cross_far expected alive=true got alive=false");
}

BOOST_AUTO_TEST_SUITE_END()
