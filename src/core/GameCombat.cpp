#include "Game.hpp"
#include "GameUtils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>


// -----------------------------------------------------------------------------
// IMPLEMENTACIÓN DE COMBATE Y PROYECTILES
// -----------------------------------------------------------------------------

void Game::performMeleeAttack() {
    gAttack.rangeTiles = 1;
    gAttack.cooldown   = CD_HANDS;
    gAttack.swingTime  = 0.1f;
    gAttack.frontOnly  = true; 

    gAttack.swinging   = true;
    gAttack.swingTimer = gAttack.swingTime;
    gAttack.cdTimer    = gAttack.cooldown; 

    IVec2 center{px, py};
    gAttack.lastTiles = computeMeleeTilesOccluded(
        center, gAttack.lastDir, gAttack.rangeTiles, gAttack.frontOnly, map);

    bool hit = false;
    for (size_t i = 0; i < enemies.size(); ++i) {
        IVec2 epos = {enemies[i].getX(), enemies[i].getY()};
        bool impacted = false;
        for (const auto& t : gAttack.lastTiles) {
            if (t.x == epos.x && t.y == epos.y) { impacted = true; break; }
        }
        
        if (impacted) {
            hit = true;
            
            // SONIDO GOLPE
            PlaySound(sfxHit); 

            if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
            
            enemyHP[i] -= DMG_HANDS;

            Vector2 ePos = { (float)enemies[i].getX() * tileSize + 8, 
                             (float)enemies[i].getY() * tileSize - 10 };
            spawnFloatingText(ePos, DMG_HANDS, RAYWHITE);

            if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;

            std::cout << "[Melee] Puñetazo! -" << DMG_HANDS << "\n";
            
            // PROVOCACIÓN (IA Agresiva)
            if (i < enemyAtkCD.size()) enemyAtkCD[i] = 0.0f;
            int dx = px - enemies[i].getX();
            int dy = py - enemies[i].getY();
            if (i < enemyFacing.size()) {
                if (std::abs(dx) >= std::abs(dy)) enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                else enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
            }
        }
    }
    
    // Limpieza de muertos + EXPLOSIONES + SONIDO MUERTE
    if (hit) {
         std::vector<size_t> toRemove;
         for(size_t i=0; i<enemyHP.size(); ++i) if(enemyHP[i] <= 0) toRemove.push_back(i);
         if (!toRemove.empty()) {
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (size_t idx : toRemove) {
                // EXPLOSIÓN VISUAL
                float ex = enemies[idx].getX() * tileSize + tileSize / 2.0f;
                float ey = enemies[idx].getY() * tileSize + tileSize / 2.0f;
                spawnExplosion({ex, ey}, 15, DARKGRAY);
                spawnExplosion({ex, ey}, 5, RED); 
                
                // SONIDO EXPLOSIÓN
                PlaySound(sfxExplosion);

                enemies.erase(enemies.begin() + idx);
                if (idx < enemyFacing.size()) enemyFacing.erase(enemyFacing.begin() + idx);
                if (idx < enemyHP.size()) enemyHP.erase(enemyHP.begin() + idx);
                if (idx < enemyAtkCD.size()) enemyAtkCD.erase(enemyAtkCD.begin() + idx);
                if (idx < enemyFlashTimer.size()) enemyFlashTimer.erase(enemyFlashTimer.begin() + idx);
            }
        }
    }

    enemyTryAttackFacing();
}

