#include "Attack.hpp"
#include <iostream>
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

static int assertAlive(const char* name, const DummyEnemy& e, bool expectedAlive)
{
    if (e.alive != expectedAlive) {
        std::cerr << "[FAIL] " << name
                  << " expected alive=" << (expectedAlive ? "true" : "false")
                  << " got alive=" << (e.alive ? "true" : "false") << "\n";
        return 1;
    }
    return 0;
}

static void reset(std::vector<DummyEnemy>& enemies) {
    for (auto& e : enemies) {
        e.hp = 1;
        e.alive = true;
    }
}

int main()
{
    int fails = 0;

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

    int hits1 = PerformMeleeHit(a, playerPos, enemies);
    if (hits1 != 1) {
        std::cerr << "[FAIL] hits front esperado=1 obtenido=" << hits1 << "\n";
        return 1;
    }

    fails += assertAlive("front_front", enemies[0], false); // muere
    fails += assertAlive("front_back",  enemies[1], true);
    fails += assertAlive("front_diag",  enemies[2], true);
    fails += assertAlive("front_far",   enemies[3], true);

    // Caso 2: Cross, range=1
    reset(enemies);

    a.mode = AttackSystem::Cross;
    a.current.rangeTiles = 1;

    int hits2 = PerformMeleeHit(a, playerPos, enemies);
    if (hits2 != 2) {
        std::cerr << "[FAIL] hits cross esperado=2 obtenido=" << hits2 << "\n";
        return 1;
    }

    fails += assertAlive("cross_front", enemies[0], false); // muere
    fails += assertAlive("cross_back",  enemies[1], false); // muere
    fails += assertAlive("cross_diag",  enemies[2], true);
    fails += assertAlive("cross_far",   enemies[3], true);

    if (fails == 0) {
        std::cout << "[OK] PerformMeleeHit: comportamiento esperado.\n";
        return 0;
    }
    return 1;
}