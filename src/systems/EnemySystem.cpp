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
    // Lambda auxiliar: Verifica si una celda es visible para el jugador ( FOV, "Fog of War")
    auto visible = [&](int x, int y) { 
        return !map.fogEnabled() || map.isVisible(x, y); 
    };

    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto &e = enemies[i];
        
        // Si el enemigo está en la niebla, no se dibuja ni él ni su barra de vida
        if (!visible(e.getX(), e.getY())) continue;

        // 1. Selección de textura (sprite) Según dirección
        // Seleccionamos el sprite correcto basándonos en hacia dónde mira el enemigo
        const Texture2D *tex = nullptr;
        switch (i < enemyFacing.size() ? enemyFacing[i] : EnemyFacing::Down) {
            case EnemyFacing::Up:    tex = &itemSprites.enemyUp;   break;
            case EnemyFacing::Down:  tex = &itemSprites.enemyDown; break;
            case EnemyFacing::Left:  tex = &itemSprites.enemyLeft; break;
            case EnemyFacing::Right: tex = &itemSprites.enemyRight;break;
        }

        // Coordenadas en píxeles (Mundo)
        const float xpx = (float)(e.getX() * tileSize);
        const float ypx = (float)(e.getY() * tileSize);

        // Renderizado del Sprite
        if (tex && tex->id != 0) {
            // Caso ideal: Tenemos sprite específico direccional
            Rectangle src{0,0,(float)tex->width,(float)tex->height};
            Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
            Vector2 origin{0,0};
            DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
        }
        else if (itemSprites.enemy.id != 0) {
            // Fallback 1: Sprite genérico de enemigo (sin dirección)
            Rectangle src{0,0,(float)itemSprites.enemy.width,(float)itemSprites.enemy.height};
            Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
            Vector2 origin{0,0};
            DrawTexturePro(itemSprites.enemy, src, dst, origin, 0.0f, WHITE);
        }
        else {
            // Fallback 2: Primitiva geométrica (círculo/cuadrado) si no cargaron texturas
            e.draw(tileSize);
        }

        // 2. Efecto de daño (flash blanco)
        // Si el enemigo ha sido golpeado recientemente, dibujamos un recuadro blanco
        // semi-transparente encima. Esto da feedback visual de impacto inmediato.
        if (i < enemyFlashTimer.size() && enemyFlashTimer[i] > 0.0f) {
            DrawRectangle((int)xpx, (int)ypx, tileSize, tileSize, Fade(WHITE, 0.7f));
        }

        // 3. Barra de vida (hp bar)
        if (i < enemyHP.size()) {
            const int w = tileSize; // Ancho igual al tile
            const int h = 4;        // Alto de 4px
            const int x = (int)(e.getX() * tileSize);
            const int y = (int)(e.getY() * tileSize) - (h + 2); // Dibujada justo encima de la cabeza

            // Cálculo del ancho de la barra verde según % de vida restante
            int hpw = (int)std::lround(w * (enemyHP[i] / 100.0));
            hpw = std::clamp(hpw, 0, w);

            // Fondo gris oscuro (para ver cuánto falta)
            DrawRectangle(x, y, w, h, Color{60, 60, 60, 200}); 
            
            // Lógica de color dinámico (Gradiente):
            // 100% Vida -> Verde (R=0, G=255)
            // 50% Vida  -> Amarillo (R=127, G=127)
            // 0% Vida   -> Rojo (R=255, G=0)
            float pct = std::clamp(static_cast<float>(enemyHP[i]) / 100.0f, 0.0f, 1.0f);
            unsigned char r = (unsigned char)std::lround(255 * (1.0f - pct));
            unsigned char g = (unsigned char)std::lround(255 * pct);
            Color lifeCol = {r, g, 0, 230};
            
            DrawRectangle(x, y, hpw, h, lifeCol);  // Parte coloreada
            DrawRectangleLines(x, y, w, h, BLACK); // Borde negro para contraste
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
        if (!isAdjacent4(ex, ey, px, py)) continue;

        // 2. Comprobación de Orientación (Facing)
        // El enemigo debe estar mirando hacia el jugador para atacar.
        // Calculamos el vector de mirada del enemigo y el vector hacia el jugador.
        IVec2 edir = facingToDir(i < enemyFacing.size() ? enemyFacing[i] : EnemyFacing::Down);
        int sdx = (px > ex) - (px < ex); // Dirección relativa X (-1, 0, 1)
        int sdy = (py > ey) - (py < ey); // Dirección relativa Y (-1, 0, 1)
        
        // Si los vectores no coinciden, el enemigo no ataca.
        // Esto permite al jugador esquivar moviéndose a la espalda del enemigo.
        if (edir.x != sdx || edir.y != sdy) continue;

        // 3. Comprobación de Cooldowns
        // - enemyAtkCD: El enemigo debe haber descansado de su último golpe.
        // - damageCooldown: El jugador no debe estar en periodo de invencibilidad tras un golpe.
        if (i < enemyAtkCD.size() && enemyAtkCD[i] <= 0.0f && damageCooldown <= 0.0f) {
            // Impacto
            takeDamage(ENEMY_CONTACT_DMG);
            
            // Reiniciar temporizadores
            enemyAtkCD[i] = ENEMY_ATTACK_COOLDOWN;
            damageCooldown = DAMAGE_COOLDOWN; // Otorga invencibilidad breve al jugador
            
            std::cout << "[EnemyAtk] Enemy at (" << ex << "," << ey << ") hit Player!\n";
        }
    }
}

