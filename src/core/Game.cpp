#include "Game.hpp"
#include "raylib.h"
#include <algorithm> // para std::min y std::clamp
#include <cmath>     // para std::floor
#include <ctime>
#include <iostream>
#include <climits>
#include "GameUtils.hpp"
#include "AssetPath.hpp"

static inline unsigned now_seed() {
    return static_cast<unsigned>(time(nullptr));
}

static bool gQuitRequested = false;

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

static bool loadTextAsset(const char *relPath, std::string &outText) {
    const std::string full = assetPath(relPath);
    char *buf = LoadFileText(full.c_str());
    if (buf) {
        outText.assign(buf);
        UnloadFileText(buf);
        return true;
    }
    else {
        outText = "No se encontro '" + full + "'.\n";
        std::cerr << "[ASSETS] No se pudo cargar texto desde: " << full << "\n";
        return false;
    }
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
    keycard = loadTex("assets/sprites/items/item_keycard.png");
    shield = loadTex("assets/sprites/items/item_shield.png");
    pila = loadTex("assets/sprites/items/item_healthbattery.png");
    glasses = loadTex("assets/sprites/items/item_glasses.png");
    swordBlue = loadTex("assets/sprites/items/item_sword_blue.png");
    swordGreen = loadTex("assets/sprites/items/item_sword_green.png");
    swordRed = loadTex("assets/sprites/items/item_sword_red.png");
    plasma1 = loadTex("assets/sprites/items/item_plasma1.png");
    plasma2 = loadTex("assets/sprites/items/item_plasma2.png");
    battery = loadTex("assets/sprites/items/item_battery.png");
    
    enemy = loadTex("assets/sprites/enemies/enemy.png");
    enemyUp = loadTex("assets/sprites/enemies/enemy_up.png");
    enemyDown = loadTex("assets/sprites/enemies/enemy_down.png");
    enemyLeft = loadTex("assets/sprites/enemies/enemy_left.png");
    enemyRight = loadTex("assets/sprites/enemies/enemy_right.png");
    
    loaded = true;
}

void ItemSprites::unload() {
    if (!loaded) return;
    
    // Descargar las procedurales
    UnloadTexture(wall);
    UnloadTexture(floor);

    // Descargar el resto
    UnloadTexture(keycard);
    UnloadTexture(shield);
    UnloadTexture(pila);
    UnloadTexture(glasses);
    UnloadTexture(swordBlue);
    UnloadTexture(swordGreen);
    UnloadTexture(swordRed);
    UnloadTexture(plasma1);
    UnloadTexture(plasma2);
    UnloadTexture(battery);
    UnloadTexture(enemy);
    UnloadTexture(enemyUp);
    UnloadTexture(enemyDown);
    UnloadTexture(enemyLeft);
    UnloadTexture(enemyRight);
    
    loaded = false;
}

// Tipos de sonido para el generador
enum SoundType { 
    SND_HIT, SND_EXPLOSION, SND_PICKUP, SND_POWERUP, 
    SND_HURT, SND_WIN, SND_LOOSE, SND_AMBIENT, SND_DASH
};

