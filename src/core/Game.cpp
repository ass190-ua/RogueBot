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

Game::Game(unsigned seed) : fixedSeed(seed) {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "RogueBot Alpha"); 
    SetExitKey(KEY_NULL);

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
    }
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
    // 1. DETECCIÓN DE DISPOSITIVO (Input Device Detection)
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
        IsKeyDown(KEY_E) || IsKeyDown(KEY_I) || IsKeyDown(KEY_O) || 
        IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_ONE) || IsKeyDown(KEY_TWO)) {
        lastInput = InputDevice::Keyboard;
    }

    const int gpId = 0;
    if (IsGamepadAvailable(gpId)) {
        bool gpActive = false;
        for (int b = GAMEPAD_BUTTON_UNKNOWN + 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++) {
            if (IsGamepadButtonDown(gpId, b)) {
                gpActive = true;
                break;
            }
        }
        if (!gpActive) {
            float lx = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X);
            float ly = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
            float rx = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_X);
            float ry = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_Y);
            if (std::abs(lx) > 0.25f || std::abs(ly) > 0.25f || 
                std::abs(rx) > 0.25f || std::abs(ry) > 0.25f) {
                gpActive = true;
            }
        }
        if (!gpActive) {
             float rt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_TRIGGER);
             float lt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_TRIGGER);
             if (rt > 0.1f || lt > 0.1f) gpActive = true;
        }

        if (gpActive) {
            lastInput = InputDevice::Gamepad;
        }
    }

    // 2. LÓGICA DE JUEGO (Zoom, Interacción)
    if (IsKeyDown(KEY_I)) cameraZoom += 1.0f * dt;
    if (IsKeyDown(KEY_O)) cameraZoom -= 1.0f * dt;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        cameraZoom = expf(logf(cameraZoom) + wheel * 0.1f);
    }

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
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) { // A / Cruz
            interact = true;
        }
    }
    if (interact) {
        tryManualPickup();
    }

    // 3. MOVIMIENTO
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
            if (axisY < -thr) gpDy = -1;
            else if (axisY > thr) gpDy = +1;
            if (axisX < -thr) gpDx = -1;
            else if (axisX > thr) gpDx = +1;
        }

        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
            gpDx = 0; gpDy = -1; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
            gpDx = 0; gpDy = +1; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
            gpDx = -1; gpDy = 0; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
            gpDx = +1; gpDy = 0; gpDpadPressed = true;
        }

        if (gpAnalogActive) {
            moveMode = MovementMode::RepeatCooldown;
        } else if (gpDpadPressed) {
            moveMode = MovementMode::StepByStep;
        }
    }

    if (moveMode == MovementMode::StepByStep) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    dy = -1;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  dy = +1;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  dx = -1;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) dx = +1;

        if (dx == 0 && dy == 0 && (gpDx != 0 || gpDy != 0)) {
            dx = gpDx;
            dy = gpDy;
        }
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

    // 4. SISTEMA DE COMBATE (INPUT)
    gAttack.cdTimer = std::max(0.f, gAttack.cdTimer - dt);
    plasmaCooldown  = std::max(0.f, plasmaCooldown - dt);

    if (gAttack.swinging) {
        gAttack.swingTimer = std::max(0.f, gAttack.swingTimer - dt);
        if (gAttack.swingTimer <= 0.f) {
            gAttack.swinging = false;
            gAttack.lastTiles.clear();
        }
    }

    // A) MANOS / GOLPE BÁSICO (Espacio / X)
    bool attackHands = IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
        attackHands = true;
    }
    if (attackHands && gAttack.cdTimer <= 0.f) {
        performMeleeAttack();
    }

    // B) ESPADA (1 / RB)
    bool attackSword = IsKeyPressed(KEY_ONE); 
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
        attackSword = true;
    }
    if (attackSword) {
        if (swordTier > 0) {
            if (gAttack.cdTimer <= 0.f) performSwordAttack();
        } else {
            // Feedback opcional visual o sonoro
        }
    }

    // C) PLASMA (2 / RT)
    bool attackPlasma = IsKeyPressed(KEY_TWO);
    if (IsGamepadAvailable(gpId)) {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) attackPlasma = true;
    }
    if (attackPlasma) {
        if (plasmaTier > 0) {
            if (plasmaCooldown <= 0.f) performPlasmaAttack();
        } else {
            // Feedback opcional
        }
    }

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

