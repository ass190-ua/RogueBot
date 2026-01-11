#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

// Definición de estado global
// En el .hpp pusimos 'extern AttackRuntime gAttack;', que solo "promete" que
// existe. Aquí es donde realmente reservamos la memoria para esa variable
// global.
AttackRuntime gAttack{};

// Matemáticas vectoriales
// Convierte un vector libre (ej: input analógico) en una dirección cardinal
// estricta. Compara las magnitudes absolutas de X e Y para ver cuál eje "gana".

IVec2 dominantAxis(IVec2 d) {
  if (std::abs(d.x) >= std::abs(d.y))
    return {(d.x >= 0) ? 1 : -1, 0}; // Gana Horizontal (Der/Izq)
  else
    return {0, (d.y >= 0) ? 1 : -1}; // Gana Vertical (Abajo/Arriba)
}

// Cálculo de áreas de efecto (AOE)

// Versión Simple: Ignora paredes (Ataque "Fantasma" o etéreo)
std::vector<IVec2> computeMeleeTiles(IVec2 center, IVec2 lastDir, int range,
                                     bool frontOnly) {
  if (range <= 0)
    return {};

  std::vector<IVec2> out;
  // Reservamos memoria anticipadamente para evitar re-allocations
  // (optimización)
  out.reserve(frontOnly ? range : range * 4);

  if (frontOnly) {
    // Modo Estocada: Una línea recta hacia donde miras
    IVec2 f = dominantAxis((lastDir.x == 0 && lastDir.y == 0) ? IVec2{0, 1}
                                                              : lastDir);
    for (int t = 1; t <= range; ++t)
      out.push_back({center.x + f.x * t, center.y + f.y * t});
  } else {
    // Modo Explosión: Cruz en 4 direcciones
    for (int t = 1; t <= range; ++t) {
      out.push_back({center.x + t, center.y}); // Der
      out.push_back({center.x - t, center.y}); // Izq
      out.push_back({center.x, center.y + t}); // Abajo
      out.push_back({center.x, center.y - t}); // Arriba
    }
  }
  return out;
}

// Versión Avanzada: con oclusión (Respeta paredes)
// Esta es la que usa la espada para no atravesar muros.
std::vector<IVec2> computeMeleeTilesOccluded(IVec2 center, IVec2 lastDir,
                                             int range, bool frontOnly,
                                             const Map &map) {
  if (range <= 0)
    return {};

  // Lambda auxiliar para chequear límites y colisiones
  auto walkable = [&](int x, int y) -> bool {
    return x >= 0 && y >= 0 && x < map.width() && y < map.height() &&
           map.isWalkable(x, y);
  };

  std::vector<IVec2> out;
  out.reserve(range * (frontOnly ? 1 : 4));

  // Lambda "Raycast": Avanza casilla a casilla en una dirección
  auto pushRay = [&](IVec2 dir) {
    for (int t = 1; t <= range; ++t) {
      int tx = center.x + dir.x * t;
      int ty = center.y + dir.y * t;

      // Lógica de corte:
      // Si encontramos una pared, el ataque se detiene aquí.
      // El 'break' impide que se añadan las casillas que están detrás de la
      // pared.
      if (!walkable(tx, ty))
        break;

      out.push_back({tx, ty});
    }
  };

  if (frontOnly) {
    // Calcula dirección segura (si lastDir es 0,0 forzamos Abajo)
    IVec2 f = dominantAxis((lastDir.x == 0 && lastDir.y == 0) ? IVec2{0, 1}
                                                              : lastDir);
    pushRay(f);
  } else {
    // Lanza 4 rayos independientes
    pushRay({1, 0});
    pushRay({-1, 0});
    pushRay({0, 1});
    pushRay({0, -1});
  }
  return out;
}

// Helpers de Adyacencia
// Comprueba si dos celdas son vecinas directas (Arriba/Abajo/Izq/Der)
// Matemáticamente: La suma de diferencias absolutas debe ser exactamente 1.
// (Distancia Manhattan == 1)
bool isAdjacent4(int ax, int ay, int bx, int by) {
  return (std::abs(ax - bx) + std::abs(ay - by)) == 1;
}

// Traductor de Enum a Vector Matemático
IVec2 facingToDir(Game::EnemyFacing f) {
  switch (f) {
  case Game::EnemyFacing::Up:
    return {0, -1};
  case Game::EnemyFacing::Down:
    return {0, 1};
  case Game::EnemyFacing::Left:
    return {-1, 0};
  case Game::EnemyFacing::Right:
    return {1, 0};
  }
  return {0, 1}; // Fallback
}
