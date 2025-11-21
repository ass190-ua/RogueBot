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
    if (loaded)
        return;
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
    if (!loaded)
        return;
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
    // Configurar ventana en fullscreen
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "RogueBot Alpha"); // tamaño se ajusta por el flag
    SetExitKey(KEY_NULL); // Desactiva el cierre por ESC; lo gestionamos nosotros

    player.load("assets/sprites/player");
    itemSprites.load();

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();

    camera.target = {0.0f, 0.0f}; // lo actualizamos tras posicionar al jugador
    camera.offset = {(float)screenW / 2, (float)screenH / 2};
    camera.rotation = 0.0f;
    camera.zoom = cameraZoom; // 1.0f por defecto

    SetTargetFPS(60);

    // newRun(); // primera partida (empieza en Level 1)
    state = GameState::MainMenu;
}

// Devuelve la seed del RUN (si hay fija, esa; si no, aleatoria en cada R)
unsigned Game::nextRunSeed() const {
    return fixedSeed > 0 ? fixedSeed : now_seed();
}

// Mezcla para derivar una seed distinta por nivel del mismo run
unsigned Game::seedForLevel(unsigned base, int level) const {
    // Mezcla simple tipo "golden ratio" para variar por nivel
    const unsigned MIX = 0x9E3779B9u; // 2654435769
    return base ^ (MIX * static_cast<unsigned>(level));
}

void Game::newRun() {
    // runSeed: fijo si hay CLI, aleatorio si no
    runSeed = nextRunSeed();
    std::cout << "[Run] Seed base del run: " << runSeed << "\n";

    currentLevel = 1;
    state = GameState::Playing;
    moveCooldown = 0.0f;
    hp = 5;

    hasKey = false;
    hasShield = false;
    hasBattery = false;
    swordTier = 0;
    plasmaTier = 0;

    // reinicia progreso persistente del run
    runCtx.espadaMejorasObtenidas = 0;
    runCtx.plasmaMejorasObtenidas = 0;
    // reset ataque
    gAttack = AttackRuntime{};
    gAttack.frontOnly = true;

    newLevel(currentLevel);
}

void Game::newLevel(int level) {
    const float WORLD_SCALE = 1.2f;
    int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
    int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

    // Deriva una seed distinta por nivel, determinista dentro del mismo run
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

    // Recalcular FOV según viewport/tileSize actual
    fovTiles = defaultFovFromViewport();
    map.computeVisibility(px, py, getFovRadius());

    // centrar cámara en jugador (mundo -> píxeles)
    Vector2 playerCenterPx = {px * (float)tileSize + tileSize / 2.0f,
                              py * (float)tileSize + tileSize / 2.0f};
    camera.target = playerCenterPx;
    clampCameraToMap();

    hasKey = false; // hay una llave por nivel

    // ==== Spawner de ítems ====
    // RNG determinista por nivel
    rng = std::mt19937(levelSeed);

    // --- Enemigos del nivel ---
    spawnEnemiesForLevel();

    // (de momento no tienes enemigos: lista vacía.
    std::vector<IVec2> enemyTiles;
    enemyTiles.reserve(enemies.size());
    for (const auto &e : enemies) {
        enemyTiles.push_back({e.getX(), e.getY()});
    }

    // Walkable: todo lo que no sea WALL, con bounds check
    auto isWalkable = [&](int x, int y) { 
        return map.isWalkable(x, y); 
    };

    // Spawn = posición actual del jugador
    IVec2 spawnTile{px, py};

    // Exit = pedimos al mapa su posición
    auto [exitX, exitY] = map.findExitTile();
    IVec2 exitTile{exitX, exitY};

    // Generar ítems de este nivel (solo colocación; sin lógica aún)
    items = ItemSpawner::generate(map.width(), map.height(), isWalkable,
                                  spawnTile, exitTile, enemyTiles,
                                  level, // 1..3
                                  rng, runCtx);
}

