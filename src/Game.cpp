#include "Game.hpp"
#include "raylib.h"
#include <ctime>
#include <iostream>

static inline unsigned now_seed()
{
    return static_cast<unsigned>(time(nullptr));
}

Game::Game(unsigned seed) : fixedSeed(seed)
{
    // Configurar ventana en fullscreen
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "RogueBot Alpha"); // tamaño se ajusta por el flag

    player.load("assets/sprites/player");

    screenW = GetScreenWidth();
    screenH = GetScreenHeight();

    SetTargetFPS(60);

    newRun(); // primera partida (empieza en Level 1)
}

// Devuelve la seed del RUN (si hay fija, esa; si no, aleatoria en cada R)
unsigned Game::nextRunSeed() const
{
    return fixedSeed > 0 ? fixedSeed : now_seed();
}

// Mezcla para derivar una seed distinta por nivel del mismo run
unsigned Game::seedForLevel(unsigned base, int level) const
{
    // Mezcla simple tipo "golden ratio" para variar por nivel
    const unsigned MIX = 0x9E3779B9u; // 2654435769
    return base ^ (MIX * static_cast<unsigned>(level));
}

void Game::newRun()
{
    // runSeed: fijo si hay CLI, aleatorio si no
    runSeed = nextRunSeed();
    std::cout << "[Run] Seed base del run: " << runSeed << "\n";

    currentLevel = 1;
    state = GameState::Playing;
    moveCooldown = 0.0f;
    hp = 3;

    newLevel(currentLevel);
}

void Game::newLevel(int level)
{
    int tilesX = screenW / tileSize;
    int tilesY = screenH / tileSize;

    // Deriva una seed distinta por nivel, determinista dentro del mismo run
    levelSeed = seedForLevel(runSeed, level);
    std::cout << "[Level] " << level << "/" << maxLevels
              << "  (seed nivel: " << levelSeed << ")\n";

    map.generate(tilesX, tilesY, levelSeed);

    auto r = map.firstRoom();
    if (r.w > 0 && r.h > 0)
    {
        px = r.x + r.w / 2;
        py = r.y + r.h / 2;
        map.computeVisibility(px, py, getFovRadius());
    }
    else
    {
        px = tilesX / 2;
        py = tilesY / 2;
    }

    player.setGridPos(px, py);
}

void Game::tryMove(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return;
    int nx = px + dx, ny = py + dy;

    if (nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() && map.at(nx, ny) != WALL)
    {
        px = nx;
        py = ny;
    }
}

const char *Game::movementModeText() const
{
    return (moveMode == MovementMode::StepByStep) ? "Step-by-step" : "Repeat (cooldown)";
}

void Game::run()
{
    while (!WindowShouldClose())
    {
        processInput();
        update();
        render();
    }
    player.unload();

    CloseWindow();
}

// ======================
// Sub-issue #4 – Movimiento del jugador
// ======================
//
// Funcionalidad: El jugador se mueve por el grid usando WASD o flechas.
//
// - Modo StepByStep: mueve un tile por pulsación (IsKeyPressed).
// - Modo RepeatCooldown: se mueve continuamente mientras mantienes la tecla.
// - tryMove() evita salir del mapa o atravesar paredes (WALL).
// - setDirectionFromDelta(dx, dy) cambia la dirección del sprite.
// - player.update(dt, moved) actualiza animación (walk / idle).

void Game::processInput()
{
    // Reiniciar run completo
    if (IsKeyPressed(KEY_R))
    {
        if (fixedSeed == 0)
            runSeed = nextRunSeed();
        newRun();
        return;
    }

    // Toggle modo de movimiento (step-by-step <-> cooldown)
    if (IsKeyPressed(KEY_T))
    {
        moveMode = (moveMode == MovementMode::StepByStep)
                       ? MovementMode::RepeatCooldown
                       : MovementMode::StepByStep;
        moveCooldown = 0.0f;
    }

    // --- Toggle SECRETO de NIEBLA (no aparece en HUD) ---
    if (IsKeyPressed(KEY_F2))
    {
        fogEnabled = !fogEnabled;
        map.setFogEnabled(fogEnabled);
        if (fogEnabled)
        {
            map.computeVisibility(px, py, getFovRadius());
        }
    }

    // Si no estamos jugando (p.ej., victoria), no mover ni animar
    if (state != GameState::Playing)
    {
        player.update(GetFrameTime(), /*isMoving=*/false);
        return;
    }

    // === Movimiento del jugador + animación de sprites ===
    int dx = 0, dy = 0;
    bool moved = false;
    const float dt = GetFrameTime();

    if (moveMode == MovementMode::StepByStep)
    {
        // Un paso por pulsación
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
            dy = -1;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
            dy = +1;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
            dx = -1;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
            dx = +1;

        int oldx = px, oldy = py;
        tryMove(dx, dy);
        moved = (px != oldx || py != oldy);

        if (moved)
        {
            player.setGridPos(px, py);
            player.setDirectionFromDelta(dx, dy);
            if (map.fogEnabled())
                map.computeVisibility(px, py, getFovRadius());
        }

        // Actualiza animación (idle si no te moviste)
        player.update(dt, moved);
    }
    else
    {
        // Repetición con cooldown mientras mantienes tecla
        moveCooldown -= dt;

        const bool pressedNow =
            IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
            IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
            IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
            IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT);

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            dy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            dy = +1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            dx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            dx = +1;

        if (pressedNow)
        {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);

            if (moved)
            {
                player.setGridPos(px, py);
                player.setDirectionFromDelta(dx, dy);
                if (map.fogEnabled())
                    map.computeVisibility(px, py, getFovRadius());
            }

            moveCooldown = MOVE_INTERVAL;
        }
        else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f)
        {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);

            if (moved)
            {
                player.setGridPos(px, py);
                player.setDirectionFromDelta(dx, dy);
                if (map.fogEnabled())
                    map.computeVisibility(px, py, getFovRadius());
            }

            moveCooldown = MOVE_INTERVAL;
        }

        // Actualiza animación (idle si no hubo movimiento este frame)
        player.update(dt, moved);
    }
}

void Game::update()
{
    if (state != GameState::Playing)
        return;

    // ¿Has llegado a la salida?
    if (map.at(px, py) == EXIT)
    {
        onExitReached();
    }

    // Aquí podrías añadir daño / enemigos y pasar a Game Over si hp<=0
}

void Game::onExitReached()
{
    if (currentLevel < maxLevels)
    {
        currentLevel++;
        newLevel(currentLevel); // ← cada nivel usa levelSeed diferente
    }
    else
    {
        state = GameState::Victory;
        std::cout << "[Victory] ¡Has completado los 3 niveles!\n";
    }
}

void Game::render()
{
    BeginDrawing();
    ClearBackground(BLACK);

    map.draw(tileSize);

    // Jugador (sprite normal)
    player.draw(tileSize, px, py);

    // --- Punto indicador del jugador ---
    // Siempre visible, útil para orientarse con niebla
    Vector2 playerCenter = {
        px * (float)tileSize + tileSize / 2.0f,
        py * (float)tileSize + tileSize / 2.0f};
    DrawCircleV(playerCenter, 2, WHITE);

    // HUD según estado
    if (state == GameState::Playing)
    {
        hud.drawPlaying(*this);
    }
    else if (state == GameState::Victory)
    {
        hud.drawVictory(*this);
    }

    EndDrawing();
}
