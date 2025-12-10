#include "Game.hpp"
#include "GameUtils.hpp"
#include "AssetPath.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <string>

// Variable global definida en Game.cpp
extern bool gQuitRequested;

// Función auxiliar para leer el texto de ayuda
static bool loadTextAsset(const char *relPath, std::string &outText)
{
    const std::string full = assetPath(relPath);
    char *buf = LoadFileText(full.c_str());
    if (buf)
    {
        outText.assign(buf);
        UnloadFileText(buf);
        return true;
    }
    else
    {
        outText = "No se encontro '" + full + "'.\n";
        std::cerr << "[ASSETS] No se pudo cargar texto desde: " << full << "\n";
        return false;
    }
}

void Game::processInput()
{
    // --------------------------------------------------------
    // 1. DETECCIÓN DE ACTIVACIÓN (F12) - MODO DIOS
    // --------------------------------------------------------
    if (IsKeyPressed(KEY_F12)) {
        showGodModeInput = !showGodModeInput;
        godModeInput.clear(); // Limpiar texto al abrir
        return;
    }

    // 2. SI ESTAMOS ESCRIBIENDO LA CONTRASEÑA...
    if (showGodModeInput) {
        // Capturar caracteres del teclado (Input de texto)
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (godModeInput.length() < 10)) {
                godModeInput.push_back((char)key);
            }
            key = GetCharPressed();
        }

        // Borrar caracteres (Backspace)
        if (IsKeyPressed(KEY_BACKSPACE) && !godModeInput.empty()) {
            godModeInput.pop_back();
        }

        // Confirmar (Enter)
        if (IsKeyPressed(KEY_ENTER)) {
            if (godModeInput == "IDDQD") { // Contraseña
                toggleGodMode(!godMode);   // Invertir estado actual
            }
            showGodModeInput = false;      // Cerrar cuadro
            godModeInput.clear();
        }

        // Cancelar (Escape)
        if (IsKeyPressed(KEY_ESCAPE)) {
            showGodModeInput = false;
        }
        
        return; // IMPORTANTE: Bloquear el resto del input del juego mientras escribes
    }

    // --------------------------------------------------------
    // 3. INPUT GENERAL (MENÚ / SALIR)
    // --------------------------------------------------------
    // Excluimos OptionsMenu porque tiene su propia lógica de 'Volver' en handleOptionsInput
    if (state != GameState::MainMenu && state != GameState::OptionsMenu)
    {
        bool escPressed = IsKeyPressed(KEY_ESCAPE);
        const int gp0 = 0;
        if (IsGamepadAvailable(gp0))
        {
            escPressed = escPressed ||
                         IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || // B / Círculo
                         // ELIMINADO: IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_MIDDLE_RIGHT) || <-- Start ya no saca del juego
                         IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_MIDDLE);             // Guide
        }
        if (escPressed)
        {
            showHelp = false;
            gAttack.swinging = false;
            gAttack.lastTiles.clear();

            // Desactivar Modo Dios al salir al menú para no hacer trampa en la siguiente run
            if (godMode) {
                godMode = false;        // Quitar invencibilidad
                map.setRevealAll(false);// Restaurar niebla
                // No llamamos a toggleGodMode() para evitar el sonido al salir
            }

            // Asegurarnos de cerrar el cuadro de diálogo si estaba abierto
            showGodModeInput = false; 
            godModeInput.clear();
            
            state = GameState::MainMenu;
            mainMenuSelection = 0;
            return;
        }
    }

    auto restartRun = [&]()
    {
        if (fixedSeed == 0)
            runSeed = nextRunSeed();
        newRun();
    };

    if (IsKeyPressed(KEY_R))
    {
        restartRun();
        return;
    }

    const int gpRestart = 0;
    // Evitar reiniciar con botón si estamos en menús
    if (state != GameState::MainMenu && state != GameState::OptionsMenu &&
        IsGamepadAvailable(gpRestart) &&
        IsGamepadButtonPressed(gpRestart, GAMEPAD_BUTTON_RIGHT_FACE_UP))
    {
        restartRun();
        return;
    }

    if (IsKeyPressed(KEY_T))
    {
        moveMode = (moveMode == MovementMode::StepByStep)
                       ? MovementMode::RepeatCooldown
                       : MovementMode::StepByStep;
        moveCooldown = 0.0f;
    }

    const float dt = GetFrameTime();
    if (state == GameState::MainMenu)
    {
        handleMenuInput();
        return;
    }
    else if (state == GameState::OptionsMenu)   
    {
        handleOptionsInput();                   
        return;
    }
    else if (state == GameState::Playing)
    {
        handlePlayingInput(dt);
        return;
    }

    player.update(dt, false);
}