void Game::tryMove(int dx, int dy) {
    if (dx == 0 && dy == 0)
        return;

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
        // (si está ocupada, simplemente no te mueves)
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
    // ESC desde cualquier sitio distinto al menú -> volver al menú
    // También permitimos usar un botón del mando (por ejemplo, B o Start)
    if (state != GameState::MainMenu) {
        bool escPressed = IsKeyPressed(KEY_ESCAPE);
        // Comprobar botón de mando que actúe como ESC
        const int gp0 = 0;
        if (IsGamepadAvailable(gp0)) {
            // Usamos el botón derecho de la cara (B) o el botón central
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
            return;
        }
    }

    // Reiniciar run completo (igual que antes)
    if (IsKeyPressed(KEY_R)) {
        if (fixedSeed == 0) runSeed = nextRunSeed();
        newRun();
        return;
    }

    // Toggle modo de movimiento
    if (IsKeyPressed(KEY_T)) {
        moveMode = (moveMode == MovementMode::StepByStep)
                   ? MovementMode::RepeatCooldown
                   : MovementMode::StepByStep;
        moveCooldown = 0.0f;
    }

    // Toggle niebla
    if (IsKeyPressed(KEY_F2)) {
        fogEnabled = !fogEnabled;
        map.setFogEnabled(fogEnabled);
        if (fogEnabled) map.computeVisibility(px, py, getFovRadius());
    }

    // Ajuste manual del FOV
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        fovTiles = std::max(2, fovTiles - 1);
        if (map.fogEnabled()) map.computeVisibility(px, py, getFovRadius());
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        fovTiles = std::min(30, fovTiles + 1);
        if (map.fogEnabled()) map.computeVisibility(px, py, getFovRadius());
    }

    // Derivar por estado
    const float dt = GetFrameTime();
    if (state == GameState::MainMenu) {
        handleMenuInput();
        return;
    }
    if (state == GameState::Playing) {
        handlePlayingInput(dt);
        return;
    }

    // Estados no jugables (victoria/game over): solo actualizar animación idle
    player.update(dt, /*isMoving=*/false);
}

