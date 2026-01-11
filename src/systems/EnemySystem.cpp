#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// Renderizado de enemigos
void Game::drawEnemies() const {
  // Calculamos la vida máxima para la barra según la dificultad y el nivel.
  // En modo fácil: 60/80/100; en medio: 70/90/110; en difícil: 100/125/150.
  int maxHPLevel;
  switch (difficulty) {
  case Difficulty::Easy:
    maxHPLevel = (currentLevel == 1) ? 60 : (currentLevel == 2) ? 80 : 100;
    break;
  case Difficulty::Medium:
    maxHPLevel = (currentLevel == 1) ? 70 : (currentLevel == 2) ? 90 : 110;
    break;
  case Difficulty::Hard:
  default:
    // Fórmula original: 100 + 25*(nivel-1)
    maxHPLevel = ENEMY_BASE_HP + (currentLevel - 1) * 25;
    break;
  }
  float maxHP = static_cast<float>(maxHPLevel);

  auto visible = [&](int x, int y) {
    return !map.fogEnabled() || map.isVisible(x, y);
  };

  for (size_t i = 0; i < enemies.size(); ++i) {
    const auto &e = enemies[i];
    if (!visible(e.getX(), e.getY()))
      continue;

    const Texture2D *tex = nullptr;

    EnemyFacing face =
        (i < enemyFacing.size()) ? enemyFacing[i] : EnemyFacing::Down;

    // Alternamos frame 1 / frame 2 con el tiempo de animación del enemigo
    // (no depende de si se mueve o no, pero te asegura que NUNCA se quedará en
    // idle “por error de assets”)
    bool frame2 = ((int)std::floor(e.getAnimTime() * 4.0f) % 2) == 1;

    // Elegimos set según tipo
    const bool isEnemy2 = (e.getType() == Enemy::Shooter);

    if (!isEnemy2) {
      switch (face) {
      case EnemyFacing::Up:
        tex = frame2 ? &itemSprites.enemy1Up2 : &itemSprites.enemy1Up1;
        break;
      case EnemyFacing::Down:
        tex = frame2 ? &itemSprites.enemy1Down2 : &itemSprites.enemy1Down1;
        break;
      case EnemyFacing::Left:
        tex = frame2 ? &itemSprites.enemy1Left2 : &itemSprites.enemy1Left1;
        break;
      case EnemyFacing::Right:
        tex = frame2 ? &itemSprites.enemy1Right2 : &itemSprites.enemy1Right1;
        break;
      }
    } else {
      switch (face) {
      case EnemyFacing::Up:
        tex = frame2 ? &itemSprites.enemy2Up2 : &itemSprites.enemy2Up1;
        break;
      case EnemyFacing::Down:
        tex = frame2 ? &itemSprites.enemy2Down2 : &itemSprites.enemy2Down1;
        break;
      case EnemyFacing::Left:
        tex = frame2 ? &itemSprites.enemy2Left2 : &itemSprites.enemy2Left1;
        break;
      case EnemyFacing::Right:
        tex = frame2 ? &itemSprites.enemy2Right2 : &itemSprites.enemy2Right1;
        break;
      }
    }

    const float xpx = (float)(e.getX() * tileSize);
    const float ypx = (float)(e.getY() * tileSize);

    Color tint = WHITE;

    // 1. Respiración (Squash & Stretch)
    // Usamos el seno del tiempo para calcular una escala Y que oscila entre
    // 0.95 y 1.05
    float breathe = 1.0f + sinf(e.getAnimTime()) * 0.05f;

    // 2. Origen de rotación
    // Queremos que roten desde sus "pies" (centro-abajo), no desde la esquina
    // El centro del tile es (16, 16). Los pies son (16, 32).
    Vector2 origin = {(float)tileSize / 2.0f, (float)tileSize};

    // 3. Destino ajustado
    // Como rotamos desde los pies, la posición Y de destino debe ser la base
    // del tile
    Rectangle dest = {
        xpx + tileSize / 2.0f,    // Centro X
        ypx + tileSize,           // Pies Y
        (float)tileSize,          // Ancho
        (float)tileSize * breathe // Alto (Respirando)
    };

    if (tex && tex->id != 0) {
      Rectangle src{0, 0, (float)tex->width, (float)tex->height};
      // Usamos la rotación (tilt)
      DrawTexturePro(*tex, src, dest, origin, e.getTilt(), tint);
    } else {
      // Fallback a idle del tipo correspondiente
      const Texture2D &idle = (e.getType() == Enemy::Shooter)
                                  ? itemSprites.enemy2Idle
                                  : itemSprites.enemy1Idle;
      if (idle.id != 0) {
        Rectangle src{0, 0, (float)idle.width, (float)idle.height};
        DrawTexturePro(idle, src, dest, origin, e.getTilt(), tint);
      } else {
        e.draw(tileSize);
      }
    }

    // Flash blanco
    if (i < enemyFlashTimer.size() && enemyFlashTimer[i] > 0.0f) {
      // Dibujamos el cuadrado simple encima porque rotar un rectángulo sin
      // textura es complejo en Raylib simple Pero como es un flash rápido, no
      // se nota la discrepancia
      DrawRectangle((int)xpx, (int)ypx, tileSize, tileSize, Fade(WHITE, 0.7f));
    }

    // Barra de vida
    // Barra de vida
    if (i < enemyHP.size()) {
      const int w = tileSize, h = 4;
      const int x = static_cast<int>(xpx);
      const int y = static_cast<int>(ypx) - (h + 2);

      // Calculamos la fracción de vida usando maxHP como denominador
      int hpw = static_cast<int>(std::lround(w * (enemyHP[i] / maxHP)));
      hpw = std::clamp(hpw, 0, w);

      DrawRectangle(x, y, w, h, Color{60, 60, 60, 200});
      DrawRectangle(x, y, hpw, h, RED);
      DrawRectangleLines(x, y, w, h, BLACK);
    }
  }
}

