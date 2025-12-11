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

    // Carga del Boss
    bossDownIdle = ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_down_idle.png");
    bossUpIdle   = ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_up_idle.png");
    bossLeftIdle = ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_left_idle.png");
    bossRightIdle= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_right_idle.png");
    
    bossDownWalk1= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_down_walk1.png");
    bossDownWalk2= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_down_walk2.png");
    bossUpWalk1  = ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_up_walk1.png");
    bossUpWalk2  = ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_up_walk2.png");
    bossLeftWalk1= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_left_walk1.png");
    bossLeftWalk2= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_left_walk2.png");
    bossRightWalk1= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_right_walk1.png");
    bossRightWalk2= ResourceManager::getInstance().getTexture("assets/sprites/boss/boss_right_walk2.png");
    
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

    // Reiniciar estado básico
    currentLevel = 1;
    state = GameState::Playing;
    moveCooldown = 0.0f;
    hp = 10; // O el valor inicial que desees

    // --- REINICIAR INVENTARIO Y POWER-UPS ---
    hasKey = false;
    
    // Escudo
    hasShield = false;
    shieldTimer = 0.0f; // ¡Importante resetear el tiempo!

    // Batería (Vida extra)
    hasBattery = false;

    // Gafas 3D (Visión)
    glassesTimer = 0.0f; // ¡Importante! Si no es 0, el juego cree que siguen activas
    glassesFovMod = 0;   // Resetear el modificador de visión extra

    // Armas
    swordTier = 0;
    plasmaTier = 0;

    // --- REINICIAR MODO DIOS (Seguridad) ---
    godMode = false;
    showGodModeInput = false;
    map.setRevealAll(false); // Apagar la luz del modo dios

    // Reiniciar contexto de mejoras entre niveles
    runCtx.espadaMejorasObtenidas = 0;
    runCtx.plasmaMejorasObtenidas = 0;
    
    // Reiniciar lógica de ataque
    gAttack = AttackRuntime{};
    gAttack.frontOnly = true;

    // Limpiar proyectiles y entidades
    projectiles.clear();
    floatingTexts.clear(); // Limpiar números flotantes viejos
    particles.clear();     // Limpiar explosiones viejas

    // Reset boss
    boss = Boss{};

    newLevel(currentLevel);
}

void Game::newLevel(int level) {
    // MODIFICADO: Lógica especial para Nivel 4 (Boss)
    
    projectiles.clear(); // Limpiar balas
    enemies.clear();     // Limpiar enemigos anteriores
    items.clear();       // Limpiar items
    
    levelSeed = seedForLevel(runSeed, level);
    rng = std::mt19937(levelSeed);
    
    if (level == maxLevels) { // NIVEL FINAL
        std::cout << "[Level] FINAL BOSS LEVEL Initializing...\n";
        
        // 1. Generar Arena A PANTALLA COMPLETA
        // Calculamos cuántos tiles caben en la pantalla
        int arenaW = screenW / tileSize;
        int arenaH = screenH / tileSize;
        
        // Generamos la arena con esas dimensiones
        map.generateBossArena(arenaW, arenaH);
        
        // 2. Configurar Visibilidad
        map.setFogEnabled(false); 
        
        // 3. Posicionar Jugador (Abajo centro)
        px = arenaW / 2;
        py = arenaH - 4; // Un poco separado del borde
        player.setGridPos(px, py);
        
        // 4. Centrar cámara (Fija en el centro de la pantalla)
        // Al ser tamaño exacto de pantalla, el centro es screenW/2, screenH/2
        camera.target = {(float)screenW / 2.0f, (float)screenH / 2.0f};
        
        fovTiles = 100; 
        map.computeVisibility(px, py, fovTiles);

        hasKey = false;
        spawnBoss();
        
    } 
    else {
        // NIVELES NORMALES (1, 2, 3)
        const float WORLD_SCALE = 1.2f;
        int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
        int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

        std::cout << "[Level] " << level << "/" << maxLevels
                  << " (seed nivel: " << levelSeed << ")\n";

        map.generate(tilesX, tilesY, levelSeed);
        map.setFogEnabled(true); // Asegurar niebla activada

        fovTiles = defaultFovFromViewport();

        auto r = map.firstRoom();
        if (r.w > 0 && r.h > 0) {
            px = r.x + r.w / 2;
            py = r.y + r.h / 2;
        } else {
            px = tilesX / 2;
            py = tilesY / 2;
        }

        player.setGridPos(px, py);
        map.computeVisibility(px, py, getFovRadius());

        Vector2 playerCenterPx = {px * (float)tileSize + tileSize / 2.0f,
                                  py * (float)tileSize + tileSize / 2.0f};
        camera.target = playerCenterPx;
        clampCameraToMap();

        hasKey = false; 
        spawnEnemiesForLevel();
        
        // Generar items
        std::vector<IVec2> enemyTiles;
        for (const auto &e : enemies) enemyTiles.push_back({e.getX(), e.getY()});
        auto isWalkable = [&](int x, int y) { return map.isWalkable(x, y); };
        IVec2 spawnTile{px, py};
        auto [exitX, exitY] = map.findExitTile();
        IVec2 exitTile{exitX, exitY};

        items = ItemSpawner::generate(map.width(), map.height(), isWalkable,
                                      spawnTile, exitTile, enemyTiles,
                                      level, rng, runCtx);
    }
}