// Generador de ondas procedurales (Sintetizador)
Sound Game::generateSound(int type) {
    const int sampleRate = 44100;
    
    // CAMBIO: 60 segundos exactos.
    const int durationFrames = (type == SND_AMBIENT) ? 44100 * 60 : 
                               (type == SND_WIN || type == SND_LOOSE) ? 44100 * 4 : 
                               44100 / 2;
    
    Wave wave;
    wave.frameCount = durationFrames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = malloc(wave.frameCount * wave.sampleSize / 8);
    
    short *data = (short *)wave.data;
    
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / sampleRate; // Tiempo en segundos
        float val = 0.0f;
        
        switch (type) {
            case SND_HIT: // Ruido blanco corto + Onda cuadrada baja
                if (t < 0.1f) val = ((float)GetRandomValue(-100, 100) / 100.0f) * (1.0f - t/0.1f);
                else val = (sinf(2 * PI * 100 * t) > 0 ? 0.5f : -0.5f) * (1.0f - t);
                break;
                
            case SND_EXPLOSION: // Ruido puro con decay largo
                val = ((float)GetRandomValue(-100, 100) / 100.0f);
                val *= (1.0f - (float)i / wave.frameCount); // Fade out lineal
                break;
                
            case SND_PICKUP: // Onda cuadrada aguda (Coin)
                // Frecuencia sube de 800 a 1200
                {
                    float freq = 800.0f + (t * 4000.0f); 
                    val = (sinf(2 * PI * freq * t) > 0 ? 0.3f : -0.3f);
                    val *= (1.0f - t*4.0f); // Muy corto
                }
                break;
                
            case SND_POWERUP: // Onda triangular subiendo (Powerup)
                {
                    float freq = 300.0f + (t * 1000.0f);
                    val = asinf(sinf(2 * PI * freq * t)) * 0.5f; 
                }
                break;
                
            case SND_HURT: // Sierra descendente (Auch)
                {
                    float freq = 200.0f - (t * 400.0f);
                    if (freq < 50) freq = 50;
                    val = (float)(fmod(t * freq, 1.0) * 2.0 - 1.0) * 0.5f;
                }
                break;

            case SND_AMBIENT: 
                {
                    // --- TEMA: "CYBER COMBAT LOOP" (60s Acción Pura) ---
                    // Sin intro suave. Directo al grano.
                    
                    float bpm = 120.0f;
                    float beatLen = 60.0f / bpm; // 0.5 segundos exactos
                    
                    // Variables de tiempo
                    float measurePos = t / (beatLen * 4.0f); 
                    int measure = (int)measurePos;           
                    float beatPos = fmod(t, beatLen);        
                    float subBeat = fmod(t, beatLen / 4.0f); 
                    
                    // --- 1. KICK (EL MOTOR) ---
                    // Suena DESDE EL PRINCIPIO. Golpe seco y potente.
                    float kick = 0.0f;
                    if (beatPos < 0.2f) {
                        // Frecuencia estable para que pegue fuerte
                        float kFreq = 100.0f - (beatPos * 400.0f);
                        if (kFreq < 40.0f) kFreq = 40.0f;
                        kick = sinf(2 * PI * kFreq * t);
                        kick *= expf(-15.0f * beatPos); 
                        kick *= 0.9f; // Volumen alto
                    }

                    // --- 2. BASSLINE (LA ENERGÍA) ---
                    // Bajo "Rolling" constante desde el segundo 0.
                    float bassFreq = 55.0f; // La (A1)
                    
                    // Pequeña variación de nota cada 8 compases (16s) para no aburrir
                    if ((measure / 8) % 2 == 1) bassFreq = 43.65f; // Fa (F1)

                    // Onda de Sierra filtrada (agresiva pero controlada)
                    float saw = (fmod(t * bassFreq, 1.0f) * 2.0f) - 1.0f;
                    float bass = (saw * 0.4f) + (sinf(2 * PI * bassFreq * t) * 0.6f);
                    
                    // SIDECHAIN: El bajo se "agacha" cuando pega el Kick
                    float sidechain = fmod(t, beatLen) / beatLen; 
                    sidechain = powf(sidechain, 0.5f); // Curva rápida
                    bass *= sidechain * 0.6f;

                    // --- 3. HI-HATS (VELOCIDAD) ---
                    // Ritmo de semicorcheas constante "tik-tik-tik-tik"
                    float noise = ((float)GetRandomValue(-100, 100) / 100.0f);
                    // Filtro High-Pass simulado (solo frecuencias altas)
                    noise *= sinf(2 * PI * 6000.0f * t);
                    
                    float hat = noise * expf(-80.0f * subBeat) * 0.15f;
                    // Acento en el "off-beat" (el "y" del 1 y 2...)
                    if (fmod(t, beatLen) > beatLen/2.0f) hat *= 1.8f; 

                    // --- 4. ARPEGIO (LA MELODÍA) ---
                    // Entra y sale para dar variedad, pero el ritmo de fondo sigue.
                    float arp = 0.0f;
                    
                    // La melodía suena en los segundos 0-15 y 30-45. 
                    // Deja huecos de "solo ritmo" en 15-30 y 45-60.
                    bool playArp = ((int)(t / 16.0f) % 2 == 0); 
                    
                    if (playArp) {
                        int noteIdx = (int)(t * 8.0f); // 8 notas/segundo
                        // Patrón simple y pegadizo
                        float scale[] = { 1.0f, 1.0f, 1.5f, 1.2f, 2.0f, 1.5f, 1.2f, 1.5f };
                        int idx = noteIdx % 8;
                        float arpFreq = bassFreq * 4.0f * scale[idx];
                        
                        // Onda "Cristal"
                        arp = sinf(2 * PI * arpFreq * t);
                        // Delay
                        arp += sinf(2 * PI * arpFreq * (t - 0.25f)) * 0.4f;
                        
                        float env = fmod(t, 0.125f) / 0.125f;
                        arp *= (1.0f - env) * 0.15f; // Volumen sutil
                    }

                    // --- MEZCLA FINAL ---
                    val = kick + bass + hat + arp;
                    
                    // Compresor/Limitador
                    if (val > 0.9f) val = 0.9f;
                    if (val < -0.9f) val = -0.9f;
                    
                    // Fade micro-rápido en los bordes del bucle (0.05s) para evitar "clicks"
                    if (t < 0.05f) val *= (t / 0.05f);
                    if (t > 59.95f) val *= (60.0f - t) / 0.05f;
                }
                break;
            case SND_DASH: // Ruido rosa/aire rápido
                {
                    // Ruido aleatorio suave
                    float noise = ((float)GetRandomValue(-100, 100) / 100.0f);
                    // Volumen baja muy rápido (0.2 segundos)
                    val = noise * 0.4f * (1.0f - t/0.2f); 
                    if (val < 0) val = 0;
                }
                break;

            case SND_WIN: // Acorde mayor arpegiado
                {
                    float note = 440.0f; // La
                    if (t > 0.2) note = 554.37f; // Do#
                    if (t > 0.4) note = 659.25f; // Mi
                    if (t > 0.6) note = 880.0f;  // La (8va)
                    val = sinf(2 * PI * note * t) * 0.5f;
                    val *= (1.0f - t/2.0f);
                }
                break;

            case SND_LOOSE: // Tono descendente triste
                {
                    float freq = 300.0f * (1.0f - t/2.0f);
                    val = (sinf(2 * PI * freq * t) > 0 ? 0.4f : -0.4f);
                }
                break;
        }
        
        data[i] = (short)(val * 32000.0f);
    }
    
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
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