void Game::performSwordAttack() {
    int dmg = DMG_SWORD_T1;
    Color trailColor = SKYBLUE; // Color T1 (Azul claro)

    if (swordTier == 2) { 
        dmg = DMG_SWORD_T2; 
        trailColor = LIME; // Color T2 (Verde)
    }
    if (swordTier == 3) { 
        dmg = DMG_SWORD_T3; 
        trailColor = RED;  // Color T3 (Rojo Sith)
    }

    // --- ACTIVAR EFECTO SLASH ---
    slashActive = true;
    slashTimer = 0.15f; // Duración corta y rápida
    slashColor = trailColor;

    // Calcular ángulo según dirección (Raylib usa grados: Derecha=0, Abajo=90...)
    if (gAttack.lastDir.x > 0) slashBaseAngle = 0.0f;        // Derecha
    else if (gAttack.lastDir.x < 0) slashBaseAngle = 180.0f; // Izquierda
    else if (gAttack.lastDir.y > 0) slashBaseAngle = 90.0f;  // Abajo
    else slashBaseAngle = 270.0f;                            // Arriba
    // ----------------------------

    gAttack.rangeTiles = 1; 
    gAttack.cooldown   = CD_SWORD;
    gAttack.swingTime  = 0.15f; 
    gAttack.frontOnly  = true; 

    gAttack.swinging   = true;
    gAttack.swingTimer = gAttack.swingTime;
    gAttack.cdTimer    = gAttack.cooldown;
    
    IVec2 center{px, py};
    gAttack.lastTiles = computeMeleeTilesOccluded(
        center, gAttack.lastDir, gAttack.rangeTiles, gAttack.frontOnly, map);

    bool hit = false;
    for (size_t i = 0; i < enemies.size(); ++i) {
        IVec2 epos = {enemies[i].getX(), enemies[i].getY()};
        for (const auto& t : gAttack.lastTiles) {
            if (t.x == epos.x && t.y == epos.y) {
                hit = true;
                
                PlaySound(sfxHit);

                if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
                
                enemyHP[i] -= dmg;

                Vector2 ePos = { (float)enemies[i].getX() * tileSize + 8, 
                                 (float)enemies[i].getY() * tileSize - 10 };
                // Usamos el mismo color del trail para el texto
                spawnFloatingText(ePos, dmg, trailColor);

                if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;

                std::cout << "[Sword] Slash! -" << dmg << "\n";

                if (i < enemyAtkCD.size()) enemyAtkCD[i] = 0.0f; 
                int dx = px - enemies[i].getX();
                int dy = py - enemies[i].getY();
                if (i < enemyFacing.size()) {
                    if (std::abs(dx) >= std::abs(dy)) enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                    else enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
                }
                break;
            }
        }
    }
    
     if (hit) {
         std::vector<size_t> toRemove;
         for(size_t i=0; i<enemyHP.size(); ++i) if(enemyHP[i] <= 0) toRemove.push_back(i);
         if (!toRemove.empty()) {
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (size_t idx : toRemove) {
                float ex = enemies[idx].getX() * tileSize + tileSize / 2.0f;
                float ey = enemies[idx].getY() * tileSize + tileSize / 2.0f;
                spawnExplosion({ex, ey}, 15, DARKGRAY);
                spawnExplosion({ex, ey}, 5, RED); 
                
                PlaySound(sfxExplosion);

                enemies.erase(enemies.begin() + idx);
                if (idx < enemyFacing.size()) enemyFacing.erase(enemyFacing.begin() + idx);
                if (idx < enemyHP.size()) enemyHP.erase(enemyHP.begin() + idx);
                if (idx < enemyAtkCD.size()) enemyAtkCD.erase(enemyAtkCD.begin() + idx);
                if (idx < enemyShootCD.size()) enemyShootCD.erase(enemyShootCD.begin() + idx);
                if (idx < enemyFlashTimer.size()) enemyFlashTimer.erase(enemyFlashTimer.begin() + idx);
            }
        }
    }

    enemyTryAttackFacing();
}

void Game::performPlasmaAttack() {
    plasmaCooldown = CD_PLASMA;
    spawnProjectile(DMG_PLASMA);

    if (plasmaTier >= 2) {
        burstShotsLeft = 1;     
        burstTimer = 0.15f;     
    } else {
        burstShotsLeft = 0;
    }
}

