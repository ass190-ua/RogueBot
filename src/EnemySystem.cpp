#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// Dibujo de enemigos
void Game::drawEnemies() const {
    auto visible = [&](int x, int y) { 
        return !map.fogEnabled() || map.isVisible(x, y); 
    };

    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto &e = enemies[i];
        if (!visible(e.getX(), e.getY())) continue;

        // elige textura por facing
        const Texture2D *tex = nullptr;
        switch (i < enemyFacing.size() ? enemyFacing[i] : EnemyFacing::Down) {
            case EnemyFacing::Up:    tex = &itemSprites.enemyUp;   break;
            case EnemyFacing::Down:  tex = &itemSprites.enemyDown; break;
            case EnemyFacing::Left:  tex = &itemSprites.enemyLeft; break;
            case EnemyFacing::Right: tex = &itemSprites.enemyRight;break;
        }

        const float xpx = (float)(e.getX() * tileSize);
        const float ypx = (float)(e.getY() * tileSize);

        if (tex && tex->id != 0) {
            Rectangle src{0,0,(float)tex->width,(float)tex->height};
            Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
            Vector2 origin{0,0};
            DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
        }
        else if (itemSprites.enemy.id != 0) {
            Rectangle src{0,0,(float)itemSprites.enemy.width,(float)itemSprites.enemy.height};
            Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
            Vector2 origin{0,0};
            DrawTexturePro(itemSprites.enemy, src, dst, origin, 0.0f, WHITE);
        }
        else {
            // Fallback final
            e.draw(tileSize);
        }

        // barra de vida
        if (i < enemyHP.size()) {
            const int w = tileSize, h = 4;
            const int x = (int)(e.getX() * tileSize);
            const int y = (int)(e.getY() * tileSize) - (h + 2);

            int hpw = (int)std::lround(w * (enemyHP[i] / 100.0));
            hpw = std::clamp(hpw, 0, w);

            DrawRectangle(x, y, w, h, Color{60, 60, 60, 200});
            float pct = std::clamp(static_cast<float>(enemyHP[i]) / 100.0f, 0.0f, 1.0f);
            unsigned char r = (unsigned char)std::lround(255 * (1.0f - pct));
            unsigned char g = (unsigned char)std::lround(255 * pct);
            Color lifeCol = {r, g, 0, 230};
            DrawRectangle(x, y, hpw, h, lifeCol);
            DrawRectangleLines(x, y, w, h, BLACK);
        }
    }
}

// Ataque enemigo (cara a cara)
void Game::enemyTryAttackFacing() {
    for (size_t i = 0; i < enemies.size(); ++i) {
        const int ex = enemies[i].getX();
        const int ey = enemies[i].getY();

        if (!isAdjacent4(ex, ey, px, py)) continue;

        // ¿El enemigo mira al jugador?
        IVec2 edir = facingToDir(i < enemyFacing.size() ? enemyFacing[i] : EnemyFacing::Down);
        int sdx = (px > ex) - (px < ex);
        int sdy = (py > ey) - (py < ey);
        if (edir.x != sdx || edir.y != sdy) continue;

        // ¿El jugador mira al enemigo?
        IVec2 pdir = dominantAxis((gAttack.lastDir.x == 0 && gAttack.lastDir.y == 0)
                                  ? IVec2{0, 1} : gAttack.lastDir);
        int psx = (ex > px) - (ex < px);
        int psy = (ey > py) - (ey < py);
        bool playerFacingEnemy = (pdir.x == psx && pdir.y == psy);
        if (!playerFacingEnemy) continue;

        if (i < enemyAtkCD.size() && enemyAtkCD[i] <= 0.0f && damageCooldown <= 0.0f) {
            takeDamage(ENEMY_CONTACT_DMG);
            enemyAtkCD[i] = ENEMY_ATTACK_COOLDOWN;
            damageCooldown = DAMAGE_COOLDOWN;
            std::cout << "[EnemyAtk] (" << ex << "," << ey
                      << ") facing player and player facing enemy.\n";
        }
    }
}

// Spawn enemigos por nivel
void Game::spawnEnemiesForLevel() {
    enemies.clear();
    const int n = enemiesPerLevel(currentLevel);
    const int minDistTiles = 8;

    IVec2 spawnTile{px, py};
    auto [exitX, exitY] = map.findExitTile();
    IVec2 exitTile{exitX, exitY};

    std::vector<IVec2> candidates;
    candidates.reserve(map.width() * map.height());
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (!map.isWalkable(x, y)) continue;
            if ((x == spawnTile.x && y == spawnTile.y) ||
                (x == exitTile.x && y == exitTile.y)) continue;
            int manhattan = std::abs(x - spawnTile.x) + std::abs(y - spawnTile.y);
            if (manhattan < minDistTiles) continue;
            candidates.push_back({x, y});
        }
    }
    if (candidates.empty()) return;

    std::uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
    auto used = std::vector<IVec2>{spawnTile, exitTile};
    auto isUsed = [&](int x, int y) {
        for (auto &u : used) if (u.x == x && u.y == y) return true;
        for (auto &it : items) if (it.tile.x == x && it.tile.y == y) return true;
        return false;
    };

    for (int i = 0; i < n; ++i) {
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
    enemyFacing.assign(enemies.size(), EnemyFacing::Down);
    enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
    enemyAtkCD.assign(enemies.size(), 0.0f);
}