void Game::handleMenuInput() {
    // --- Soporte básico de mando en el menú principal ---
    // Permite iniciar la partida, abrir el visor de ayuda o salir usando
    // los botones del mando. Usamos el mando 0 por defecto.  Si el visor
    // de ayuda está abierto, sólo permitimos cerrarlo.
    const int menuGamepad = 0;
    if (IsGamepadAvailable(menuGamepad)) {
        // Cerrar visor de ayuda con botón B (derecha) o volver atrás
        if (showHelp) {
            if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
                showHelp = false;
            }
        }
        else {
            // Botón A (face down) = jugar
            if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                newRun();
                return;
            }
            // Botón X/Y (face up) = leer/ayuda
            if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                if (helpText.empty()) {
                    loadTextAsset("assets/docs/objetos.txt", helpText);
                }
                showHelp = true;
                helpScroll = 0;
                return;
            }
            // Botón B (face right) = salir del juego
            if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                gQuitRequested = true;
                return;
            }
        }
    }

    // --- Si el visor de ayuda está abierto ---
    if (showHelp) {
        int panelW = (int)std::round(screenW * 0.86f);
        int panelH = (int)std::round(screenH * 0.76f);
        if (panelW > 1500) panelW = 1500;
        if (panelH > 900) panelH = 900;
        int pxl = (screenW - panelW) / 2;
        int pyl = (screenH - panelH) / 2;

        // Scroll con la rueda del ratón
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            helpScroll -= (int)(wheel * 40);
            if (helpScroll < 0) helpScroll = 0;
        }

        // Calcular límites del scroll
        int margin = 24;
        int titleSize = (int)std::round(panelH * 0.06f);
        int top = pyl + margin + titleSize + 16;
        int backFs = std::max(16, (int)std::round(panelH * 0.048f));
        int footerH = backFs + 16;
        int viewportH = panelH - (top - pyl) - margin - footerH;
        int fontSize = (int)std::round(screenH * 0.022f);
        if (fontSize < 16) fontSize = 16;
        int lineH = (int)std::round(fontSize * 1.25f);
        int lines = 1;
        for (char c : helpText)
            if (c == '\n') ++lines;
        int maxScroll = std::max(0, lines * lineH - viewportH);
        if (helpScroll > maxScroll) helpScroll = maxScroll;

        // --- Enlace “VOLVER” ---
        const char *backTxt = "VOLVER";
        int tw = MeasureText(backTxt, backFs);
        int tx = pxl + panelW - tw - 16;
        int ty = pyl + panelH - backFs - 12;
        Rectangle backHit = { (float)(tx - 6), (float)(ty - 4),
                              (float)(tw + 12), (float)(backFs + 8) };

        // Clic o ESC → cerrar visor
        if ((IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
             CheckCollisionPointRec(GetMousePosition(), backHit)) ||
            IsKeyPressed(KEY_ESCAPE)) {
            showHelp = false;
        }
        return; // No procesar nada más mientras se muestra ayuda
    }

    // --- Geometría de botones principales ---
    int bw = (int)std::round(screenW * 0.35f);
    bw = std::clamp(bw, 320, 560);
    int bh = (int)std::round(screenH * 0.12f);
    bh = std::clamp(bh, 72, 120);
    int startY = (int)std::round(screenH * 0.52f);
    int gap = (int)std::round(screenH * 0.04f);

    Rectangle playBtn = { (float)((screenW - bw) / 2), (float)startY,
                          (float)bw, (float)bh };
    Rectangle readBtn = { (float)((screenW - bw) / 2),
                          (float)(startY + bh + gap), (float)bw, (float)bh };
    Rectangle quitBtn = { (float)((screenW - bw) / 2),
                          (float)(startY + (bh + gap) * 2), (float)bw, (float)bh };

    // --- Clic izquierdo: JUGAR / LEER / SALIR ---
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();

        // Botón JUGAR
        if (CheckCollisionPointRec(mp, playBtn)) {
            newRun();
            return;
        }

        // Botón LEER ANTES DE JUGAR
        if (CheckCollisionPointRec(mp, readBtn)) {
            if (helpText.empty()) {
                loadTextAsset("assets/docs/objetos.txt", helpText);
            }
            showHelp = true;
            helpScroll = 0;
            return;
        }

        // Botón SALIR
        if (CheckCollisionPointRec(mp, quitBtn)) {
            gQuitRequested = true;
            return;
        }
    }
}