// Lógica de ataque enemigo
void Game::enemyTryAttackFacing() {
  for (size_t i = 0; i < enemies.size(); ++i) {
    const int ex = enemies[i].getX();
    const int ey = enemies[i].getY();

    // 1. Comprobación de Adyacencia (Rango Melee)
    // Solo puede atacar si está en una casilla vecina (cruz, no diagonales)
    if (!isAdjacent4(ex, ey, px, py))
      continue;

    // 2. Comprobación de Orientación (Facing)
    // El enemigo debe estar mirando hacia el jugador para atacar.
    // Calculamos el vector de mirada del enemigo y el vector hacia el jugador.
    IVec2 edir = facingToDir(i < enemyFacing.size() ? enemyFacing[i]
                                                    : EnemyFacing::Down);
    int sdx = (px > ex) - (px < ex); // Dirección relativa X (-1, 0, 1)
    int sdy = (py > ey) - (py < ey); // Dirección relativa Y (-1, 0, 1)

    // Si los vectores no coinciden, el enemigo no ataca.
    // Esto permite al jugador esquivar moviéndose a la espalda del enemigo.
    if (edir.x != sdx || edir.y != sdy)
      continue;

    // 3. Comprobación de Cooldowns
    // - enemyAtkCD: El enemigo debe haber descansado de su último golpe.
    // - damageCooldown: El jugador no debe estar en periodo de invencibilidad
    // tras un golpe.
    if (i < enemyAtkCD.size() && enemyAtkCD[i] <= 0.0f &&
        damageCooldown <= 0.0f) {
      // Impacto
      takeDamage(ENEMY_CONTACT_DMG);

      // Reiniciar temporizadores
      enemyAtkCD[i] = ENEMY_ATTACK_COOLDOWN;
      damageCooldown =
          DAMAGE_COOLDOWN; // Otorga invencibilidad breve al jugador

      std::cout << "[EnemyAtk] Enemy at (" << ex << "," << ey
                << ") hit Player!\n";
    }
  }
}