void Game::spawnProjectile(int dmg) {
    Projectile p;
    p.pos = { px * (float)tileSize + tileSize/2.0f, py * (float)tileSize + tileSize/2.0f };
    
    Vector2 dir = { (float)gAttack.lastDir.x, (float)gAttack.lastDir.y };
    if (dir.x == 0 && dir.y == 0) dir = {0, 1};

    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len > 0) { dir.x/=len; dir.y/=len; }

    p.vel = { dir.x * PLASMA_SPEED, dir.y * PLASMA_SPEED };
    p.maxDistance = PLASMA_RANGE_TILES * tileSize;
    p.distanceTraveled = 0.0f;
    p.damage = dmg;
    p.active = true;

    projectiles.push_back(p);
}

void Game::updateProjectiles(float dt) {
    // 1. Burst Jugador
    if (burstShotsLeft > 0) {
        burstTimer -= dt;
        if (burstTimer <= 0.0f) {
            spawnProjectile(DMG_PLASMA);
            burstShotsLeft--;
            burstTimer = 0.15f;
        }
    }

    // 2. Movimiento y Colisiones
    for (auto &p : projectiles) {
        if (!p.active) continue;

        float step = std::sqrt(p.vel.x*p.vel.x + p.vel.y*p.vel.y) * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.distanceTraveled += step;

        if (p.distanceTraveled >= p.maxDistance) {
            p.active = false;
            continue;
        }

        // Paredes
        int tx = (int)(p.pos.x / tileSize);
        int ty = (int)(p.pos.y / tileSize);
        if (tx < 0 || ty < 0 || tx >= map.width() || ty >= map.height() || 
            map.at(tx, ty) == WALL) {
            p.active = false; 
            continue;
        }

        // LÓGICA DE IMPACTO
        if (p.isEnemy) {
            // --- BALA ENEMIGA -> JUGADOR ---
            Vector2 pCenter = { px * (float)tileSize + tileSize/2.0f, py * (float)tileSize + tileSize/2.0f };
            float dx = p.pos.x - pCenter.x;
            float dy = p.pos.y - pCenter.y;
            float rad = tileSize * 0.4f; // Hitbox pequeña para esquivar

            if (dx*dx + dy*dy < rad*rad) {
                p.active = false;
                takeDamage(p.damage); 
                // Feedback visual
                spawnFloatingText(p.pos, p.damage, RED);
                break;
            }
        } 
        else {
            // --- BALA JUGADOR -> ENEMIGOS ---
            for (size_t i = 0; i < enemies.size(); ++i) {
                Vector2 ePos = { 
                    enemies[i].getX() * (float)tileSize + tileSize/2.0f,
                    enemies[i].getY() * (float)tileSize + tileSize/2.0f
                };
                
                float dx = p.pos.x - ePos.x;
                float dy = p.pos.y - ePos.y;
                float rad = tileSize * 0.6f; 

                if (dx*dx + dy*dy < rad*rad) {
                    p.active = false;
                    PlaySound(sfxHit);

                    if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), 100);
                    enemyHP[i] -= p.damage;

                    Vector2 txtPos = { (float)enemies[i].getX() * tileSize + 8, 
                                       (float)enemies[i].getY() * tileSize - 10 };
                    spawnFloatingText(txtPos, p.damage, SKYBLUE); 

                    if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;
        
                    // Provocación (Corrección Bug anterior)
                    if (i < enemyAtkCD.size()) enemyAtkCD[i] = 0.0f;
                    int edx = px - enemies[i].getX();
                    int edy = py - enemies[i].getY();
                    if (i < enemyFacing.size()) {
                        if (std::abs(edx) >= std::abs(edy)) enemyFacing[i] = (edx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                        else enemyFacing[i] = (edy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
                    }

                    break; 
                }
            }
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), 
            [](const Projectile& p){ return !p.active; }),
        projectiles.end()
    );

    // 3. Limpieza de muertos
    std::vector<size_t> toRemove;
    for(size_t i=0; i<enemyHP.size(); ++i) {
        if (enemyHP[i] <= 0) toRemove.push_back(i);
    }
    if (!toRemove.empty()) {
        std::sort(toRemove.rbegin(), toRemove.rend());
        for (size_t idx : toRemove) {
            float ex = enemies[idx].getX() * tileSize + tileSize / 2.0f;
            float ey = enemies[idx].getY() * tileSize + tileSize / 2.0f;
            spawnExplosion({ex, ey}, 15, DARKGRAY);
            spawnExplosion({ex, ey}, 5, RED); 
            PlaySound(sfxExplosion);

            enemies.erase(enemies.begin() + idx);
            if (idx < enemyFacing.size()) enemyFacing.erase(enemyFacing.begin() + idx);
            if (idx < enemyHP.size()) enemyHP.erase(enemyHP.begin() + idx);
            if (idx < enemyAtkCD.size()) enemyAtkCD.erase(enemyAtkCD.begin() + idx);
            if (idx < enemyShootCD.size()) enemyShootCD.erase(enemyShootCD.begin() + idx); // <--- BORRAR CD DISPARO
            if (idx < enemyFlashTimer.size()) enemyFlashTimer.erase(enemyFlashTimer.begin() + idx);
        }
    }
    
    enemyTryAttackFacing();
}

