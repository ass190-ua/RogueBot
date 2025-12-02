#include "Game.hpp"
#include "raylib.h"
#include <algorithm> // para std::min y std::clamp
#include <cmath>     // para std::floor
#include <ctime>
#include <iostream>
#include <climits>
#include "GameUtils.hpp"
#include "AssetPath.hpp"
#include "ResourceManager.hpp"

static inline unsigned now_seed() {
    return static_cast<unsigned>(time(nullptr));
}

bool gQuitRequested = false;

static Texture2D loadTex(const char *path) {
    const std::string full = assetPath(path);
    Image img = LoadImage(full.c_str());
    if (img.data == nullptr) {
        Image white = GenImageColor(32, 32, WHITE);
        Texture2D t = LoadTextureFromImage(white);
        UnloadImage(white);
        std::cerr << "[ASSETS] FALLBACK tex para: " << full << "\n";
        return t;
    }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    return t;
}

void ItemSprites::load() {
    if (loaded) return;

    // 1. GENERACIÓN PROCEDURAL "ESTILO RETRO CLEAN"
    
    // A) SUELO: Losas de cemento limpias
    Image imgFloor = GenImageColor(32, 32, Color{30, 30, 35, 255});
    
    // CORRECCIÓN 1: Usamos Rectangle para el borde del suelo
    ImageDrawRectangleLines(&imgFloor, Rectangle{0, 0, 32, 32}, 1, Color{15, 15, 20, 255});
    
    floor = LoadTextureFromImage(imgFloor);
    UnloadImage(imgFloor);

    // B) PARED: Bloque Biselado (Efecto 3D clásico)
    // 1. Base del bloque
    Image imgWall = GenImageColor(32, 32, Color{70, 70, 80, 255});
    
    // 2. Iluminación (Borde Superior e Izquierdo -> Color Claro)
    // ImageDrawRectangle SÍ acepta coordenadas sueltas, así que estas están bien:
    ImageDrawRectangle(&imgWall, 0, 0, 32, 2, Color{110, 110, 120, 255}); // Borde Arriba
    ImageDrawRectangle(&imgWall, 0, 0, 2, 32, Color{110, 110, 120, 255}); // Borde Izquierda
    
    // 3. Sombra (Borde Inferior y Derecho -> Color Oscuro)
    ImageDrawRectangle(&imgWall, 0, 30, 32, 2, Color{40, 40, 50, 255});  // Borde Abajo
    ImageDrawRectangle(&imgWall, 30, 0, 2, 32, Color{40, 40, 50, 255});  // Borde Derecha
    
    // 4. Detalle interior (Un cuadrado grabado en el centro)
    // CORRECCIÓN 2: Usamos Rectangle para el detalle interior
    ImageDrawRectangleLines(&imgWall, Rectangle{8, 8, 16, 16}, 1, Color{50, 50, 60, 255});

    wall = LoadTextureFromImage(imgWall);
    UnloadImage(imgWall);

    // 2. CARGA DE SPRITES (Archivos reales)
    keycard    = ResourceManager::getInstance().getTexture("assets/sprites/items/item_keycard.png");
    shield     = ResourceManager::getInstance().getTexture("assets/sprites/items/item_shield.png");
    pila       = ResourceManager::getInstance().getTexture("assets/sprites/items/item_healthbattery.png");
    glasses    = ResourceManager::getInstance().getTexture("assets/sprites/items/item_glasses.png");
    swordBlue  = ResourceManager::getInstance().getTexture("assets/sprites/items/item_sword_blue.png");
    swordGreen = ResourceManager::getInstance().getTexture("assets/sprites/items/item_sword_green.png");
    swordRed   = ResourceManager::getInstance().getTexture("assets/sprites/items/item_sword_red.png");
    plasma1    = ResourceManager::getInstance().getTexture("assets/sprites/items/item_plasma1.png");
    plasma2    = ResourceManager::getInstance().getTexture("assets/sprites/items/item_plasma2.png");
    battery    = ResourceManager::getInstance().getTexture("assets/sprites/items/item_battery.png");
    
    enemy      = ResourceManager::getInstance().getTexture("assets/sprites/enemies/enemy.png");
    enemyUp    = ResourceManager::getInstance().getTexture("assets/sprites/enemies/enemy_up.png");
    enemyDown  = ResourceManager::getInstance().getTexture("assets/sprites/enemies/enemy_down.png");
    enemyLeft  = ResourceManager::getInstance().getTexture("assets/sprites/enemies/enemy_left.png");
    enemyRight = ResourceManager::getInstance().getTexture("assets/sprites/enemies/enemy_right.png");
    
    // Jugador (Player tiene su propio load, habría que cambiarlo también, 
    // pero ItemSprites ya cumple gran parte).
    
    loaded = true;
}