// Spawn del Boss
void Game::spawnBoss() {
    boss.active = true;
    boss.awakened = false; // Empieza dormido
    
    // Posición del Boss: Centro arriba
    boss.x = map.width() / 2;
    boss.y = 4; 
    
    // NUEVO: Guardamos dónde está el jugador al entrar
    boss.playerStartX = px;
    boss.playerStartY = py;

    // Configurar Vida según Dificultad
    int baseHp = 800;
    float diffMult = (difficulty == Difficulty::Easy) ? 0.5f : (difficulty == Difficulty::Hard) ? 1.5f : 1.0f;
    
    boss.maxHp = (int)(baseHp * diffMult);
    boss.hp = boss.maxHp;
    
    boss.phase = 1;
    boss.actionCooldown = 2.0f;
    boss.moveTimer = 1.0f; 
    boss.facing = Boss::DOWN;
    
    std::cout << "[BOSS] SPAWNED. Waiting for movement...\n";
}

void Game::updateBoss(float dt) {
    // 0. Verificar muerte
    if (boss.hp <= 0) {
        boss.active = false;
        spawnExplosion({(float)boss.x*tileSize, (float)boss.y*tileSize}, 200, GOLD);
        state = GameState::Victory;
        PlaySound(sfxWin);
        return;
    }

    boss.flashTimer = std::max(0.0f, boss.flashTimer - dt);

    // 1. MECÁNICA DE DESPERTAR (CORREGIDA)
    // Ahora detecta si tu posición (px, py) es distinta a la inicial.
    // Esto funciona con teclado, mando o cualquier input.
    if (!boss.awakened) {
        if (px != boss.playerStartX || py != boss.playerStartY) {
            boss.awakened = true;
            PlaySound(sfxExplosion); // Rugido
            spawnFloatingText({(float)boss.x*tileSize, (float)boss.y*tileSize - 20}, 0, RED); 
            std::cout << "[BOSS] AWAKENED! TIEMBLA MORTAL!\n";
        } else {
            // Si el jugador dispara al boss dormido, lo despierta también
            if (boss.hp < boss.maxHp) boss.awakened = true;
            else return; // Sigue durmiendo
        }
    }

    // 2. CONFIGURACIÓN DE FASES
    float hpPercent = (float)boss.hp / (float)boss.maxHp;
    
    float phaseMoveDelay = 1.0f; 
    float phaseFireDelay = 2.0f; 
    int   phaseDmg       = 1;

    // FASE 1 (100-75%)
    if (hpPercent > 0.75f) {
        boss.phase = 1; phaseMoveDelay = 0.8f; phaseFireDelay = 2.0f; phaseDmg = 1;
    } 
    // FASE 2 (75-50%)
    else if (hpPercent > 0.50f) {
        boss.phase = 2; phaseMoveDelay = 0.6f; phaseFireDelay = 1.5f; phaseDmg = 2;
    }
    // FASE 3 (50-25%)
    else if (hpPercent > 0.25f) {
        boss.phase = 3; phaseMoveDelay = 0.4f; phaseFireDelay = 1.0f; phaseDmg = 3;
    }
    // FASE 4 (<25%)
    else {
        boss.phase = 4; phaseMoveDelay = 0.25f; phaseFireDelay = 0.6f; phaseDmg = 4;
        if (GetRandomValue(0, 10) < 2) spawnExplosion({(float)boss.x*tileSize, (float)boss.y*tileSize}, 1, RED);
    }

    // Ajustes por Dificultad
    float diffSpeedMult = 1.0f, diffFireMult = 1.0f; int diffDmgBonus = 0;
    if (difficulty == Difficulty::Easy) { diffSpeedMult = 1.3f; diffFireMult = 1.5f; }
    if (difficulty == Difficulty::Hard) { diffSpeedMult = 0.8f; diffFireMult = 0.7f; diffDmgBonus = 1; }

    float finalMoveDelay = phaseMoveDelay * diffSpeedMult;
    float finalFireDelay = phaseFireDelay * diffFireMult;
    int   finalDmg       = phaseDmg + diffDmgBonus;

    // 3. MOVIMIENTO
    boss.moveTimer -= dt;
    if (boss.moveTimer <= 0.0f) {
        boss.moveTimer = finalMoveDelay;
        int dx = px - boss.x, dy = py - boss.y;
        
        if (std::abs(dx) > 1 || std::abs(dy) > 1) {
            int stepX = 0, stepY = 0;
            if (std::abs(dx) >= std::abs(dy)) stepX = (dx > 0) ? 1 : -1;
            else stepY = (dy > 0) ? 1 : -1;

            if (map.isWalkable(boss.x + stepX, boss.y + stepY)) {
                boss.x += stepX; boss.y += stepY;
            } else {
                if (stepX != 0) stepY = (dy > 0) ? 1 : -1; else stepX = (dx > 0) ? 1 : -1;
                if (map.isWalkable(boss.x + stepX, boss.y + stepY)) {
                    boss.x += stepX; boss.y += stepY;
                }
            }
        }
        if (std::abs(dx) > std::abs(dy)) boss.facing = (dx > 0) ? Boss::RIGHT : Boss::LEFT;
        else boss.facing = (dy > 0) ? Boss::DOWN : Boss::UP;
    }

    // 4. ATAQUE (DISPARO)
    boss.actionCooldown -= dt;
    if (boss.actionCooldown <= 0.0f) {
        boss.actionCooldown = finalFireDelay;

        auto shoot = [&](float vx, float vy) {
            Projectile p;
            p.isEnemy = true; 
            p.damage = finalDmg;
            p.pos = {(float)boss.x * tileSize + tileSize/2.0f, (float)boss.y * tileSize + tileSize/2.0f};
            p.maxDistance = 1000.0f;
            p.vel = {vx, vy};
            projectiles.push_back(p);
        };

        float speed = 250.0f + (boss.phase * 50.0f);
        float vx = 0, vy = 0;
        switch(boss.facing) {
            case Boss::UP:    vy = -speed; break;
            case Boss::DOWN:  vy =  speed; break;
            case Boss::LEFT:  vx = -speed; break;
            case Boss::RIGHT: vx =  speed; break;
        }
        
        shoot(vx, vy);
        if (boss.phase >= 3) shoot(-vx, -vy);
        if (boss.phase == 4) { shoot(vy, vx); shoot(-vy, -vx); PlaySound(sfxExplosion); }
    }

    // 5. COLISIÓN MELEE
    if (std::abs(px - boss.x) <= 1 && std::abs(py - boss.y) <= 1) {
        if (damageCooldown <= 0.0f) {
            takeDamage(finalDmg + 1);
            damageCooldown = DAMAGE_COOLDOWN;
            int pushX = (px > boss.x) ? 2 : (px < boss.x ? -2 : 0);
            int pushY = (py > boss.y) ? 2 : (py < boss.y ? -2 : 0);
            if (map.isWalkable(px + pushX, py + pushY)) { px += pushX; py += pushY; }
            else if (map.isWalkable(px + pushX/2, py + pushY/2)) { px += pushX/2; py += pushY/2; }
            spawnFloatingText({(float)px*tileSize, (float)py*tileSize}, finalDmg+1, RED);
        }
    }
}