void Game::handlePlayingInput(float dt) {
    // Zoom / cámara
    if (IsKeyDown(KEY_Q)) cameraZoom += 1.0f * dt;   // acercar
    if (IsKeyDown(KEY_E)) cameraZoom -= 1.0f * dt;   // alejar

    float wheel = GetMouseWheelMove();               // rueda (escala log)
    if (wheel != 0.0f) {
        cameraZoom = expf(logf(cameraZoom) + wheel * 0.1f);
    }

    // Zoom con el stick derecho del mando
    const int zoomPad = 0;
    if (IsGamepadAvailable(zoomPad)) {
        float rightY = GetGamepadAxisMovement(zoomPad, GAMEPAD_AXIS_RIGHT_Y);
        const float zoomThr = 0.2f;
        if (rightY < -zoomThr) {
            // Arriba (negativo) → acercar cámara
            cameraZoom += (-rightY) * dt;
        }
        else if (rightY > zoomThr) {
            // Abajo (positivo) → alejar cámara
            cameraZoom -= rightY * dt;
        }
        // Reset de cámara con el botón del stick derecho
        if (IsGamepadButtonPressed(zoomPad, GAMEPAD_BUTTON_RIGHT_THUMB)) {
            cameraZoom = 1.0f;
            camera.zoom = cameraZoom;
            camera.rotation = 0.0f;
            clampCameraToMap();
        }
    }

    cameraZoom = std::clamp(cameraZoom, 0.5f, 3.0f);
    camera.zoom = cameraZoom;
    clampCameraToMap();

    // Reset SOLO de cámara
    if (IsKeyPressed(KEY_C)) {
        cameraZoom = 1.0f;
        camera.zoom = cameraZoom;
        camera.rotation = 0.0f;
        clampCameraToMap();
    }

    // Movimiento del jugador + animación de sprites
    int  dx = 0, dy = 0;
    bool moved = false;

    // ============================================
    // Soporte de mando (gamepad) para movimiento y ataque
    // Para permitir el uso de un mando junto al teclado, recogemos
    // el estado del primer mando disponible. Se usan los botones
    // del "D‑pad" (izquierdo) y el stick izquierdo para mover, y
    // algunos botones frontales para atacar.  Si no hay mando
    // conectado, estas variables se mantienen en cero y no
    // interferirán con el control mediante teclado y ratón.
    int  gpDx = 0, gpDy = 0;
    bool gpAttackPressed = false;
    bool gpDpadPressed = false;
    bool gpAnalogActive = false;
    const int gamepadId = 0;
    if (IsGamepadAvailable(gamepadId)) {
        // Detectar botón de ataque (cara derecha inferior o gatillos delanteros)
        if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
            IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) ||
            IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
            gpAttackPressed = true;
        }

        // Leer el stick analógico izquierdo (movimiento continuo)
        float axisX = GetGamepadAxisMovement(gamepadId, GAMEPAD_AXIS_LEFT_X);
        float axisY = GetGamepadAxisMovement(gamepadId, GAMEPAD_AXIS_LEFT_Y);
        const float thr = 0.5f;
        // Consideramos activo el modo analógico si alguna componente supera el umbral
        if (axisX < -thr || axisX > thr || axisY < -thr || axisY > thr) {
            gpAnalogActive = true;
            if (axisY < -thr) gpDy = -1;
            else if (axisY > thr) gpDy = +1;
            if (axisX < -thr) gpDx = -1;
            else if (axisX > thr) gpDx = +1;
        }

        // Leer la cruceta (D‑pad) como movimiento discreto (solo pulsaciones)
        if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
            gpDx = 0; gpDy = -1; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
            gpDx = 0; gpDy = +1; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
            gpDx = -1; gpDy = 0; gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gamepadId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
            gpDx = +1; gpDy = 0; gpDpadPressed = true;
        }

        // Cambiar el modo de movimiento automáticamente según el input del mando
        if (gpAnalogActive) {
            moveMode = MovementMode::RepeatCooldown;
        } else if (gpDpadPressed) {
            moveMode = MovementMode::StepByStep;
        }
    }

    if (moveMode == MovementMode::StepByStep) {
        // Un paso por pulsación
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    dy = -1;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  dy = +1;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  dx = -1;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) dx = +1;

        // Complementar con eventos de mando (D‑pad) en modo paso a paso
        // Si no hay input de teclado, asigna el desplazamiento del mando.
        if (dx == 0 && dy == 0 && (gpDx != 0 || gpDy != 0)) {
            dx = gpDx;
            dy = gpDy;
        }

        if (dx != 0 || dy != 0) {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy); // girar sin moverse

            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);

            if (moved) {
                onSuccessfulStep(dx, dy);
            }
        }

        // Animación (idle si no te moviste)
        player.update(dt, moved);
    }
    else {
        // Repetición con cooldown mientras mantienes tecla
        moveCooldown -= dt;

        // Detectar si hay una pulsación inicial de cualquier dirección
        bool pressedNow = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
                          IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
                          IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                          IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT);
        // Añadir las pulsaciones del mando: cruceta y stick analógico
        // Si gpDpadPressed o gpAnalogActive está activo, iniciamos un paso inmediato
        pressedNow = pressedNow || gpDpadPressed || gpAnalogActive;

        // Lectura continua de teclas
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy = +1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx = +1;

        // Complementar con valores del mando
        if (dx == 0 && gpDx != 0) dx = gpDx;
        if (dy == 0 && gpDy != 0) dy = gpDy;

        // permite girarte sin moverte
        if (dx != 0 || dy != 0) {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy);
        }

        if (pressedNow) {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);

            if (moved) {
                onSuccessfulStep(dx, dy);
            }

            moveCooldown = MOVE_INTERVAL;
        }
        else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f) {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);

            if (moved) {
                onSuccessfulStep(dx, dy);
            }

            moveCooldown = MOVE_INTERVAL;
        }

        // Animación (idle si no hubo movimiento este frame)
        player.update(dt, moved);
    }

    // Ataque Melee
    gAttack.cdTimer    = std::max(0.f, gAttack.cdTimer - dt);
    if (gAttack.swinging) {
        gAttack.swingTimer = std::max(0.f, gAttack.swingTimer - dt);
        if (gAttack.swingTimer <= 0.f) {
            gAttack.swinging = false;
            gAttack.lastTiles.clear();
        }
    }

    // Rango base del puño
    gAttack.rangeTiles = RANGE_MELEE_HAND;

    // Input de ataque: SPACE, click izquierdo o botones del gamepad
    const bool attackPressed = IsKeyPressed(KEY_SPACE) ||
                               IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                               gpAttackPressed;

    if (attackPressed && gAttack.cdTimer <= 0.f) {
        // Calcula tiles válidos con oclusión ANTES de iniciar el gesto
        IVec2 center{px, py};
        auto tiles = computeMeleeTilesOccluded(
            center, gAttack.lastDir, gAttack.rangeTiles, gAttack.frontOnly, map);

        if (tiles.empty()) {
            // Delante hay pared/obstáculo: NO swing, NO cooldown, NO flash
            return;
        }

        // Inicia gesto/flash/cooldown
        gAttack.swinging   = true;
        gAttack.swingTimer = gAttack.swingTime;
        gAttack.cdTimer    = gAttack.cooldown;
        gAttack.lastTiles  = std::move(tiles);

        std::vector<size_t> toRemove;
        toRemove.reserve(enemies.size());

        bool hitSomeone = false;

        for (size_t i = 0; i < enemies.size(); ++i) {
            const int ex = enemies[i].getX();
            const int ey = enemies[i].getY();

            bool impacted = false;
            for (const auto &t : gAttack.lastTiles) {
                if (t.x == ex && t.y == ey) { impacted = true; break; }
            }
            if (!impacted) continue;

            // Asegura tamaño de arrays paralelos
            if (enemyHP.size() != enemies.size()) {
                enemyHP.assign(enemies.size(), ENEMY_BASE_HP);
                enemyAtkCD.assign(enemies.size(), 0.0f);
            }

            hitSomeone = true;

            int before  = enemyHP[i];
            enemyHP[i]  = std::max(0, enemyHP[i] - PLAYER_MELEE_DMG);

            std::cout << "[Melee] Player -> Enemy (" << ex << "," << ey
                      << ") " << before << "% -> " << enemyHP[i] << "%\n";

            if (enemyHP[i] <= 0) toRemove.push_back(i);
        }

        if (!toRemove.empty()) {
            std::sort(toRemove.rbegin(), toRemove.rend());
            for (size_t idx : toRemove) {
                enemies.erase(enemies.begin() + (long)idx);
                if (idx < enemyFacing.size())
                    enemyFacing.erase(enemyFacing.begin() + (long)idx);
                if (idx < enemyHP.size())
                    enemyHP.erase(enemyHP.begin() + (long)idx);
                if (idx < enemyAtkCD.size())
                    enemyAtkCD.erase(enemyAtkCD.begin() + (long)idx);
            }
        }

        if (!hitSomeone)
            std::cout << "[Melee] Swing (no target)\n";

        // Posible contraataque del enemigo si está enfrente y mirando
        enemyTryAttackFacing();
    }

    // Debug/controles de vida
    if (IsKeyPressed(KEY_H)) { hp = std::max(0, hp - 1); }      // perder vida
    if (IsKeyPressed(KEY_J)) { hp = std::min(hpMax, hp + 1); }  // ganar vida
}