int Game::getFovRadius() const {
    int r = fovTiles;
    if (glassesTimer > 0.0f) {
        r += glassesFovMod;
    }
    return std::clamp(r, 2, 30);
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
    CloseWindow();
}

void Game::processInput() {
    if (state != GameState::MainMenu) {
        bool escPressed = IsKeyPressed(KEY_ESCAPE);
        const int gp0 = 0;
        if (IsGamepadAvailable(gp0)) {
            escPressed = escPressed ||
                         IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                         IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_MIDDLE_RIGHT) ||
                         IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_MIDDLE);
        }
        if (escPressed) {
            showHelp = false;
            gAttack.swinging = false;
            gAttack.lastTiles.clear();
            state = GameState::MainMenu;
            mainMenuSelection = 0;   
            return;
        }
    }

    auto restartRun = [&]() {
        if (fixedSeed == 0) runSeed = nextRunSeed();
        newRun();
    };

    if (IsKeyPressed(KEY_R)) {
        restartRun();
        return;
    }

    const int gpRestart = 0;
    if (state != GameState::MainMenu &&
        IsGamepadAvailable(gpRestart) &&
        IsGamepadButtonPressed(gpRestart, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
        restartRun();
        return;
    }

    if (IsKeyPressed(KEY_T)) {
        moveMode = (moveMode == MovementMode::StepByStep)
                   ? MovementMode::RepeatCooldown
                   : MovementMode::StepByStep;
        moveCooldown = 0.0f;
    }

    if (IsKeyPressed(KEY_F2)) {
        fogEnabled = !fogEnabled;
        map.setFogEnabled(fogEnabled);
        if (fogEnabled) map.computeVisibility(px, py, getFovRadius());
    }

    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        fovTiles = std::max(2, fovTiles - 1);
        if (map.fogEnabled()) map.computeVisibility(px, py, getFovRadius());
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        fovTiles = std::min(30, fovTiles + 1);
        if (map.fogEnabled()) map.computeVisibility(px, py, getFovRadius());
    }

    const float dt = GetFrameTime();
    if (state == GameState::MainMenu) {
        handleMenuInput();
        return;
    }
    if (state == GameState::Playing) {
        handlePlayingInput(dt);
        return;
    }

    player.update(dt, false);
}