// Implementación del toggle
void Game::toggleGodMode(bool enable) {
    godMode = enable;
    
    // Le decimos al mapa que lo revele todo (o vuelva a la normalidad)
    map.setRevealAll(godMode);

    if (godMode) {
        std::cout << "[GOD MODE] ACTIVADO - IDDQD\n";

         PlaySound(sfxPowerUp); 
        // Mensaje visual (puedes usar tu sistema de texto flotante)
        spawnFloatingText({(float)px*tileSize, (float)py*tileSize}, 9999, GOLD); // Truco visual

        // Opcional: Curar al jugador
        hp = hpMax; 
    } else {
        std::cout << "[GOD MODE] DESACTIVADO\n";
        // Al desactivar, forzamos un recálculo de visión para que
        // la niebla vuelva a aparecer correctamente alrededor del jugador.
        // Sonido de error/apagado (ej. Hurt o Loose)
        PlaySound(sfxLoose);
        map.computeVisibility(px, py, getFovRadius());
    }
}

void Game::run() {
    while (!WindowShouldClose() && !gQuitRequested) {
        processInput();
        update();
        render();

        // --- GESTIÓN DE MÚSICA AMBIENTE ---
        // Solo suena si estamos jugando. Si salimos al menú, se calla.
        if (state == GameState::Playing || state == GameState::Paused) {
            if (!IsSoundPlaying(sfxAmbient)) {
                PlaySound(sfxAmbient);
            }
            
            // Truco: Si está en pausa, pausamos el stream de audio
            if (state == GameState::Paused) ResumeSound(sfxAmbient);
            // Mejor lógica:
            if (state == GameState::Paused) {
                 // ilencio total:
                 if (IsSoundPlaying(sfxAmbient)) PauseSound(sfxAmbient);
            } else {
                 if (!IsSoundPlaying(sfxAmbient)) ResumeSound(sfxAmbient);
            }

        } else {
            // Menú principal / GameOver / Victory
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

void Game::update() {
    if (state != GameState::Playing) return;
    if (showGodModeInput) return;

    float dt = GetFrameTime();

    // --- CORRECCIÓN BUG DASH ---
    if (isDashing) {
        dashTimer -= dt;
        
        // Efectos visuales
        if (GetRandomValue(0, 10) < 8) { 
             float t = 1.0f - (dashTimer / DASH_DURATION);
             Vector2 lerpPos = {
                 dashStartPos.x + (dashEndPos.x - dashStartPos.x) * t,
                 dashStartPos.y + (dashEndPos.y - dashStartPos.y) * t
             };
             // spawnExplosion(lerpPos, 1, SKYBLUE); 
        }

        if (dashTimer <= 0.0f) isDashing = false;
        
        // CORRECCIÓN CLAVE: 
        // Solo movemos la cámara si NO estamos en el nivel del boss.
        // Si es el boss, la cámara se queda quieta.
        if (currentLevel != maxLevels) {
            Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                               py * (float)tileSize + tileSize / 2.0f};
            camera.target.x = desired.x; 
            camera.target.y = desired.y;
            clampCameraToMap();
        }
        
        return; // Salimos mientras dashea
    }
    // ---------------------------

    dashCooldownTimer = std::max(0.0f, dashCooldownTimer - dt);
    tryAutoPickup();
    
    if (hasShield) { shieldTimer -= dt; if (shieldTimer <= 0.0f) hasShield = false; }
    if (glassesTimer > 0.0f) { glassesTimer -= dt; if (glassesTimer <= 0.0f) recomputeFovIfNeeded(); }
    if (slashActive) { slashTimer -= dt; if (slashTimer <= 0.0f) slashActive = false; }

    for (auto &t : enemyFlashTimer) t = std::max(0.0f, t - dt);
    for (auto &e : enemies) e.updateAnimation(dt);

    if (boss.active) {
        updateBoss(dt);
    } else {
        updateShooters(dt);
    }

    updateProjectiles(dt);
    updateFloatingTexts(dt);
    updateParticles(dt);

    if (currentLevel < maxLevels && map.at(px, py) == EXIT) {
        if (hasKey) onExitReached();
    }

    // Cámara Suave (Solo en niveles normales)
    if (currentLevel != maxLevels) {
        auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                           py * (float)tileSize + tileSize / 2.0f};
        float smooth = 10.0f * dt; 
        camera.target.x = Lerp(camera.target.x, desired.x, smooth);
        camera.target.y = Lerp(camera.target.y, desired.y, smooth);
        clampCameraToMap(); 
    }

    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        float intensity = 5.0f;
        camera.target.x += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
        camera.target.y += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
    }

    if (hp <= 0) {
        if (hasBattery) {
            hasBattery = false; hp = hpMax / 2; if (hp < 1) hp = 1;
            std::cout << "[Bateria] Resucitado.\n";
            damageCooldown = 2.0f; shakeTimer = 0.5f; PlaySound(sfxPowerUp);
        } else {
            state = GameState::GameOver; gAttack.swinging = false; gAttack.lastTiles.clear();
            PlaySound(sfxLoose);
        }
        return;
    }

    damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
    for (auto &cd : enemyAtkCD) cd = std::max(0.0f, cd - dt);
    for (auto &cd : enemyShootCD) cd = std::max(0.0f, cd - dt); 
}