void Game::clampCameraToMap() {
    const float worldW = map.width() * (float)tileSize;
    const float worldH = map.height() * (float)tileSize;

    // tamaño del viewport en coordenadas de mundo (depende del zoom)
    const float viewW = screenW / camera.zoom;
    const float viewH = screenH / camera.zoom;
    const float halfW = viewW * 0.5f;
    const float halfH = viewH * 0.5f;

    // Si el mundo es más pequeño que el viewport, centramos;
    // si no, acotamos el target al rango [half, world-half]
    if (worldW <= viewW)
        camera.target.x = worldW * 0.5f;
    else
        camera.target.x = std::clamp(camera.target.x, halfW, worldW - halfW);

    if (worldH <= viewH)
        camera.target.y = worldH * 0.5f;
    else
        camera.target.y = std::clamp(camera.target.y, halfH, worldH - halfH);
}

void Game::centerCameraOnPlayer() {
    camera.target = {
        px * (float)tileSize + tileSize / 2.0f,
        py * (float)tileSize + tileSize / 2.0f
    };
    clampCameraToMap();
}

void Game::recomputeFovIfNeeded() {
    if (map.fogEnabled()) {
        map.computeVisibility(px, py, getFovRadius());
    }
}

void Game::onSuccessfulStep(int dx, int dy) {
    if (dx != 0 || dy != 0) {
        gAttack.lastDir = {dx, dy};           // mantiene facing del melee
        player.setDirectionFromDelta(dx, dy); // gira el sprite
    }

    player.setGridPos(px, py);                // estado del jugador
    recomputeFovIfNeeded();                   // FOV si la niebla está activa
    centerCameraOnPlayer();                   // cámara centrada en tile del player
    updateEnemiesAfterPlayerMove(true);       // IA de enemigos tras moverte
}