// Generación de enemigos (Spawning)
void Game::spawnEnemiesForLevel() {
  enemies.clear();
  const int n = enemiesPerLevel(currentLevel); // Cantidad según dificultad
  const int minDistTiles =
      8; // Distancia de seguridad para no aparecer encima del jugador

  IVec2 spawnTile{px, py};
  auto [exitX, exitY] = map.findExitTile();
  IVec2 exitTile{exitX, exitY};

  // 1. Recolección de candidatos
  // Recorremos todo el mapa buscando tiles válidos (suelo, lejos del jugador y
  // salida).
  std::vector<IVec2> candidates;
  candidates.reserve(map.width() * map.height());

  for (int y = 0; y < map.height(); ++y) {
    for (int x = 0; x < map.width(); ++x) {
      if (!map.isWalkable(x, y))
        continue; // Debe ser suelo
      // No spawnear en inicio ni salida
      if ((x == spawnTile.x && y == spawnTile.y) ||
          (x == exitTile.x && y == exitTile.y))
        continue;

      // Distancia Manhattan mínima al jugador
      int manhattan = std::abs(x - spawnTile.x) + std::abs(y - spawnTile.y);
      if (manhattan < minDistTiles)
        continue;

      candidates.push_back({x, y});
    }
  }
  if (candidates.empty())
    return; // Mapa demasiado pequeño o lleno

  // 2. Selección aleatoria
  std::uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
  std::uniform_int_distribution<int> typeDist(0, 100); // Probabilidad 0-100

  // Lista de posiciones ocupadas para evitar superponer enemigos entre sí o con
  // items
  auto used = std::vector<IVec2>{spawnTile, exitTile};
  auto isUsed = [&](int x, int y) {
    for (auto &u : used)
      if (u.x == x && u.y == y)
        return true;
    for (auto &it : items)
      if (it.tile.x == x && it.tile.y == y)
        return true;
    return false;
  };

  for (int i = 0; i < n; ++i) {
    // Intentamos hasta 200 veces encontrar un hueco libre de la lista de
    // candidatos
    for (int tries = 0; tries < 200; ++tries) {
      const auto &p = candidates[dist(rng)];
      if (!isUsed(p.x, p.y)) {
        // Decidir tipo de enemigo
        // Nivel 1: 100% Melee
        // Nivel 2+: 30% Shooter
        Enemy::Type t = Enemy::Melee;
        if (currentLevel >= 2 && typeDist(rng) < 30) {
          t = Enemy::Shooter;
        }

        enemies.emplace_back(p.x, p.y, t); // Creamos enemigo con tipo
        used.push_back(p);
        break;
      }
    }
  }

  // 3. Escalado de dificultad (HP)
  // Ajustamos la vida base de los enemigos según la dificultad seleccionada.
  int hpForLevel;
  switch (difficulty) {
  case Difficulty::Easy:
    // Fácil: menos enemigos y vida reducida por nivel (60, 80, 100)
    hpForLevel = (currentLevel == 1) ? 60 : (currentLevel == 2) ? 80 : 100;
    break;
  case Difficulty::Medium:
    // Medio: vida intermedia (70, 90, 110)
    hpForLevel = (currentLevel == 1) ? 70 : (currentLevel == 2) ? 90 : 110;
    break;
  case Difficulty::Hard:
  default:
    // Difícil: comportamiento original (100, 125, 150)
    hpForLevel = ENEMY_BASE_HP + (currentLevel - 1) * 25;
    break;
  }

  // Inicializamos los vectores paralelos con los datos de los nuevos enemigos
  enemyFacing.assign(enemies.size(), EnemyFacing::Down);
  enemyHP.assign(enemies.size(), hpForLevel);
  enemyMaxHP.assign(enemies.size(), hpForLevel);
  enemyAtkCD.assign(enemies.size(), 0.0f);
  enemyShootCD.assign(enemies.size(), 0.0f); // Inicializar cooldown disparo
  enemyFlashTimer.assign(enemies.size(), 0.0f);
}