void Game::handleMenuInput() {
    const int menuGamepad = 0;

    if (showHelp) {
        // ... (Tu código de menú de ayuda, igual que antes) ...
        int panelW = (int)std::round(screenW * 0.86f);
        int panelH = (int)std::round(screenH * 0.76f);
        if (panelW > 1500) panelW = 1500;
        if (panelH > 900)  panelH = 900;
        int pxl = (screenW - panelW) / 2;
        int pyl = (screenH - panelH) / 2;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            helpScroll -= (int)(wheel * 40);
            if (helpScroll < 0) helpScroll = 0;
        }

        if (IsGamepadAvailable(menuGamepad)) {
            float ay = GetGamepadAxisMovement(menuGamepad, GAMEPAD_AXIS_LEFT_Y);
            const float dead = 0.25f;
            if (std::fabs(ay) > dead) {
                const float speed = 400.0f; 
                helpScroll += (int)(ay * speed * GetFrameTime());
                if (helpScroll < 0) helpScroll = 0;
            }
        }

        int margin    = 24;
        int titleSize = (int)std::round(panelH * 0.06f);
        int top       = pyl + margin + titleSize + 16;
        int backFs    = std::max(16, (int)std::round(panelH * 0.048f));
        int footerH   = backFs + 16;
        int viewportH = panelH - (top - pyl) - margin - footerH;
        int fontSize  = (int)std::round(screenH * 0.022f);
        if (fontSize < 16) fontSize = 16;
        int lineH = (int)std::round(fontSize * 1.25f);
        int lines = 1;
        for (char c : helpText) if (c == '\n') ++lines;
        int maxScroll = std::max(0, lines * lineH - viewportH);
        if (helpScroll > maxScroll) helpScroll = maxScroll;

        const char *backTxt = "VOLVER";
        int tw = MeasureText(backTxt, backFs);
        int tx = pxl + panelW - tw - 16;
        int ty = pyl + panelH - backFs - 12;
        Rectangle backHit = { (float)(tx - 6), (float)(ty - 4),
                              (float)(tw + 12), (float)(backFs + 8) };

        bool clickBack = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                         CheckCollisionPointRec(GetMousePosition(), backHit);
        bool escBack   = IsKeyPressed(KEY_ESCAPE);
        bool gpBack    = false;

        if (IsGamepadAvailable(menuGamepad)) {
            gpBack = IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || 
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)  || 
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);    
        }

        if (clickBack || escBack || gpBack) {
            showHelp = false;
        }
        return; 
    }

    int bw = (int)std::round(screenW * 0.35f);
    bw = std::clamp(bw, 320, 560);
    int bh = (int)std::round(screenH * 0.12f);
    bh = std::clamp(bh, 72, 120);
    int startY = (int)std::round(screenH * 0.52f);
    int gap    = (int)std::round(screenH * 0.04f);

    Rectangle playBtn = { (float)((screenW - bw) / 2), (float)startY,
                          (float)bw, (float)bh };
    Rectangle readBtn = { (float)((screenW - bw) / 2),
                          (float)(startY + bh + gap), (float)bw, (float)bh };
    Rectangle quitBtn = { (float)((screenW - bw) / 2),
                          (float)(startY + (bh + gap) * 2), (float)bw, (float)bh };

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        if (CheckCollisionPointRec(mp, playBtn)) {
            newRun();
            return;
        }
        if (CheckCollisionPointRec(mp, readBtn)) {
            if (helpText.empty()) loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp  = true;
            helpScroll = 0;
            return;
        }
        if (CheckCollisionPointRec(mp, quitBtn)) {
            gQuitRequested = true;
            return;
        }
    }

    auto activateSelection = [&]() {
        if (mainMenuSelection == 0) {
            newRun();
        }
        else if (mainMenuSelection == 1) {
            if (helpText.empty()) loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp  = true;
            helpScroll = 0;
        }
        else { 
            gQuitRequested = true;
        }
    };

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        mainMenuSelection = (mainMenuSelection + 3 - 1) % 3;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        mainMenuSelection = (mainMenuSelection + 1) % 3;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        activateSelection();
        return;
    }

    if (IsGamepadAvailable(menuGamepad)) {
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
            mainMenuSelection = (mainMenuSelection + 3 - 1) % 3;
        }
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
            mainMenuSelection = (mainMenuSelection + 1) % 3;
        }

        static bool stickNeutral = true;
        float ay = GetGamepadAxisMovement(menuGamepad, GAMEPAD_AXIS_LEFT_Y);
        const float dead = 0.35f;

        if (std::fabs(ay) < dead) {
            stickNeutral = true;
        }
        else if (stickNeutral) {
            if (ay < 0.0f) mainMenuSelection = (mainMenuSelection + 3 - 1) % 3;
            else mainMenuSelection = (mainMenuSelection + 1) % 3;
            stickNeutral = false;
        }

        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            activateSelection();
            return;
        }
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
            if (helpText.empty()) loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp  = true;
            helpScroll = 0;
            return;
        }
    }
}