void Game::updateShooters(float dt) {
    for (size_t i = 0; i < enemies.size(); ++i) {
        // Solo procesamos Shooters vivos
        if (enemies[i].getType() != Enemy::Shooter) continue;
        if (enemyHP[i] <= 0) continue; 

        // Chequear Cooldown (Tiempo Real)
        if (enemyShootCD[i] > 0.0f) continue;

        const int ex = enemies[i].getX();
        const int ey = enemies[i].getY();

        // 1. Chequear Alineación (Ejes)
        if (ex != px && ey != py) continue; // No está en línea recta

        // 2. Chequear Paredes (Raycast simple)
        bool blocked = false;
        if (ex == px) { // Vertical
            int y1 = std::min(ey, py) + 1;
            int y2 = std::max(ey, py);
            for (int y = y1; y < y2; ++y) if (map.at(px, y) == WALL) { blocked = true; break; }
        } else { // Horizontal
            int x1 = std::min(ex, px) + 1;
            int x2 = std::max(ex, px);
            for (int x = x1; x < x2; ++x) if (map.at(x, py) == WALL) { blocked = true; break; }
        }
        
        if (blocked) continue;
        
        // A. Girar hacia el jugador (por si estaba mirando a otro lado)
        int dx = px - ex;
        int dy = py - ey;
        if (dx > 0) enemyFacing[i] = EnemyFacing::Right;
        else if (dx < 0) enemyFacing[i] = EnemyFacing::Left;
        else if (dy > 0) enemyFacing[i] = EnemyFacing::Down;
        else if (dy < 0) enemyFacing[i] = EnemyFacing::Up;

        // B. Crear el Proyectil Enemigo
        Projectile p;
        p.isEnemy = true; // Importante: Daña al jugador
        p.active = true;
        p.damage = 1; // 1 = Medio Corazón
        p.maxDistance = 7.0f * tileSize;
        p.pos = { ex * (float)tileSize + tileSize/2.0f, ey * (float)tileSize + tileSize/2.0f };
        
        IVec2 dir = facingToDir(enemyFacing[i]);
        p.vel = { dir.x * 150.0f, dir.y * 150.0f }; // Velocidad lenta esquivable
        
        projectiles.push_back(p);
        
        // C. Reiniciar Cooldown (Dispara cada 2.5 segundos)
        enemyShootCD[i] = 2.5f; 
        
        // D. Efectos
        PlaySound(sfxDash); // Reusamos el sonido de aire/silenciador
    }
}