// Generación de enemigos (Spawning)
void Game::spawnEnemiesForLevel() {
    enemies.clear();
    const int n = enemiesPerLevel(currentLevel); // Cantidad según dificultad
    const int minDistTiles = 8; // Distancia de seguridad para no aparecer encima del jugador

    IVec2 spawnTile{px, py};
    auto [exitX, exitY] = map.findExitTile();
    IVec2 exitTile{exitX, exitY};

    // 1. Recolección de candidatos
    // Recorremos todo el mapa buscando tiles válidos (suelo, lejos del jugador y salida).
    std::vector<IVec2> candidates;
    candidates.reserve(map.width() * map.height());
    
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (!map.isWalkable(x, y)) continue; // Debe ser suelo
            // No spawnear en inicio ni salida
            if ((x == spawnTile.x && y == spawnTile.y) ||
                (x == exitTile.x && y == exitTile.y)) continue;
            
            // Distancia Manhattan mínima al jugador
            int manhattan = std::abs(x - spawnTile.x) + std::abs(y - spawnTile.y);
            if (manhattan < minDistTiles) continue;
            
            candidates.push_back({x, y});
        }
    }
    if (candidates.empty()) return; // Mapa demasiado pequeño o lleno

    // 2. Selección aleatoria
    std::uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
    
    // Lista de posiciones ocupadas para evitar superponer enemigos entre sí o con items
    auto used = std::vector<IVec2>{spawnTile, exitTile};
    auto isUsed = [&](int x, int y) {
        for (auto &u : used) if (u.x == x && u.y == y) return true;
        for (auto &it : items) if (it.tile.x == x && it.tile.y == y) return true;
        return false;
    };

    for (int i = 0; i < n; ++i) {
        // Intentamos hasta 200 veces encontrar un hueco libre de la lista de candidatos
        for (int tries = 0; tries < 200; ++tries) {
            const auto &p = candidates[dist(rng)];
            if (!isUsed(p.x, p.y))
            {
                enemies.emplace_back(p.x, p.y);
                used.push_back(p);
                break;
            }
        }
    }
    
    // 3. Escalado de dificultad (HP)
    // Fórmula lineal: HP Base + 25 por nivel extra.
    // Nivel 1: 100 HP, Nivel 2: 125 HP, Nivel 3: 150HP
    int hpForLevel = ENEMY_BASE_HP + (currentLevel - 1) * 25;

    // Inicializamos los vectores paralelos con los datos de los nuevos enemigos
    enemyFacing.assign(enemies.size(), EnemyFacing::Down);
    enemyHP.assign(enemies.size(), hpForLevel); 
    enemyAtkCD.assign(enemies.size(), 0.0f);
    enemyFlashTimer.assign(enemies.size(), 0.0f);
}
