#pragma once
#include "Game.hpp"
#include <vector>

// Constantes de combate/movimiento
inline constexpr int   ENEMY_BASE_HP           = 100;   // % (0..100)
inline constexpr int   PLAYER_MELEE_DMG        = 25;    // daño del jugador
inline constexpr int   ENEMY_CONTACT_DMG       = 1;     // daño por contacto enemigo
inline constexpr float ENEMY_ATTACK_COOLDOWN   = 0.40f; // s entre golpes enemigo
inline constexpr int   RANGE_MELEE_HAND        = 1;
inline constexpr int   RANGE_SWORD             = 3;
inline constexpr int   RANGE_PISTOL            = 7;

// Estado del ataque melee
struct AttackRuntime {
    // parámetros
    int   rangeTiles = 1;      // 1 o 2 tiles de alcance
    float cooldown   = 0.20f;  // s entre golpes
    float swingTime  = 0.10f;  // s que dura el gesto/flash

    // estado runtime
    float cdTimer    = 0.f;
    float swingTimer = 0.f;
    bool  swinging   = false;

    // modo: false = cruz (4 direcciones); true = solo al frente
    bool  frontOnly  = true;

    // última dirección del jugador (discreta) para "frente"
    IVec2 lastDir    = {0, 1};

    // tiles golpeados este gesto (para debug/flash)
    std::vector<IVec2> lastTiles;
};

// Variable global (única) con el estado del ataque melee
extern AttackRuntime gAttack;

//Utilidades de melee y facing
IVec2 dominantAxis(IVec2 d);

// Genera tiles objetivo según modo y rango (sin oclusión)
std::vector<IVec2> computeMeleeTiles(IVec2 center, IVec2 lastDir,
                                     int range, bool frontOnly);

// Genera tiles objetivo CORTANDO por paredes/no walkables (con oclusión)
std::vector<IVec2> computeMeleeTilesOccluded(IVec2 center, IVec2 lastDir,
                                             int range, bool frontOnly,
                                             const Map& map);

// Adyacencia 4-neighbors
bool isAdjacent4(int ax, int ay, int bx, int by);

// Convierte EnemyFacing a vector discreto
IVec2 facingToDir(Game::EnemyFacing f);