int Game::defaultFovFromViewport() const {
    // nº de tiles visibles en cada eje
    int tilesX = screenW / tileSize;
    int tilesY = screenH / tileSize;
    int r = static_cast<int>(std::floor(std::min(tilesX, tilesY) * 0.15f));
    return std::clamp(r, 3, 20);
}

void Game::update() {
    if (state != GameState::Playing)
        return;

    // recoger si estás encima de algo
    tryPickupHere();

    // ¿Has llegado a la salida?
    if (map.at(px, py) == EXIT) {
        if (hasKey)
            onExitReached();
    }

    // helper local
    auto Lerp = [](float a, float b, float t) { 
        return a + (b - a) * t; 
    };

    // cada frame, en vez de asignar directo:
    Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                       py * (float)tileSize + tileSize / 2.0f};
    float smooth = 10.0f * GetFrameTime(); // 0.0–1.0 (ajusta a gusto)
    camera.target.x = Lerp(camera.target.x, desired.x, smooth);
    camera.target.y = Lerp(camera.target.y, desired.y, smooth);
    clampCameraToMap();

    // Transición a Game Over si la vida llega a 0
    if (hp <= 0) {
        state = GameState::GameOver;
        gAttack.swinging = false; // por si había flash de melee activo
        gAttack.lastTiles.clear();
        return;
    }

    damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
    for (auto &cd : enemyAtkCD) cd = std::max(0.0f, cd - GetFrameTime());
}

void Game::onExitReached() {
    if (currentLevel < maxLevels) {
        currentLevel++;
        newLevel(currentLevel); // ← cada nivel usa levelSeed diferente
    }
    else {
        state = GameState::Victory;
        std::cout << "[Victory] ¡Has completado los 3 niveles!\n";
    }
}