// --- ARCHIVO: src/core/Game.cpp ---

void Game::update() {
    if (state != GameState::Playing) return;

    tryAutoPickup();
    float dt = GetFrameTime();
    
    // Temporizadores
    if (hasShield) {
        shieldTimer -= dt;
        if (shieldTimer <= 0.0f) hasShield = false;
    }
    if (glassesTimer > 0.0f) {
        glassesTimer -= dt;
        if (glassesTimer <= 0.0f) recomputeFovIfNeeded(); 
    }
    for (auto &t : enemyFlashTimer) t = std::max(0.0f, t - dt);

    updateProjectiles(dt);
    updateFloatingTexts(dt);

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
        } else {
            state = GameState::GameOver;
            gAttack.swinging = false; 
            gAttack.lastTiles.clear();
        }
        return;
    }

    damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
    for (auto &cd : enemyAtkCD) cd = std::max(0.0f, cd - dt);
}

void Game::onExitReached() {
    if (currentLevel < maxLevels) {
        currentLevel++;
        newLevel(currentLevel); 
    }
    else {
        state = GameState::Victory;
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

    map.draw(tileSize);
    drawEnemies();
    player.draw(tileSize, px, py);
    drawItems();
    
    // PROYECTILES (NUEVO)
    drawProjectiles();
    drawFloatingTexts();

    if (gAttack.swinging && !gAttack.lastTiles.empty()) {
        Color fill = {255, 230, 50, 120}; 
        for (const auto &t : gAttack.lastTiles) {
            int xpx = t.x * tileSize;
            int ypx = t.y * tileSize;
            DrawRectangle(xpx, ypx, tileSize, tileSize, fill);
            DrawRectangleLines(xpx, ypx, tileSize, tileSize, YELLOW);
        }
    }

    EndMode2D();

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
        // CORRECCIÓN SOLAPAMIENTO: 
        // El enemigo nunca puede considerar "caminable" la casilla del jugador.
        if (nx == px && ny == py) return false; 

        return nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
               map.isWalkable(nx, ny);
    };

    auto sgn = [](int v) { return (v > 0) - (v < 0); };

    // Lógica Greedy mejorada
    auto greedyNext = [&](int ex, int ey) -> std::pair<int, int> {
        int dx = px - ex, dy = py - ey;
        
        // Si ya está encima (bug extremo), no moverse
        if (dx == 0 && dy == 0) return {ex, ey};

        // Intentar eje mayor primero
        if (std::abs(dx) >= std::abs(dy)) {
            int nx = ex + sgn(dx), ny = ey;
            if (can(nx, ny)) return {nx, ny};
            // Si falla, probar eje menor
            nx = ex; ny = ey + sgn(dy);
            if (can(nx, ny)) return {nx, ny};
        }
        else {
            int nx = ex, ny = ey + sgn(dy);
            if (can(nx, ny)) return {nx, ny};
            // Si falla, probar eje mayor
            nx = ex + sgn(dx); ny = ey;
            if (can(nx, ny)) return {nx, ny};
        }
        
        // Si está bloqueado o el siguiente paso es el jugador (can devuelve false),
        // devolvemos la posición actual pero intentaremos rotarlo después.
        return {ex, ey}; 
    };

    std::vector<Intent> intents;
    intents.reserve(enemies.size());

    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto &e = enemies[i];
        Intent it{e.getX(), e.getY(), e.getX(), e.getY(), false, 1'000'000, i};

        if (inRangePx(e.getX(), e.getY())) {
            auto [nx, ny] = greedyNext(e.getX(), e.getY());
            
            // Si la función greedy dice "quédate aquí" (porque hay pared o jugador),
            // marcamos wants=false, pero luego calcularemos el facing manualmente.
            it.tox = nx; 
            it.toy = ny;
            it.wants = (nx != e.getX() || ny != e.getY());
            it.score = std::abs(px - nx) + std::abs(py - ny);
        }
        else {
            it.score = std::abs(px - e.getX()) + std::abs(py - e.getY()); 
        }
        intents.push_back(it);
    }

    // Resolución de conflictos entre enemigos (Evitar que se fusionen)
    for (size_t i = 0; i < intents.size(); ++i) {
        if (!intents[i].wants) continue;
        for (size_t j = i + 1; j < intents.size(); ++j) {
            if (!intents[j].wants) continue;
            
            // Dos enemigos quieren ir a la misma casilla
            if (intents[i].tox == intents[j].tox && intents[i].toy == intents[j].toy) {
                // Gana el que esté más cerca del jugador (menor score)
                if (intents[j].score < intents[i].score) intents[i].wants = false;
                else intents[j].wants = false;
            }
            // Intercambio directo (Head-on swap) - Evitar atravesarse
            if (intents[i].tox == intents[j].fromx && intents[i].toy == intents[j].fromy &&
                intents[j].tox == intents[i].fromx && intents[j].toy == intents[i].fromy) {
                 intents[i].wants = false;
                 intents[j].wants = false;
            }
        }
    }

    // APLICAR MOVIMIENTO Y ROTACIÓN (Facing)
    for (size_t i = 0; i < enemies.size(); ++i) {
        int ox = intents[i].fromx, oy = intents[i].fromy;
        
        if (intents[i].wants) {
            // MOVERSE
            enemies[i].setPos(intents[i].tox, intents[i].toy);
            
            // Calcular dirección basada en el movimiento
            int dx = intents[i].tox - ox;
            int dy = intents[i].toy - oy;
            if (dx > 0) enemyFacing[i] = EnemyFacing::Right;
            else if (dx < 0) enemyFacing[i] = EnemyFacing::Left;
            else if (dy > 0) enemyFacing[i] = EnemyFacing::Down;
            else if (dy < 0) enemyFacing[i] = EnemyFacing::Up;
        } 
        else {
            // NO SE MUEVE (Bloqueado o llegó al jugador)
            // IMPORTANTE: Si está adyacente al jugador, FORZAR que le mire.
            // Esto arregla el bug de que no ataquen si se pegan a ti.
            if (isAdjacent4(ox, oy, px, py)) {
                int dx = px - ox;
                int dy = py - oy;
                if (std::abs(dx) >= std::abs(dy)) {
                    enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                } else {
                    enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
                }
            }
        }
    }
    
    // Intentar atacar después de moverse/rotar
    enemyTryAttackFacing();
}