void ItemSprites::unload() {
    if (!loaded) return;
    
    // Solo descargamos lo que creamos manualmente (procedural)
    UnloadTexture(wall);
    UnloadTexture(floor);
    
    loaded = false;
}

Game::Game(unsigned seed) : fixedSeed(seed) {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "RogueBot"); 
    SetExitKey(KEY_NULL);

    // 1. INICIAR AUDIO (¡Vital!)
    InitAudioDevice();
    SetMasterVolume(0.2f); // Volumen general al 20%

    // 2. GENERAR SONIDOS
    sfxHit       = generateSound(SND_HIT);
    sfxExplosion = generateSound(SND_EXPLOSION);
    sfxPickup    = generateSound(SND_PICKUP);
    sfxPowerUp   = generateSound(SND_POWERUP);
    sfxHurt      = generateSound(SND_HURT);
    sfxWin       = generateSound(SND_WIN);
    sfxLoose     = generateSound(SND_LOOSE);
    sfxAmbient   = generateSound(SND_AMBIENT);
    sfxDash      = generateSound(SND_DASH);

    player.load("assets/sprites/player");
    itemSprites.load();

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();

    camera.target = {0.0f, 0.0f}; 
    camera.offset = {(float)screenW / 2, (float)screenH / 2};
    camera.rotation = 0.0f;
    camera.zoom = cameraZoom; 

    SetTargetFPS(60);

    state = GameState::MainMenu;
    mainMenuSelection = 0;
}

unsigned Game::nextRunSeed() const {
    return fixedSeed > 0 ? fixedSeed : now_seed();
}

unsigned Game::seedForLevel(unsigned base, int level) const {
    const unsigned MIX = 0x9E3779B9u; 
    return base ^ (MIX * static_cast<unsigned>(level));
}

void Game::newRun() {
    runSeed = nextRunSeed();
    std::cout << "[Run] Seed base del run: " << runSeed << "\n";

    currentLevel = 1;
    state = GameState::Playing;
    moveCooldown = 0.0f;
    hp = 10;

    hasKey = false;
    hasShield = false;
    hasBattery = false;
    swordTier = 0;
    plasmaTier = 0;

    runCtx.espadaMejorasObtenidas = 0;
    runCtx.plasmaMejorasObtenidas = 0;
    
    gAttack = AttackRuntime{};
    gAttack.frontOnly = true;

    // Limpiar proyectiles al iniciar run
    projectiles.clear();

    newLevel(currentLevel);
}

void Game::newLevel(int level) {
    const float WORLD_SCALE = 1.2f;
    int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
    int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

    levelSeed = seedForLevel(runSeed, level);
    std::cout << "[Level] " << level << "/" << maxLevels
              << " (seed nivel: " << levelSeed << ")\n";

    map.generate(tilesX, tilesY, levelSeed);

    auto r = map.firstRoom();
    if (r.w > 0 && r.h > 0) {
        px = r.x + r.w / 2;
        py = r.y + r.h / 2;
        map.computeVisibility(px, py, getFovRadius());
    }
    else {
        px = tilesX / 2;
        py = tilesY / 2;
    }

    player.setGridPos(px, py);

    fovTiles = defaultFovFromViewport();
    map.computeVisibility(px, py, getFovRadius());

    Vector2 playerCenterPx = {px * (float)tileSize + tileSize / 2.0f,
                              py * (float)tileSize + tileSize / 2.0f};
    camera.target = playerCenterPx;
    clampCameraToMap();

    hasKey = false; 
    projectiles.clear(); // Limpiar balas al cambiar de nivel

    rng = std::mt19937(levelSeed);

    spawnEnemiesForLevel();

    std::vector<IVec2> enemyTiles;
    enemyTiles.reserve(enemies.size());
    for (const auto &e : enemies) {
        enemyTiles.push_back({e.getX(), e.getY()});
    }

    auto isWalkable = [&](int x, int y) { 
        return map.isWalkable(x, y); 
    };

    IVec2 spawnTile{px, py};
    auto [exitX, exitY] = map.findExitTile();
    IVec2 exitTile{exitX, exitY};

    items = ItemSpawner::generate(map.width(), map.height(), isWalkable,
                                  spawnTile, exitTile, enemyTiles,
                                  level, 
                                  rng, runCtx);
}

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