void Game::render() {
    if (state == GameState::MainMenu) {
        renderMainMenu();
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    // --- Cámara: el mapa y el jugador se dibujan dentro del mundo ---
    BeginMode2D(camera);

    // Mapa (mundo)
    map.draw(tileSize);

    // Enemigos
    drawEnemies();

    // Jugador (en coordenadas del mundo)
    player.draw(tileSize, px, py);

    // Ítems del nivel (placeholder en colores)
    drawItems();
    // Requiere que exista gAttack (AttackRuntime) y que lo actualices en processInput()
    if (gAttack.swinging && !gAttack.lastTiles.empty()) {
        Color fill = {255, 230, 50, 120}; // amarillo translúcido
        for (const auto &t : gAttack.lastTiles) {
            int xpx = t.x * tileSize;
            int ypx = t.y * tileSize;
            DrawRectangle(xpx, ypx, tileSize, tileSize, fill);
            DrawRectangleLines(xpx, ypx, tileSize, tileSize, YELLOW);
        }
    }

    EndMode2D();
    // --- Fin de cámara ---

    // --- HUD --- (no afectado por cámara ni zoom)
    if (state == GameState::Playing) {
        hud.drawPlaying(*this);
    }
    else if (state == GameState::Victory) {
        hud.drawVictory(*this);
    }
    else if (state == GameState::GameOver) {
        hud.drawGameOver(*this);
    }

    EndDrawing();
}

void Game::updateEnemiesAfterPlayerMove(bool moved) {
    if (!moved)
        return;

    struct Intent {
        int fromx, fromy; // origen
        int tox, toy;     // destino propuesto
        bool wants;       // quiere moverse
        int score;        // menor = mejor (distancia al jugador tras moverse)
        size_t idx;       // índice del enemigo (para desempates estables)
    };

    auto inRangePx = [&](int ex, int ey) -> bool {
        int dx = px - ex, dy = py - ey;
        float distPx = std::sqrt(float(dx * dx + dy * dy)) * float(tileSize);
        return distPx <= float(ENEMY_DETECT_RADIUS_PX);
    };

    auto can = [&](int nx, int ny) -> bool {
        return nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
               map.isWalkable(nx, ny);
    };

    auto sgn = [](int v) { 
        return (v > 0) - (v < 0); 
    };

    auto greedyNext = [&](int ex, int ey) -> std::pair<int, int> {
        int dx = px - ex, dy = py - ey;
        if (dx == 0 && dy == 0)
            return {ex, ey};
        if (std::abs(dx) >= std::abs(dy)) {
            int nx = ex + sgn(dx), ny = ey;
            if (can(nx, ny))
                return {nx, ny};
            nx = ex;
            ny = ey + sgn(dy);
            if (can(nx, ny))
                return {nx, ny};
        }
        else {
            int nx = ex, ny = ey + sgn(dy);
            if (can(nx, ny))
                return {nx, ny};
            nx = ex + sgn(dx);
            ny = ey;
            if (can(nx, ny))
                return {nx, ny};
        }
        return {ex, ey}; // bloqueado
    };

    // Construir intenciones
    std::vector<Intent> intents;
    intents.reserve(enemies.size());
    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto &e = enemies[i];
        Intent it{e.getX(), e.getY(), e.getX(), e.getY(), false, 1'000'000, i};

        if (inRangePx(e.getX(), e.getY())) {
            auto [nx, ny] = greedyNext(e.getX(), e.getY());
            it.tox = nx;
            it.toy = ny;
            it.wants = (nx != e.getX() || ny != e.getY());
            it.score = std::abs(px - nx) + std::abs(py - ny);
        }
        else {
            it.score = std::abs(px - e.getX()) + std::abs(py - e.getY()); // fuera de rango: no se mueve
        }
        intents.push_back(it);
    }

    // Resolver conflictos de MISMO destino: gana menor score; si empatan, menor idx
    for (size_t i = 0; i < intents.size(); ++i) {
        if (!intents[i].wants)
            continue;
        for (size_t j = i + 1; j < intents.size(); ++j) {
            if (!intents[j].wants)
                continue;
            if (intents[i].tox == intents[j].tox &&
                intents[i].toy == intents[j].toy) {
                bool jWins = (intents[j].score < intents[i].score) ||
                             (intents[j].score == intents[i].score &&
                              intents[j].idx < intents[i].idx);
                if (jWins) {
                    intents[i].tox = intents[i].fromx;
                    intents[i].toy = intents[i].fromy;
                    intents[i].wants = false;
                }
                else {
                    intents[j].tox = intents[j].fromx;
                    intents[j].toy = intents[j].fromy;
                    intents[j].wants = false;
                }
            }
        }
    }

    // No invadir la casilla de un enemigo que se queda quieto
    for (size_t i = 0; i < intents.size(); ++i) {
        if (!intents[i].wants)
            continue;
        for (size_t j = 0; j < intents.size(); ++j) {
            if (i == j)
                continue;
            bool otherStays =
                !intents[j].wants || (intents[j].tox == intents[j].fromx &&
                                      intents[j].toy == intents[j].fromy);
            if (otherStays && intents[i].tox == intents[j].fromx &&
                intents[i].toy == intents[j].fromy) {
                intents[i].tox = intents[i].fromx;
                intents[i].toy = intents[i].fromy;
                intents[i].wants = false;
                break;
            }
        }
    }

    // Evitar "swap" cabeza-con-cabeza (A<->B). Gana mejor score; si empatan, menor idx
    for (size_t i = 0; i < intents.size(); ++i) {
        if (!intents[i].wants)
            continue;
        for (size_t j = i + 1; j < intents.size(); ++j) {
            if (!intents[j].wants)
                continue;
            bool headOnSwap = intents[i].tox == intents[j].fromx &&
                              intents[i].toy == intents[j].fromy &&
                              intents[j].tox == intents[i].fromx &&
                              intents[j].toy == intents[i].fromy;
            if (headOnSwap) {
                bool jWins = (intents[j].score < intents[i].score) ||
                             (intents[j].score == intents[i].score &&
                              intents[j].idx < intents[i].idx);
                if (jWins) {
                    intents[i].tox = intents[i].fromx;
                    intents[i].toy = intents[i].fromy;
                    intents[i].wants = false;
                }
                else {
                    intents[j].tox = intents[j].fromx;
                    intents[j].toy = intents[j].fromy;
                    intents[j].wants = false;
                }
            }
        }
    }

    // Calcular facing por delta y aplicar movimientos
    for (size_t i = 0; i < enemies.size(); ++i) {
        int ox = intents[i].fromx, oy = intents[i].fromy;
        int tx = intents[i].tox, ty = intents[i].toy;

        // Facing por DELTA INTENTADO (aunque luego no se mueva)
        int dxTry = tx - ox, dyTry = ty - oy;
        if (dxTry != 0 || dyTry != 0) {
            if (std::abs(dxTry) >= std::abs(dyTry))
                enemyFacing[i] = (dxTry > 0) ? EnemyFacing::Right : EnemyFacing::Left;
            else
                enemyFacing[i] = (dyTry > 0) ? EnemyFacing::Down : EnemyFacing::Up;
        }

        // No permitir que un enemigo se meta en la casilla del jugador
        int nx = tx, ny = ty;
        if (nx == px && ny == py) {
            nx = ox;
            ny = oy;
        }

        enemies[i].setPos(nx, ny);
    }

    enemyTryAttackFacing();
}

void Game::takeDamage(int amount) {
    int old = hp;
    hp = std::max(0, hp - amount);
    int oldPct = (int)std::lround(100.0 * old / std::max(1, hpMax));
    int newPct = (int)std::lround(100.0 * hp / std::max(1, hpMax));
    std::cout << "[HP] -" << amount << " (" << old << "→" << hp << ") " << oldPct
              << "%→" << newPct << "%\n";

    if (hp == 0 && state == GameState::Playing) {
        state = GameState::GameOver;
        gAttack.swinging = false;
        gAttack.lastTiles.clear();
        std::cout << "[GameOver] Has sido destruido.\n";
    }
}

Rectangle Game::uiCenterRect(float w, float h) const {
    return {(screenW - w) * 0.5f, (screenH - h) * 0.5f, w, h};
}