void Game::onExitReached() {
    // Si completamos nivel 3, vamos al 4 (Boss)
    if (currentLevel < maxLevels) {
        currentLevel++;
        newLevel(currentLevel); 
    }
}

// Cambia la dificultad actual de forma cíclica. Al llegar al final vuelve al primero.
void Game::cycleDifficulty() {
    if (difficulty == Difficulty::Easy) {
        difficulty = Difficulty::Medium;
    }
    else if (difficulty == Difficulty::Medium) {
        difficulty = Difficulty::Hard;
    }
    else {
        difficulty = Difficulty::Easy;
    }
}

// Devuelve una cadena estática con el nombre de la dificultad, usada en el menú.
const char *Game::getDifficultyLabel(Difficulty d) const {
    switch (d) {
        case Difficulty::Easy:   return "Dificultad: Fácil";
        case Difficulty::Medium: return "Dificultad: Normal";
        default:                 return "Dificultad: Difícil";
    }
}

void Game::drawBoss() const {
    if (!boss.active) return;

    Texture2D tex = itemSprites.bossDownIdle; // Default

    // Selección de sprite según facing y animación (simplificado)
    // Aquí podrías implementar el ciclo de bossWalk1/2 usando boss.animTimer
    switch(boss.facing) {
        case Boss::UP:    tex = itemSprites.bossUpIdle; break;
        case Boss::DOWN:  tex = itemSprites.bossDownIdle; break;
        case Boss::LEFT:  tex = itemSprites.bossLeftIdle; break;
        case Boss::RIGHT: tex = itemSprites.bossRightIdle; break;
    }

    // Dibujado Gigante (Scale 3x)
    float scale = 3.0f;
    float spriteW = (float)tex.width * scale;
    float spriteH = (float)tex.height * scale;

    // Posición en pantalla (Centro del tile lógico)
    Vector2 centerPos = {(float)boss.x * tileSize + tileSize/2, (float)boss.y * tileSize + tileSize/2};
    
    // Destino: Centrado en esa posición lógica
    Rectangle dest = {
        centerPos.x - spriteW/2,
        centerPos.y - spriteH/2, // Ajuste para que los pies coincidan mejor si fuera necesario
        spriteW,
        spriteH
    };

    Vector2 origin = {0,0};
    Color tint = WHITE;
    
    // Feedback de golpe
    if (boss.flashTimer > 0.0f) tint = RED;
    // Feedback de fase (se pone más rojo/oscuro cuanto más enfadado)
    if (boss.phase == 2) tint = {255, 200, 200, 255};
    if (boss.phase == 3) tint = {255, 100, 100, 255};

    DrawTexturePro(tex, {0,0,(float)tex.width,(float)tex.height}, dest, origin, 0.0f, tint);

    // Barra de vida del Boss (Estilo Boss de RPG: Barra grande arriba o abajo)
    // La dibujamos sobre su cabeza
    float barW = 100.0f;
    float barH = 10.0f;
    float barX = centerPos.x - barW/2;
    float barY = dest.y - 20;

    DrawRectangle(barX, barY, barW, barH, BLACK);
    float fill = ((float)boss.hp / (float)boss.maxHp) * barW;
    DrawRectangle(barX, barY, fill, barH, PURPLE); // Color Boss
    DrawRectangleLines(barX, barY, barW, barH, WHITE);
}