void Game::handleMenuInput()
{
    const int menuGamepad = 0;

    if (showHelp)
    {
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
        for (char c : helpText) if (c == '\n') ++lines;
        int maxScroll = std::max(0, lines * lineH - viewportH);
        if (helpScroll > maxScroll) helpScroll = maxScroll;

        const char *backTxt = "VOLVER";
        int tw = MeasureText(backTxt, backFs);
        int tx = pxl + panelW - tw - 16;
        int ty = pyl + panelH - backFs - 12;
        Rectangle backHit = {(float)(tx - 6), (float)(ty - 4),
                             (float)(tw + 12), (float)(backFs + 8)};

        bool clickBack = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                         CheckCollisionPointRec(GetMousePosition(), backHit);
        bool escBack = IsKeyPressed(KEY_ESCAPE);
        bool gpBack = false;

        if (IsGamepadAvailable(menuGamepad)) {
            gpBack = IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        }

        if (clickBack || escBack || gpBack) {
            showHelp = false;
        }
        return;
    }

    // Definición de botones
    int bw = (int)std::round(screenW * 0.35f);
    bw = std::clamp(bw, 320, 560);
    int bh = (int)std::round(screenH * 0.12f);
    bh = std::clamp(bh, 72, 120);
    int startY = (int)std::round(screenH * 0.52f);
    int gap = (int)std::round(screenH * 0.04f);

    Rectangle playBtn = {(float)((screenW - bw) / 2), (float)startY,
                         (float)bw, (float)bh};
    Rectangle readBtn = {(float)((screenW - bw) / 2),
                         (float)(startY + bh + gap), (float)bw, (float)bh};
    Rectangle quitBtn = {(float)((screenW - bw) / 2),
                         (float)(startY + (bh + gap) * 2), (float)bw, (float)bh};

    // Rectángulo del botón ajustes (Coincide con GameUI)
    int settingsSize = (int)std::round(screenH * 0.08f);
    settingsSize = std::clamp(settingsSize, 48, 120);
    Rectangle settingsRect = {(float)(screenW - settingsSize - 20), (float)(screenH - settingsSize - 20),
                              (float)settingsSize, (float)settingsSize};
    
    // El rectángulo "diffRect" solo es relevante si showSettingsMenu es true, pero aquí estamos en el menú principal
    // para ir a opciones, así que comprobamos el botón "settingsRect".

    // Lógica Ratón
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mp = GetMousePosition();

        // Clic en el botón de engranaje/ajustes
        if (CheckCollisionPointRec(mp, settingsRect))
        {
            state = GameState::OptionsMenu;
            mainMenuSelection = 0; // Reset selección para el siguiente menú
            return;
        }

        if (CheckCollisionPointRec(mp, playBtn))
        {
            newRun();
            return;
        }
        if (CheckCollisionPointRec(mp, readBtn))
        {
            if (helpText.empty())
                loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp = true;
            helpScroll = 0;
            return;
        }
        if (CheckCollisionPointRec(mp, quitBtn))
        {
            gQuitRequested = true;
            return;
        }
    }

    // --- LÓGICA DE ACTIVACIÓN ---
    auto activateSelection = [&]()
    {
        if (mainMenuSelection == 0) {
            newRun();
        }
        else if (mainMenuSelection == 1) {
            if (helpText.empty())
                loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp = true;
            helpScroll = 0;
        }
        else if (mainMenuSelection == 2) {
            gQuitRequested = true;
        }
        else if (mainMenuSelection == 3) { // NUEVO: Opción 3 = Ajustes
            state = GameState::OptionsMenu;
            mainMenuSelection = 0; 
        }
    };

    // --- NAVEGACIÓN TECLADO (Ahora con módulo 4) ---
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        mainMenuSelection = (mainMenuSelection + 4 - 1) % 4; // Cambiado 3 -> 4
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        mainMenuSelection = (mainMenuSelection + 1) % 4;     // Cambiado 3 -> 4
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        activateSelection();
        return;
    }

    // --- NAVEGACIÓN GAMEPAD (Ahora con módulo 4) ---
    if (IsGamepadAvailable(menuGamepad))
    {
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_UP))
        {
            mainMenuSelection = (mainMenuSelection + 4 - 1) % 4; // Cambiado 3 -> 4
        }
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        {
            mainMenuSelection = (mainMenuSelection + 1) % 4;     // Cambiado 3 -> 4
        }

        static bool stickNeutral = true;
        float ay = GetGamepadAxisMovement(menuGamepad, GAMEPAD_AXIS_LEFT_Y);
        const float dead = 0.35f;

        if (std::fabs(ay) < dead)
        {
            stickNeutral = true;
        }
        else if (stickNeutral)
        {
            if (ay < 0.0f)
                mainMenuSelection = (mainMenuSelection + 4 - 1) % 4; // Cambiado 3 -> 4
            else
                mainMenuSelection = (mainMenuSelection + 1) % 4;     // Cambiado 3 -> 4
            stickNeutral = false;
        }

        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        {
            activateSelection();
            return;
        }
        // Botón Y / Triángulo para ayuda rápida
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP))
        {
            if (helpText.empty())
                loadTextAsset("assets/docs/objetos.txt", helpText);
            showHelp = true;
            helpScroll = 0;
            return;
        }
    }
}

