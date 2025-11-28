#pragma once
#include "Game.hpp" // Para IVec2 y dependencias
#include "raylib.h"
#include <vector>

// --- CONSTANTES DE COMBATE BALANCEADAS ---

// 0. ENEMIGOS (Melee básico enemigo)
inline constexpr int ENEMY_BASE_HP = 100;        // Vida inicial del enemigo
inline constexpr int ENEMY_CONTACT_DMG = 1;      // Daño que hace al tocarte (1 corazón)
inline constexpr float ENEMY_ATTACK_COOLDOWN = 1.0f; // Tiempo entre golpes del enemigo

// 1. MANOS (Melee básico Jugador)
inline constexpr int DMG_HANDS = 20;       // 5 golpes para matar
inline constexpr float CD_HANDS = 0.40f;   // Rápido

// 2. ESPADA (Melee fuerte Jugador)
inline constexpr int DMG_SWORD_T1 = 30;    // 4 golpes
inline constexpr int DMG_SWORD_T2 = 40;    // 3 golpe
inline constexpr int DMG_SWORD_T3 = 50;    // 2 golpe
inline constexpr float CD_SWORD = 0.60f;   // Un poco más lento que las manos

// 3. PLASMA (Rango Jugador)
inline constexpr int DMG_PLASMA = 40;      // 3 golpes (Tier 1)
inline constexpr float CD_PLASMA = 0.80f;  // Lento
inline constexpr float PLASMA_SPEED = 300.0f; // Pixeles por segundo
inline constexpr float PLASMA_RANGE_TILES = 6.5f;

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