// ----------------------------------------------------------------------------------
// INPUT JUEGO (COMBATE Y MOVIMIENTO)
// ----------------------------------------------------------------------------------
void Game::handlePlayingInput(float dt) {
    // --------------------------------------------------------
    // 1. DETECCIÓN DE DISPOSITIVO
    // --------------------------------------------------------
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
        IsKeyDown(KEY_E) || IsKeyDown(KEY_I) || IsKeyDown(KEY_O) || 
        IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_ONE) || IsKeyDown(KEY_TWO) || 
        IsKeyDown(KEY_LEFT_SHIFT)) { // Añadido Shift
        lastInput = InputDevice::Keyboard;
    }

    const int gpId = 0;
    if (IsGamepadAvailable(gpId)) {
        bool gpActive = false;
        // Check botones
        for (int b = GAMEPAD_BUTTON_UNKNOWN + 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++) {
            if (IsGamepadButtonDown(gpId, b)) { gpActive = true; break; }
        }
        // Check ejes
        if (!gpActive) {
            float lx = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X);
            float ly = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
            if (std::abs(lx) > 0.25f || std::abs(ly) > 0.25f) gpActive = true;
        }
        // Check gatillos
        if (!gpActive) {
             float rt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_TRIGGER);
             float lt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_TRIGGER);
             if (rt > 0.1f || lt > 0.1f) gpActive = true;
        }

        if (gpActive) lastInput = InputDevice::Gamepad;
    }

    // --------------------------------------------------------
    // 2. LÓGICA DE JUEGO (Zoom, Interacción)
    // --------------------------------------------------------
    if (IsKeyDown(KEY_I)) cameraZoom += 1.0f * dt;
    if (IsKeyDown(KEY_O)) cameraZoom -= 1.0f * dt;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) cameraZoom = expf(logf(cameraZoom) + wheel * 0.1f);

    if (IsGamepadAvailable(gpId)) {
        float rightY = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_Y);
        const float zoomThr = 0.2f;
        if (rightY < -zoomThr) cameraZoom += (-rightY) * dt;
        else if (rightY > zoomThr) cameraZoom -= rightY * dt;
        
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_THUMB)) {
            cameraZoom = 1.0f;
            camera.zoom = cameraZoom;
            camera.rotation = 0.0f;
            clampCameraToMap();
        }
    }

    cameraZoom = std::clamp(cameraZoom, 0.5f, 3.0f);
    camera.zoom = cameraZoom;
    clampCameraToMap();

    if (IsKeyPressed(KEY_C)) {
        cameraZoom = 1.0f;
        camera.zoom = cameraZoom;
        camera.rotation = 0.0f;
        clampCameraToMap();
    }

    // Interacción (Pickup)
    bool interact = IsKeyPressed(KEY_E);
    if (IsGamepadAvailable(gpId)) {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) interact = true;
    }
    if (interact) tryManualPickup();

    // --------------------------------------------------------
    // 3. DASH (ESQUIVA) - NUEVO BLOQUE
    // --------------------------------------------------------
    bool dashPressed = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);
    if (IsGamepadAvailable(gpId)) {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_TRIGGER_1) || 
            IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
            dashPressed = true;
        }
    }

    if (dashPressed && !isDashing && dashCooldownTimer <= 0.0f) {
        int ddx = 0, ddy = 0;
        // Prioridad: Dirección pulsada ahora
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    ddy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  ddy = 1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  ddx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) ddx = 1;
        
        // Si no, última dirección
        if (ddx == 0 && ddy == 0) {
            ddx = gAttack.lastDir.x;
            ddy = gAttack.lastDir.y;
        }

        if (ddx != 0 || ddy != 0) {
            int targetX = px;
            int targetY = py;
            
            // Calcular destino (hasta DASH_DISTANCE)
            for (int i = 1; i <= DASH_DISTANCE; i++) {
                int nextX = px + ddx * i;
                int nextY = py + ddy * i;
                if (!map.isWalkable(nextX, nextY)) break; // Pared
                
                bool enemyBlock = false;
                for(const auto& e : enemies) if(e.getX() == nextX && e.getY() == nextY) enemyBlock = true;
                if (enemyBlock) break; // Enemigo

                targetX = nextX;
                targetY = nextY;
            }

            if (targetX != px || targetY != py) {
                // Configurar estado Dash
                isDashing = true;
                dashTimer = DASH_DURATION;
                dashCooldownTimer = DASH_COOLDOWN;
                
                dashStartPos = { px * (float)tileSize, py * (float)tileSize };
                dashEndPos = { targetX * (float)tileSize, targetY * (float)tileSize };
                
                px = targetX;
                py = targetY;
                player.setGridPos(px, py);
                
                PlaySound(sfxDash);
                
                onSuccessfulStep(0,0); 
                return; 
            }
        }
    }

    // --------------------------------------------------------
    // 4. MOVIMIENTO NORMAL
    // --------------------------------------------------------
    int dx = 0, dy = 0;
    bool moved = false;

    int gpDx = 0, gpDy = 0;
    bool gpDpadPressed = false;
    bool gpAnalogActive = false;

    if (IsGamepadAvailable(gpId)) {
        float axisX = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X);
        float axisY = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
        const float thr = 0.5f;

        if (axisX < -thr || axisX > thr || axisY < -thr || axisY > thr) {
            gpAnalogActive = true;
            if (axisY < -thr) gpDy = -1; else if (axisY > thr) gpDy = +1;
            if (axisX < -thr) gpDx = -1; else if (axisX > thr) gpDx = +1;
        }

        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_UP)) { gpDx = 0; gpDy = -1; gpDpadPressed = true; }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) { gpDx = 0; gpDy = +1; gpDpadPressed = true; }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) { gpDx = -1; gpDy = 0; gpDpadPressed = true; }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) { gpDx = +1; gpDy = 0; gpDpadPressed = true; }

        if (gpAnalogActive) moveMode = MovementMode::RepeatCooldown;
        else if (gpDpadPressed) moveMode = MovementMode::StepByStep;
    }

    if (moveMode == MovementMode::StepByStep) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    dy = -1;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  dy = +1;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  dx = -1;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) dx = +1;

        if (dx == 0 && dy == 0 && (gpDx != 0 || gpDy != 0)) { dx = gpDx; dy = gpDy; }
        
        if (dx != 0 || dy != 0) {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy); 
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved) onSuccessfulStep(dx, dy);
        }
        player.update(dt, moved);
    }
    else {
        moveCooldown -= dt;
        bool pressedNow = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
                        IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
                        IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                        IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                        gpDpadPressed;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy = +1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx = +1;

        if (dx == 0 && gpDx != 0) dx = gpDx;
        if (dy == 0 && gpDy != 0) dy = gpDy;

        if (dx != 0 || dy != 0) {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy);
        }

        if (pressedNow) {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved) onSuccessfulStep(dx, dy);
            moveCooldown = MOVE_INTERVAL;
        }
        else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f) {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved) onSuccessfulStep(dx, dy);
            moveCooldown = MOVE_INTERVAL;
        }
        player.update(dt, moved);
    }

    // --------------------------------------------------------
    // 5. SISTEMA DE COMBATE (INPUT)
    // --------------------------------------------------------
    gAttack.cdTimer = std::max(0.f, gAttack.cdTimer - dt);
    plasmaCooldown  = std::max(0.f, plasmaCooldown - dt);

    if (gAttack.swinging) {
        gAttack.swingTimer = std::max(0.f, gAttack.swingTimer - dt);
        if (gAttack.swingTimer <= 0.f) {
            gAttack.swinging = false;
            gAttack.lastTiles.clear();
        }
    }

    // A) MANOS
    bool attackHands = IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) attackHands = true;
    
    if (attackHands && gAttack.cdTimer <= 0.f) performMeleeAttack();

    // B) ESPADA
    bool attackSword = IsKeyPressed(KEY_ONE); 
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) attackSword = true;
    
    if (attackSword) {
        if (swordTier > 0) {
            if (gAttack.cdTimer <= 0.f) performSwordAttack();
        } else {
            // std::cout << "No tienes espada\n";
        }
    }

    // C) PLASMA
    bool attackPlasma = IsKeyPressed(KEY_TWO);
    if (IsGamepadAvailable(gpId)) {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) attackPlasma = true;
    }
    if (attackPlasma) {
        if (plasmaTier > 0) {
            if (plasmaCooldown <= 0.f) performPlasmaAttack();
        } else {
            // std::cout << "No tienes plasma\n";
        }
    }

    // Cheats
    if (IsKeyPressed(KEY_H)) { hp = std::max(0, hp - 2); }      
    if (IsKeyPressed(KEY_J)) { hp = std::min(hpMax, hp + 2); }  
}