void Game::handleOptionsInput()
{
    // Reusamos mainMenuSelection para navegar: 0 = Dificultad, 1 = Volver

    // 1. Salir con ESC o Botón B (Círculo)
    if (IsKeyPressed(KEY_ESCAPE)) {
        state = GameState::MainMenu;
        mainMenuSelection = 0;
        return;
    }

    const int gp0 = 0;
    // Navegación (Arriba/Abajo) y Selección (Enter/A)
    bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool back = false;

    if (IsGamepadAvailable(gp0)) {
        if (IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) back = true; // B / Círculo
        
        if (IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_LEFT_FACE_UP)) up = true;
        if (IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) down = true;
        if (IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) enter = true; // A / Cruz

        // Stick Analógico
        static bool stickNeutral = true;
        float ay = GetGamepadAxisMovement(gp0, GAMEPAD_AXIS_LEFT_Y);
        if (std::fabs(ay) < 0.35f) stickNeutral = true;
        else if (stickNeutral) {
            if (ay < 0.0f) up = true;
            else down = true;
            stickNeutral = false;
        }
    }

    if (back) {
        state = GameState::MainMenu;
        mainMenuSelection = 0;
        return;
    }

    // Lógica de movimiento (solo hay 2 opciones)
    if (up) mainMenuSelection = (mainMenuSelection + 2 - 1) % 2;
    if (down) mainMenuSelection = (mainMenuSelection + 1) % 2;

    // Lógica de acción
    if (enter) {
        if (mainMenuSelection == 0) {
            cycleDifficulty(); // Cambiar dificultad
        } else {
            state = GameState::MainMenu; // Botón Volver
            mainMenuSelection = 0;
        }
    }

    // Clic de ratón
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mp = GetMousePosition();

        int btnW = screenW / 3;
        int btnH = screenH / 12;
        int centerX = screenW / 2;

        // Estos rectángulos deben coincidir con los que usas en renderOptionsMenu()
        Rectangle diffRect = {
            (float)(centerX - btnW / 2),
            (float)(screenH / 3),
            (float)btnW,
            (float)btnH
        };

        Rectangle backRect = {
            (float)(centerX - btnW / 2),
            (float)(screenH - screenH / 4),
            (float)btnW,
            (float)btnH
        };

        // Click en dificultad → rotar dificultad
        if (CheckCollisionPointRec(mp, diffRect))
        {
            cycleDifficulty();
            return;
        }

        // Click en VOLVER → volver al menú principal
        if (CheckCollisionPointRec(mp, backRect))
        {
            state = GameState::MainMenu;
            return;
        }
    }
}