const char *Game::movementModeText() const {
    return (moveMode == MovementMode::StepByStep) ? "Step-by-step"
                                                  : "Repeat (cooldown)";
}

void Game::run() {
    while (!WindowShouldClose() && !gQuitRequested) {
        processInput();
        update();
        render();

        // --- GESTIÓN DE MÚSICA AMBIENTE ---
        // Solo suena si estamos jugando. Si salimos al menú, se calla.
        if (state == GameState::Playing) {
            // Si no está sonando, dale al play
            if (!IsSoundPlaying(sfxAmbient)) {
                PlaySound(sfxAmbient);
            }
            // Opcional: Ajustar volumen si quieres que sea sutil
            // SetSoundVolume(sfxAmbient, 0.6f); 
        } else {
            // Si estamos en menú/gameover y suena, la paramos
            if (IsSoundPlaying(sfxAmbient)) {
                StopSound(sfxAmbient);
            }
        }
    }

    // DESCARGAR SONIDOS
    UnloadSound(sfxHit);
    UnloadSound(sfxExplosion);
    UnloadSound(sfxPickup);
    UnloadSound(sfxPowerUp);
    UnloadSound(sfxHurt);
    UnloadSound(sfxWin);
    UnloadSound(sfxLoose);
    UnloadSound(sfxAmbient);
    UnloadSound(sfxDash);

    player.unload();
    itemSprites.unload();
    ResourceManager::getInstance().clear(); 
    CloseAudioDevice();
    CloseWindow();
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

void Game::update() {
    if (state != GameState::Playing) return;

    float dt = GetFrameTime();

    // LÓGICA DE DASH 
    if (isDashing) {
        dashTimer -= dt;
        
        // Efecto visual: rastro de partículas
        if (GetRandomValue(0, 10) < 8) { // 80% chance por frame
             // Generar partícula azulada en la posición actual (interpolada)
             // Puedes usar una función spawnParticle simple o reutilizar spawnExplosion con 1 sola partícula
             Vector2 currentPos = camera.target; // Aprox
             // Si quieres ser preciso, calcula la posición interpolada:
             float t = 1.0f - (dashTimer / DASH_DURATION);
             Vector2 lerpPos = {
                 dashStartPos.x + (dashEndPos.x - dashStartPos.x) * t,
                 dashStartPos.y + (dashEndPos.y - dashStartPos.y) * t
             };
             // spawnExplosion(lerpPos, 1, SKYBLUE); // Opcional: rastro
        }

        if (dashTimer <= 0.0f) {
            isDashing = false;
            // Al terminar, aseguramos posición final exacta en grid
            // (La posición lógica px, py ya se actualizó al iniciar el dash, 
            //  aquí solo termina la animación visual si hicieras algo especial)
        }
        
        // Durante el dash, saltamos el resto del update normal (movimiento standard)
        // Pero SÍ actualizamos cámara para seguir la acción rápida
        Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                           py * (float)tileSize + tileSize / 2.0f};
        // Cámara MUY rápida durante el dash
        camera.target.x = desired.x; 
        camera.target.y = desired.y;
        clampCameraToMap();
        
        return; // No hacemos nada más mientras dashea
    }

    // Gestionar Cooldown del Dash
    dashCooldownTimer = std::max(0.0f, dashCooldownTimer - dt);

    tryAutoPickup();
    
    // Temporizadores
    if (hasShield) {
        shieldTimer -= dt;
        if (shieldTimer <= 0.0f) hasShield = false;
    }
    if (glassesTimer > 0.0f) {
        glassesTimer -= dt;
        if (glassesTimer <= 0.0f) recomputeFovIfNeeded(); 
    }
    if (slashActive) {
        slashTimer -= dt;
        if (slashTimer <= 0.0f) slashActive = false;
    }

    for (auto &t : enemyFlashTimer) t = std::max(0.0f, t - dt);
    for (auto &e : enemies) e.updateAnimation(dt);

    updateShooters(dt);
    updateProjectiles(dt);
    updateFloatingTexts(dt);
    updateParticles(dt);

    if (map.at(px, py) == EXIT) {
        if (hasKey) onExitReached();
    }

    // Cámara Suave
    auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                       py * (float)tileSize + tileSize / 2.0f};
    float smooth = 10.0f * dt; 
    camera.target.x = Lerp(camera.target.x, desired.x, smooth);
    camera.target.y = Lerp(camera.target.y, desired.y, smooth);

    // 1. PRIMERO ASEGURAMOS QUE LA CÁMARA ESTÁ DENTRO
    clampCameraToMap(); 

    // 2. DESPUÉS APLICAMOS EL TERREMOTO
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        float intensity = 5.0f;
        camera.target.x += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
        camera.target.y += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
    }

    // Muerte
    if (hp <= 0) {
        if (hasBattery) {
            hasBattery = false;
            hp = hpMax / 2;
            if (hp < 1) hp = 1;
            std::cout << "[Bateria] Resucitado.\n";
            damageCooldown = 2.0f; 
            shakeTimer = 0.5f; 
            PlaySound(sfxPowerUp);
        } else {
            state = GameState::GameOver;
            gAttack.swinging = false; 
            gAttack.lastTiles.clear();
            PlaySound(sfxLoose);
        }
        return;
    }

    damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
    for (auto &cd : enemyAtkCD) cd = std::max(0.0f, cd - dt);
    for (auto &cd : enemyShootCD) cd = std::max(0.0f, cd - dt); 
}

