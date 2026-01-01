#pragma once
#include "Game.hpp" // Necesario para IVec2 y definiciones de Game::EnemyFacing
#include "raylib.h"
#include <vector>

// Tabla de balanceo (Constantes de combate)
// Usamos 'inline constexpr' (C++17) para definir constantes globales
// directamente en el header sin problemas de duplicación al compilar.

// Bando Enemigo
inline constexpr int ENEMY_BASE_HP = 100;            // Vida estándar (tanky)
inline constexpr int ENEMY_CONTACT_DMG = 1;          // Daño al tocar al jugador (1 corazón)
inline constexpr float ENEMY_ATTACK_COOLDOWN = 1.5f; // Los enemigos atacan 1 vez por segundo

// Bando Jugador: Puños (Default)
// DPS bajo, alto riesgo (hay que acercarse mucho)
inline constexpr int DMG_HANDS = 20;     // Requiere 5 golpes para matar a un enemigo base
inline constexpr float CD_HANDS = 0.40f; // Muy rápido

// Bando Jugador: Espada (Melee Mejorado)
// Escalado de daño por Tier. Recompensa la exploración.
inline constexpr int DMG_SWORD_T1 = 30;  // 4 golpes para matar (100 HP)
inline constexpr int DMG_SWORD_T2 = 40;  // 3 golpes
inline constexpr int DMG_SWORD_T3 = 50;  // 2 golpes (Power spike grande)
inline constexpr float CD_SWORD = 0.60f; // Más lento que puños (da sensación de peso)

// Bando Jugador: Plasma (Rango)
// Combate seguro a distancia, pero lento.
inline constexpr int DMG_PLASMA = 40;             // Daño sólido
inline constexpr float CD_PLASMA = 0.80f;         // Cadencia baja (tipo cañón)
inline constexpr float PLASMA_SPEED = 300.0f;     // Velocidad del proyectil en px/s
inline constexpr float PLASMA_RANGE_TILES = 6.5f; // Alcance máximo antes de disiparse

// Estado de ejecución del ataque (Runtime)
// Esta estructura actúa como una "Máquina de Estados" pequeña para el combate.
struct AttackRuntime {
    // Configuración (Stats del arma actual)
    int   rangeTiles = 1;     // Alcance en casillas (1 = Daga, 2 = Lanza/Látigo)
    float cooldown   = 0.20f; // Tiempo mínimo entre ataques
    float swingTime  = 0.10f; // Duración visual del "flash" de ataque (Hitbox activa)

    // Estado Dinámico (Cambia frame a frame)
    float cdTimer    = 0.f;   // Cuenta atrás para poder atacar de nuevo
    float swingTimer = 0.f;   // Cuenta atrás de la duración del golpe actual
    bool  swinging   = false; // true = La hitbox está activa ahora mismo

    // Lógica de Apuntado
    // false = Ataque en área (tipo Bomberman, 4 direcciones a la vez)
    // true  = Ataque direccional (tipo Zelda, solo al frente)
    bool  frontOnly  = true;

    // "Memoria" de dirección: Si el jugador suelta las teclas (vector 0,0),
    // recordamos hacia dónde miraba para atacar en esa dirección.
    IVec2 lastDir    = {0, 1};

    // Debug / Render: Guardamos qué casillas fueron golpeadas en el último frame
    // para dibujar los efectos visuales o los cuadrados rojos de depuración.
    std::vector<IVec2> lastTiles;
};

// Importante: Declaramos la variable global, pero se debería definir en el .cpp
// (ej: en GameUtils.cpp poner: "AttackRuntime gAttack;")
extern AttackRuntime gAttack;

// Utilidades geométricas y de IA
// Convierte un vector libre (ej: joystick analógico) a una de las 4 direcciones cardinales.
// Prioriza el eje con mayor magnitud.
IVec2 dominantAxis(IVec2 d);

// Cálculo de Hitboxes (Sin tener en cuenta paredes)
// Útil para armas "fantasmas" o efectos de área.
std::vector<IVec2> computeMeleeTiles(IVec2 center, IVec2 lastDir,
                                     int range, bool frontOnly);

// Cálculo de Hitboxes (Con Oclusión)
// "Corta" el ataque si encuentra una pared (Map::isWalkable == false).
// Esto evita que el jugador mate enemigos a través de los muros (anti-cheese).
std::vector<IVec2> computeMeleeTilesOccluded(IVec2 center, IVec2 lastDir,
                                             int range, bool frontOnly,
                                             const Map& map);

// Chequeo rápido de vecindad (Cruz de Von Neumann)
// Devuelve true si (bx,by) está justo Arriba, Abajo, Izq o Der de (ax,ay).
bool isAdjacent4(int ax, int ay, int bx, int by);

// Convierte el Enum de estado del enemigo a vector matemático (ej: Up -> {0, -1})
IVec2 facingToDir(Game::EnemyFacing f);