void Game::spawnExplosion(Vector2 pos, int count, Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos;
        
        // Velocidad aleatoria en 360 grados
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(50, 150); // Rapidez variable
        
        p.vel = { cosf(angle) * speed, sinf(angle) * speed };
        
        p.maxLife = (float)GetRandomValue(5, 10) / 10.0f; // Entre 0.5s y 1.0s
        p.life = p.maxLife;
        
        p.size = (float)GetRandomValue(2, 5); // Tamaños variados
        p.color = color;
        
        particles.push_back(p);
    }
}

void Game::updateParticles(float dt) {
    for (auto &p : particles) {
        p.life -= dt;
        
        // Movimiento
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        
        // Fricción (las partículas se frenan un poco)
        p.vel.x *= 0.95f;
        p.vel.y *= 0.95f;
        
        // Opcional: Hacerlas más pequeñas al morir
        if (p.life < 0.5f) p.size *= 0.98f;
    }

    // Borrar partículas muertas
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), 
            [](const Particle& p){ return p.life <= 0.0f; }),
        particles.end()
    );
}

void Game::drawParticles() const {
    for (const auto& p : particles) {
        // Fade out (se vuelven transparentes al final)
        float alpha = p.life / p.maxLife;
        Color c = Fade(p.color, alpha);
        
        DrawRectangleV(p.pos, {p.size, p.size}, c);
    }
}

void Game::drawSlash() const {
    if (!slashActive) return;

    // Calculamos el centro del jugador
    Vector2 center = { 
        px * (float)tileSize + tileSize / 2.0f, 
        py * (float)tileSize + tileSize / 2.0f 
    };

    // Radio del corte (un poco más grande que el tile para que sobresalga)
    float radiusInner = tileSize * 0.5f;
    float radiusOuter = tileSize * 1.2f;

    // Ángulos del arco (120 grados de amplitud)
    float startAngle = slashBaseAngle - 60.0f;
    float endAngle   = slashBaseAngle + 60.0f;

    // Fade Out (Se vuelve transparente al final)
    // 0.15f es la duración total del golpe
    float alpha = std::clamp(slashTimer / 0.15f, 0.0f, 1.0f); 
    Color c = Fade(slashColor, alpha);

    // Dibujamos el arco
    DrawRing(center, radiusInner, radiusOuter, startAngle, endAngle, 16, c);
}

// SISTEMA DE TEXTOS FLOTANTES
void Game::spawnFloatingText(Vector2 pos, int value, Color color) {
    FloatingText ft;
    // Un poco de aleatoriedad en la posición para que no se solapen si hay muchos
    float offsetX = (float)(rand() % 16 - 8); 
    ft.pos = { pos.x + offsetX, pos.y };
    ft.value = value;
    ft.lifeTime = 0.8f; // Duran 0.8 segundos
    ft.timer = ft.lifeTime;
    ft.color = color;
    floatingTexts.push_back(ft);
}

void Game::updateFloatingTexts(float dt) {
    for (auto &ft : floatingTexts) {
        ft.timer -= dt;
        // El texto sube hacia arriba (eje Y negativo)
        ft.pos.y -= 30.0f * dt; 
    }

    // Borrar textos expirados
    floatingTexts.erase(
        std::remove_if(floatingTexts.begin(), floatingTexts.end(), 
            [](const FloatingText& ft){ return ft.timer <= 0.0f; }),
        floatingTexts.end()
    );
}

void Game::drawFloatingTexts() const {
    for (const auto& ft : floatingTexts) {
        // Calcular transparencia (fade out)
        float alpha = ft.timer / ft.lifeTime;
        Color c = ft.color;
        c.a = (unsigned char)(255.0f * alpha);

        // Dibujar borde negro para que se lea bien
        const char* txt = TextFormat("%d", ft.value);
        DrawText(txt, (int)ft.pos.x + 1, (int)ft.pos.y + 1, 10, Fade(BLACK, alpha)); // Sombra
        DrawText(txt, (int)ft.pos.x, (int)ft.pos.y, 10, c); // Texto
    }
}