void Game::clampCameraToMap() {
    const float worldW = map.width() * (float)tileSize;
    const float worldH = map.height() * (float)tileSize;
    const float viewW = screenW / camera.zoom;
    const float viewH = screenH / camera.zoom;
    const float halfW = viewW * 0.5f;
    const float halfH = viewH * 0.5f;

    if (worldW <= viewW) camera.target.x = worldW * 0.5f;
    else camera.target.x = std::clamp(camera.target.x, halfW, worldW - halfW);

    if (worldH <= viewH) camera.target.y = worldH * 0.5f;
    else camera.target.y = std::clamp(camera.target.y, halfH, worldH - halfH);
}

void Game::centerCameraOnPlayer() {
    camera.target = { px * (float)tileSize + tileSize / 2.0f,
                      py * (float)tileSize + tileSize / 2.0f };
    clampCameraToMap();
}

void Game::recomputeFovIfNeeded() {
    if (map.fogEnabled()) {
        map.computeVisibility(px, py, getFovRadius());
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

int Game::defaultFovFromViewport() const {
    int tilesX = screenW / tileSize;
    int tilesY = screenH / tileSize;
    int r = static_cast<int>(std::floor(std::min(tilesX, tilesY) * 0.15f));
    return std::clamp(r, 3, 20);
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