// ----------------------------------------------------------------------------------
// INPUT JUEGO (COMBATE Y MOVIMIENTO)
// ----------------------------------------------------------------------------------
void Game::handlePlayingInput(float dt)
{
    // --------------------------------------------------------
    // 1. DETECCIÓN DE DISPOSITIVO
    // --------------------------------------------------------
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
        IsKeyDown(KEY_E) || IsKeyDown(KEY_I) || IsKeyDown(KEY_O) ||
        IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_ONE) || IsKeyDown(KEY_TWO) ||
        IsKeyDown(KEY_LEFT_SHIFT))
    { // Añadido Shift
        lastInput = InputDevice::Keyboard;
    }

    const int gpId = 0;
    if (IsGamepadAvailable(gpId))
    {
        bool gpActive = false;
        // Check botones
        for (int b = GAMEPAD_BUTTON_UNKNOWN + 1; b <= GAMEPAD_BUTTON_RIGHT_THUMB; b++)
        {
            if (IsGamepadButtonDown(gpId, b))
            {
                gpActive = true;
                break;
            }
        }
        // Check ejes
        if (!gpActive)
        {
            float lx = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X);
            float ly = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
            if (std::abs(lx) > 0.25f || std::abs(ly) > 0.25f)
                gpActive = true;
        }
        // Check gatillos
        if (!gpActive)
        {
            float rt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_TRIGGER);
            float lt = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_TRIGGER);
            if (rt > 0.1f || lt > 0.1f)
                gpActive = true;
        }

        if (gpActive)
            lastInput = InputDevice::Gamepad;
    }

    // --------------------------------------------------------
    // 2. LÓGICA DE JUEGO (Zoom, Interacción)
    // --------------------------------------------------------
    if (IsKeyDown(KEY_I))
        cameraZoom += 1.0f * dt;
    if (IsKeyDown(KEY_O))
        cameraZoom -= 1.0f * dt;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
        cameraZoom = expf(logf(cameraZoom) + wheel * 0.1f);

    if (IsGamepadAvailable(gpId))
    {
        float rightY = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_RIGHT_Y);
        const float zoomThr = 0.2f;
        if (rightY < -zoomThr)
            cameraZoom += (-rightY) * dt;
        else if (rightY > zoomThr)
            cameraZoom -= rightY * dt;

        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_THUMB))
        {
            cameraZoom = 1.0f;
            camera.zoom = cameraZoom;
            camera.rotation = 0.0f;
            clampCameraToMap();
        }
    }

    cameraZoom = std::clamp(cameraZoom, 0.5f, 3.0f);
    camera.zoom = cameraZoom;
    clampCameraToMap();

    if (IsKeyPressed(KEY_C))
    {
        cameraZoom = 1.0f;
        camera.zoom = cameraZoom;
        camera.rotation = 0.0f;
        clampCameraToMap();
    }

    // Interacción (Pickup)
    bool interact = IsKeyPressed(KEY_E);
    if (IsGamepadAvailable(gpId))
    {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
            interact = true;
    }
    if (interact)
        tryManualPickup();

    // --------------------------------------------------------
    // 3. DASH (ESQUIVA)
    // --------------------------------------------------------
    bool dashPressed = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT);
    if (IsGamepadAvailable(gpId))
    {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_TRIGGER_1) ||
            IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_TRIGGER_2))
        {
            dashPressed = true;
        }
    }

    if (dashPressed && !isDashing && dashCooldownTimer <= 0.0f)
    {
        int ddx = 0, ddy = 0;
        // Prioridad: Dirección pulsada ahora
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            ddy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            ddy = 1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            ddx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            ddx = 1;

        // Si no, última dirección
        if (ddx == 0 && ddy == 0)
        {
            ddx = gAttack.lastDir.x;
            ddy = gAttack.lastDir.y;
        }

        if (ddx != 0 || ddy != 0)
        {
            int targetX = px;
            int targetY = py;

            // Calcular destino (hasta DASH_DISTANCE)
            for (int i = 1; i <= DASH_DISTANCE; i++)
            {
                int nextX = px + ddx * i;
                int nextY = py + ddy * i;
                if (!map.isWalkable(nextX, nextY))
                    break; // Pared

                bool enemyBlock = false;
                for (const auto &e : enemies)
                    if (e.getX() == nextX && e.getY() == nextY)
                        enemyBlock = true;
                if (enemyBlock)
                    break; // Enemigo

                targetX = nextX;
                targetY = nextY;
            }

            if (targetX != px || targetY != py)
            {
                // Configurar estado Dash
                isDashing = true;
                dashTimer = DASH_DURATION;
                dashCooldownTimer = DASH_COOLDOWN;

                dashStartPos = {px * (float)tileSize, py * (float)tileSize};
                dashEndPos = {targetX * (float)tileSize, targetY * (float)tileSize};

                px = targetX;
                py = targetY;
                player.setGridPos(px, py);

                PlaySound(sfxDash);

                onSuccessfulStep(0, 0);
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

    if (IsGamepadAvailable(gpId))
    {
        float axisX = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X);
        float axisY = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
        const float thr = 0.5f;

        if (axisX < -thr || axisX > thr || axisY < -thr || axisY > thr)
        {
            gpAnalogActive = true;
            if (axisY < -thr)
                gpDy = -1;
            else if (axisY > thr)
                gpDy = +1;
            if (axisX < -thr)
                gpDx = -1;
            else if (axisX > thr)
                gpDx = +1;
        }

        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_UP))
        {
            gpDx = 0;
            gpDy = -1;
            gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        {
            gpDx = 0;
            gpDy = +1;
            gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
        {
            gpDx = -1;
            gpDy = 0;
            gpDpadPressed = true;
        }
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
        {
            gpDx = +1;
            gpDy = 0;
            gpDpadPressed = true;
        }

        if (gpAnalogActive)
            moveMode = MovementMode::RepeatCooldown;
        else if (gpDpadPressed)
            moveMode = MovementMode::StepByStep;
    }

    if (moveMode == MovementMode::StepByStep)
    {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
            dy = -1;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
            dy = +1;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
            dx = -1;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
            dx = +1;

        if (dx == 0 && dy == 0 && (gpDx != 0 || gpDy != 0))
        {
            dx = gpDx;
            dy = gpDy;
        }

        if (dx != 0 || dy != 0)
        {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy);
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved)
                onSuccessfulStep(dx, dy);
        }
        player.update(dt, moved);
    }
    else
    {
        moveCooldown -= dt;
        bool pressedNow = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
                          IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
                          IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                          IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                          gpDpadPressed;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
            dy = -1;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
            dy = +1;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            dx = -1;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            dx = +1;

        if (dx == 0 && gpDx != 0)
            dx = gpDx;
        if (dy == 0 && gpDy != 0)
            dy = gpDy;

        if (dx != 0 || dy != 0)
        {
            gAttack.lastDir = {dx, dy};
            player.setDirectionFromDelta(dx, dy);
        }

        if (pressedNow)
        {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved)
                onSuccessfulStep(dx, dy);
            moveCooldown = MOVE_INTERVAL;
        }
        else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f)
        {
            int oldx = px, oldy = py;
            tryMove(dx, dy);
            moved = (px != oldx || py != oldy);
            if (moved)
                onSuccessfulStep(dx, dy);
            moveCooldown = MOVE_INTERVAL;
        }
        player.update(dt, moved);
    }

    // --------------------------------------------------------
    // 5. SISTEMA DE COMBATE (INPUT)
    // --------------------------------------------------------
    gAttack.cdTimer = std::max(0.f, gAttack.cdTimer - dt);
    plasmaCooldown = std::max(0.f, plasmaCooldown - dt);

    if (gAttack.swinging)
    {
        gAttack.swingTimer = std::max(0.f, gAttack.swingTimer - dt);
        if (gAttack.swingTimer <= 0.f)
        {
            gAttack.swinging = false;
            gAttack.lastTiles.clear();
        }
    }

    // A) MANOS
    bool attackHands = IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))
        attackHands = true;

    if (attackHands && gAttack.cdTimer <= 0.f)
        performMeleeAttack();

    // B) ESPADA
    bool attackSword = IsKeyPressed(KEY_ONE);
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_1))
        attackSword = true;

    if (attackSword)
    {
        if (swordTier > 0)
        {
            if (gAttack.cdTimer <= 0.f)
                performSwordAttack();
        }
        else
        {
            // std::cout << "No tienes espada\n";
        }
    }

    // C) PLASMA
    bool attackPlasma = IsKeyPressed(KEY_TWO);
    if (IsGamepadAvailable(gpId))
    {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_TRIGGER_2))
            attackPlasma = true;
    }
    if (attackPlasma)
    {
        if (plasmaTier > 0)
        {
            if (plasmaCooldown <= 0.f)
                performPlasmaAttack();
        }
        else
        {
            // std::cout << "No tienes plasma\n";
        }
    }
}
