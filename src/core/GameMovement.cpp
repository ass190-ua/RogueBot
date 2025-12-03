#include "Game.hpp"
#include "GameUtils.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>


void Game::tryMove(int dx, int dy) {
    if (dx == 0 && dy == 0) return;

    int nx = px + dx, ny = py + dy;

    if (nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
        map.at(nx, ny) != WALL) {
        bool occupied = false;
        for (const auto &e : enemies) {
            if (e.getX() == nx && e.getY() == ny) {
                occupied = true;
                break;
            }
        }

        if (!occupied) {
            px = nx;
            py = ny;
        }
    }
}

void Game::onSuccessfulStep(int dx, int dy) {
    if (dx != 0 || dy != 0) {
        gAttack.lastDir = {dx, dy};           
        player.setDirectionFromDelta(dx, dy); 
    }
    player.setGridPos(px, py);                
    recomputeFovIfNeeded();                   
    centerCameraOnPlayer();                   
    updateEnemiesAfterPlayerMove(true);       
}

void Game::updateEnemiesAfterPlayerMove(bool moved) {
    if (!moved) return; 

    struct Intent {
        int fromx, fromy; 
        int tox, toy;     
        bool wants;       
        int score;        
        size_t idx;       
    };

    auto inRangePx = [&](int ex, int ey) -> bool {
        int dx = px - ex, dy = py - ey;
        float distPx = std::sqrt(float(dx * dx + dy * dy)) * float(tileSize);
        return distPx <= float(ENEMY_DETECT_RADIUS_PX);
    };

    auto can = [&](int nx, int ny) -> bool {
        if (nx == px && ny == py) return false; // No pisar al jugador
        return nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
               map.isWalkable(nx, ny);
    };

    auto sgn = [](int v) { return (v > 0) - (v < 0); };

    auto greedyNext = [&](int ex, int ey) -> std::pair<int, int> {
        int dx = px - ex, dy = py - ey;
        if (dx == 0 && dy == 0) return {ex, ey};
        if (std::abs(dx) >= std::abs(dy)) {
            int nx = ex + sgn(dx), ny = ey;
            if (can(nx, ny)) return {nx, ny};
            nx = ex; ny = ey + sgn(dy);
            if (can(nx, ny)) return {nx, ny};
        } else {
            int nx = ex, ny = ey + sgn(dy);
            if (can(nx, ny)) return {nx, ny};
            nx = ex + sgn(dx); ny = ey;
            if (can(nx, ny)) return {nx, ny};
        }
        return {ex, ey}; 
    };

    std::vector<Intent> intents;
    intents.reserve(enemies.size());

    // FASE 1: DECIDIR INTENCIONES
    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto &e = enemies[i];
        Intent it{e.getX(), e.getY(), e.getX(), e.getY(), false, 1'000'000, i};

        bool shouldMove = true;

        // LÓGICA SHOOTER: Si tengo tiro, ME PARO (pero NO disparo aquí, eso lo hace updateShooters)
        if (e.getType() == Enemy::Shooter) {
            bool hasLoS = false;
            if (e.getX() == px || e.getY() == py) {
                hasLoS = true;
                int x1 = std::min(e.getX(), px), x2 = std::max(e.getX(), px);
                int y1 = std::min(e.getY(), py), y2 = std::max(e.getY(), py);
                
                if (e.getY() == py) { 
                    for(int x = x1 + 1; x < x2; ++x) if(map.at(x, py) == WALL) hasLoS = false;
                } else { 
                    for(int y = y1 + 1; y < y2; ++y) if(map.at(px, y) == WALL) hasLoS = false;
                }
            }

            if (hasLoS && inRangePx(e.getX(), e.getY())) {
                shouldMove = false; // STOP para apuntar
                
                // Actualizamos facing para mirar al jugador
                int dx = px - e.getX();
                int dy = py - e.getY();
                if (dx > 0) enemyFacing[i] = EnemyFacing::Right;
                else if (dx < 0) enemyFacing[i] = EnemyFacing::Left;
                else if (dy > 0) enemyFacing[i] = EnemyFacing::Down;
                else if (dy < 0) enemyFacing[i] = EnemyFacing::Up;
            }
        }

        // Si debe moverse y está en rango, calcula ruta
        if (shouldMove && inRangePx(e.getX(), e.getY())) {
            auto [nx, ny] = greedyNext(e.getX(), e.getY());
            it.tox = nx; it.toy = ny;
            it.wants = (nx != e.getX() || ny != e.getY());
            it.score = std::abs(px - nx) + std::abs(py - ny);
        } else {
            it.score = std::abs(px - e.getX()) + std::abs(py - e.getY()); 
        }
        intents.push_back(it);
    }

    // FASE 2: RESOLUCIÓN DE CONFLICTOS
    for (size_t i = 0; i < intents.size(); ++i) {
        if (!intents[i].wants) continue;
        for (size_t j = i + 1; j < intents.size(); ++j) {
            if (!intents[j].wants) continue;
            if (intents[i].tox == intents[j].tox && intents[i].toy == intents[j].toy) {
                if (intents[j].score < intents[i].score) intents[i].wants = false;
                else intents[j].wants = false;
            }
            if (intents[i].tox == intents[j].fromx && intents[i].toy == intents[j].fromy &&
                intents[j].tox == intents[i].fromx && intents[j].toy == intents[i].fromy) {
                 intents[i].wants = false; intents[j].wants = false;
            }
        }
    }

    // FASE 3: MOVER
    for (size_t i = 0; i < enemies.size(); ++i) {
        int ox = intents[i].fromx, oy = intents[i].fromy;
        if (intents[i].wants) {
            enemies[i].setPos(intents[i].tox, intents[i].toy);
            int dx = intents[i].tox - ox;
            int dy = intents[i].toy - oy;

            // Si va a la derecha, se inclina a la derecha (-15 grados visuales)
            // Si va a la izquierda, a la izquierda (+15 grados)
            // Si va arriba/abajo, hacemos un pequeño "wobble" alterno
            if (dx > 0) enemies[i].addTilt(-15.0f);
            else if (dx < 0) enemies[i].addTilt(15.0f);
            else enemies[i].addTilt((i % 2 == 0) ? 10.0f : -10.0f);

            if (dx > 0) enemyFacing[i] = EnemyFacing::Right;
            else if (dx < 0) enemyFacing[i] = EnemyFacing::Left;
            else if (dy > 0) enemyFacing[i] = EnemyFacing::Down;
            else if (dy < 0) enemyFacing[i] = EnemyFacing::Up;
        } else {
            // Si choca o se para, se gira hacia el jugador si está adyacente
            if (isAdjacent4(ox, oy, px, py)) {
                int dx = px - ox; int dy = py - oy;
                if (std::abs(dx) >= std::abs(dy)) enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                else enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
            }
        }
    }

    enemyTryAttackFacing();
}

void Game::takeDamage(int amount) {
    if (isDashing) return;
    // Si tiene escudo
    if (hasShield) {
        hasShield = false;    // Romper el escudo
        shieldTimer = 0.0f;   // Resetear tiempo visual
        
        // Feedback visual: Muestra un "0" azul indicando daño bloqueado
        Vector2 pos = { px * (float)tileSize + 8, py * (float)tileSize - 10 };
        spawnFloatingText(pos, 0, SKYBLUE); 

        std::cout << "[Shield] ¡Golpe bloqueado! El escudo se ha roto.\n";
        return; // Salimos sin restar vida real
    }

    // 2. Si no tiene escudo (Daño normal)
    hp = std::max(0, hp - amount);
    PlaySound(sfxHurt);

    shakeTimer = 0.3f;
    
    // Feedback visual: Texto flotante ROJO con el daño recibido
    Vector2 pos = { px * (float)tileSize + 8, py * (float)tileSize - 10 };
    spawnFloatingText(pos, amount, RED);
    
    std::cout << "[HP] -" << amount << " -> " << hp << "\n";
}

const char *Game::movementModeText() const {
    return (moveMode == MovementMode::StepByStep) ? "Step-by-step"
                                                  : "Repeat (cooldown)";
}