void Game::takeDamage(int amount) {
    // Si tiene escudi
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
            if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
            
            // 1. Daño
            enemyHP[i] -= DMG_HANDS;

            Vector2 ePos = { (float)enemies[i].getX() * tileSize + 8, 
                             (float)enemies[i].getY() * tileSize - 10 };
            spawnFloatingText(ePos, DMG_HANDS, RAYWHITE);

            if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;

            std::cout << "[Melee] Puñetazo! -" << DMG_HANDS << "\n";
            
            // 2. PROVOCACIÓN INSTANTÁNEA
            // Reseteamos su cooldown a 0: ¡Está listo para atacar YA!
            if (i < enemyAtkCD.size()) enemyAtkCD[i] = 0.0f;
            
            // Le obligamos a mirarte a la cara
            int dx = px - enemies[i].getX();
            int dy = py - enemies[i].getY();
            if (i < enemyFacing.size()) {
                if (std::abs(dx) >= std::abs(dy)) 
                    enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                else 
                    enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
            }
        }
    }
    
    // Limpieza de muertos
    if (hit) {
         std::vector<size_t> toRemove;
         for(size_t i=0; i<enemyHP.size(); ++i) if(enemyHP[i] <= 0) toRemove.push_back(i);
         if (!toRemove.empty()) {
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (size_t idx : toRemove) {
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
    if (swordTier == 2) dmg = DMG_SWORD_T2;
    if (swordTier == 3) dmg = DMG_SWORD_T3;

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
                if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
                
                // 1. Daño
                enemyHP[i] -= dmg;

                Vector2 ePos = { (float)enemies[i].getX() * tileSize + 8, 
                                 (float)enemies[i].getY() * tileSize - 10 };
                // Si es critico (Tier 3) lo ponemos Amarillo, si no Blanco
                Color c = (swordTier >= 3) ? YELLOW : RAYWHITE;
                spawnFloatingText(ePos, dmg, c);

                if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;

                std::cout << "[Sword] Slash! -" << dmg << "\n";

                // 2. PROVOCACIÓN
                if (i < enemyAtkCD.size()) enemyAtkCD[i] = 0.0f; // Reset CD
                
                // Forzar encare hacia el jugador
                int dx = px - enemies[i].getX();
                int dy = py - enemies[i].getY();
                if (i < enemyFacing.size()) {
                    if (std::abs(dx) >= std::abs(dy)) 
                        enemyFacing[i] = (dx > 0) ? EnemyFacing::Right : EnemyFacing::Left;
                    else 
                        enemyFacing[i] = (dy > 0) ? EnemyFacing::Down : EnemyFacing::Up;
                }
                break;
            }
        }
    }
    
    // Limpieza de muertos
     if (hit) {
         std::vector<size_t> toRemove;
         for(size_t i=0; i<enemyHP.size(); ++i) if(enemyHP[i] <= 0) toRemove.push_back(i);
         if (!toRemove.empty()) {
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (size_t idx : toRemove) {
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
    // 1. Burst
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

        int tx = (int)(p.pos.x / tileSize);
        int ty = (int)(p.pos.y / tileSize);
        if (tx < 0 || ty < 0 || tx >= map.width() || ty >= map.height() || 
            map.at(tx, ty) == WALL) {
            p.active = false; 
            continue;
        }

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
                if (enemyHP.size() != enemies.size()) enemyHP.assign(enemies.size(), 100);
                enemyHP[i] -= p.damage;

                Vector2 ePos = { (float)enemies[i].getX() * tileSize + 8, 
                                 (float)enemies[i].getY() * tileSize - 10 };
                spawnFloatingText(ePos, p.damage, SKYBLUE); // Azul para el plasma

                if (i < enemyFlashTimer.size()) enemyFlashTimer[i] = 0.15f;
    
                std::cout << "[Plasma] Hit! -" << p.damage << "\n";
                break; 
            }
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), 
            [](const Projectile& p){ return !p.active; }),
        projectiles.end()
    );

    std::vector<size_t> toRemove;
    for(size_t i=0; i<enemyHP.size(); ++i) {
        if (enemyHP[i] <= 0) toRemove.push_back(i);
    }
    if (!toRemove.empty()) {
        std::sort(toRemove.rbegin(), toRemove.rend());
        for (size_t idx : toRemove) {
            enemies.erase(enemies.begin() + idx);
            if (idx < enemyFacing.size()) enemyFacing.erase(enemyFacing.begin() + idx);
            if (idx < enemyHP.size()) enemyHP.erase(enemyHP.begin() + idx);
            if (idx < enemyAtkCD.size()) enemyAtkCD.erase(enemyAtkCD.begin() + idx);
            if (idx < enemyFlashTimer.size()) enemyFlashTimer.erase(enemyFlashTimer.begin() + idx);
        }
    }
}

void Game::drawProjectiles() const {
    for (const auto& p : projectiles) {
        DrawCircleV(p.pos, 5.0f, SKYBLUE);
        DrawCircleV(p.pos, 2.0f, WHITE);
    }
}

// --- SISTEMA DE TEXTOS FLOTANTES ---

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