void Game::onExitReached() {
    if (currentLevel < maxLevels) {
        currentLevel++;
        newLevel(currentLevel); 
    }
    else {
        state = GameState::Victory;
        PlaySound(sfxWin);
        std::cout << "[Victory] Fin!\n";
    }
}

void Game::render() {
    if (state == GameState::MainMenu) {
        renderMainMenu();
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(camera);

    // 1. Dibujar Mapa
    map.draw(tileSize, px, py, getFovRadius(), itemSprites.wall, itemSprites.floor);
    
    // 2. Dibujar Enemigos
    drawEnemies();
    
    // 3. Dibujar Jugador
    player.draw(tileSize, px, py);
    
    // 4. Dibujar Items
    drawItems();
    
    // 5. Efectos Visuales
    drawSlash();        // Estela de la espada
    drawProjectiles();  // Balas de plasma
    drawFloatingTexts(); // Números de daño
    drawParticles();    // Sangre y explosiones

    EndMode2D();

    // 6. HUD
    if (state == GameState::Playing) hud.drawPlaying(*this);
    else if (state == GameState::Victory) hud.drawVictory(*this);
    else if (state == GameState::GameOver) hud.drawGameOver(*this);

    EndDrawing();
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

Rectangle Game::uiCenterRect(float w, float h) const {
    return {(screenW - w) * 0.5f, (screenH - h) * 0.5f, w, h};
}

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

void Game::drawProjectiles() const {
    for (const auto& p : projectiles) {
        // Elegir color según el bando
        Color mainColor = p.isEnemy ? RED : SKYBLUE;
        
        // Si es enemiga, el núcleo lo hacemos naranja para que parezca fuego/láser dañino
        // Si es tuya, el núcleo es blanco puro (plasma)
        Color coreColor = p.isEnemy ? ORANGE : WHITE;

        // Dibujar halo exterior
        DrawCircleV(p.pos, 5.0f, mainColor);
        
        // Dibujar núcleo brillante
        DrawCircleV(p.pos, 2.0f, coreColor);
    }
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

// --- SISTEMA DE PARTÍCULAS (Al final de Game.cpp) ---

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
