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
    audioVolume = 0.2f;
    SetMasterVolume(audioVolume); // Volumen general al 20%

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
const char *Game::getDifficultyLabel() const {
    switch (difficulty) {
        case Difficulty::Easy:   return "Dificultad: Fácil";
        case Difficulty::Medium: return "Dificultad: Normal";
        default:                 return "Dificultad: Difícil";
    }
}

std::string Game::getVolumeLabel() const {
    int pct = (int)std::round(audioVolume * 100.0f);
    pct = std::clamp(pct, 0, 100);
    return "Volumen: " + std::to_string(pct) + "%";
}
