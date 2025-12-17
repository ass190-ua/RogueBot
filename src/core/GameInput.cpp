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
    // 0. Detección globar de dispositivo (Input Device)
    // --------------------------------------------------------
    // Esto asegura que la UI cambie los iconos (Teclado vs Mando) en cualquier menú.

    // A) Detectar Teclado / Ratón
    bool mouseMoved = (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f);
    bool mouseClicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);

    // Usamos GetKeyPressed() para ver si hay alguna tecla en la cola,
    // pero ojo, esto la consume, así que solo lo usamos de 'flag' genérico
    // si no usamos input de texto complejo en el frame actual (salvo godmode).
    // Una alternativa segura es chequear las teclas de navegación comunes:
    bool keysPressed = IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
                       IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
                       IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_ESCAPE);

    if (mouseMoved || mouseClicked || keysPressed)
    {
        lastInput = InputDevice::Keyboard;
    }

    // B) Detectar Gamepad
    const int gpId = 0;
    if (IsGamepadAvailable(gpId))
    {
        bool gpActive = false;

        // Chequear botones principales
        if (IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
            IsGamepadButtonDown(gpId, GAMEPAD_BUTTON_MIDDLE_RIGHT))
        {
            gpActive = true;
        }

        // Chequear Sticks (con zona muerta para evitar drift)
        if (!gpActive)
        {
            if (std::abs(GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_X)) > 0.25f ||
                std::abs(GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y)) > 0.25f)
            {
                gpActive = true;
            }
        }

        if (gpActive)
            lastInput = InputDevice::Gamepad;
    }

    // --------------------------------------------------------
    // Detección de Cheat Code (Konami Code)
    // --------------------------------------------------------

    // Temporizador estático para evitar dobles pulsaciones (Debounce)
    static float cheatTimer = 0.0f;
    if (cheatTimer > 0.0f)
        cheatTimer -= GetFrameTime();

    const int gpCheat = 0;

    // Solo leemos el mando si el temporizador ha llegado a cero
    if (IsGamepadAvailable(gpCheat) && cheatTimer <= 0.0f)
    {

        int pressedInput = GetGamepadButtonPressed();
        // Filtrar ruido (ID 0 es desconocido)
        if (pressedInput == 0)
            pressedInput = -1;

        // 1. Fallback cruceta (D-PAD)
        if (pressedInput == -1)
        {
            if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_LEFT_FACE_UP))
                pressedInput = GAMEPAD_BUTTON_LEFT_FACE_UP;
            else if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
                pressedInput = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
            else if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
                pressedInput = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
            else if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
                pressedInput = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
            // Botones de acción
            else if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                pressedInput = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
            else if (IsGamepadButtonPressed(gpCheat, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                pressedInput = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
        }

        // 2. Joystick analógico (Umbral más alto 0.7 para evitar falsos positivos)
        static bool stickUp = false;
        static bool stickDown = false;
        static bool stickLeft = false;
        static bool stickRight = false;

        float axisY = GetGamepadAxisMovement(gpCheat, GAMEPAD_AXIS_LEFT_Y);
        float axisX = GetGamepadAxisMovement(gpCheat, GAMEPAD_AXIS_LEFT_X);
        float threshold = 0.7f;

        if (pressedInput == -1)
        {
            if (axisY < -threshold)
            {
                if (!stickUp)
                {
                    pressedInput = GAMEPAD_BUTTON_LEFT_FACE_UP;
                    stickUp = true;
                }
            }
            else
                stickUp = false;

            if (axisY > threshold)
            {
                if (!stickDown)
                {
                    pressedInput = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
                    stickDown = true;
                }
            }
            else
                stickDown = false;

            if (axisX < -threshold)
            {
                if (!stickLeft)
                {
                    pressedInput = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
                    stickLeft = true;
                }
            }
            else
                stickLeft = false;

            if (axisX > threshold)
            {
                if (!stickRight)
                {
                    pressedInput = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
                    stickRight = true;
                }
            }
            else
                stickRight = false;
        }

        // 3. Pocesar secuencia
        if (pressedInput != -1)
        {
            cheatTimer = 0.25f;

            // Log limpio para que veas el progreso
            std::cout << "[CHEAT] Detectado: " << pressedInput
                      << " (Paso " << cheatCodeIndex + 1 << "/10)\n";

            if (pressedInput == konamiCode[cheatCodeIndex])
            {
                cheatCodeIndex++;

                if (cheatCodeIndex >= konamiCode.size())
                {
                    std::cout << "[CHEAT] GOD MODE ACTIVADO!\n";
                    toggleGodMode(!godMode);
                    cheatCodeIndex = 0;
                    PlaySound(sfxPowerUp);
                }
            }
            else
            {
                cheatCodeIndex = 0;
                // Si el error fue pulsar 'Arriba', cuenta como el inicio de un nuevo intento
                if (pressedInput == konamiCode[0])
                    cheatCodeIndex = 1;
                std::cout << "[CHEAT] Secuencia reiniciada.\n";
            }
        }
    }

    // --------------------------------------------------------
    // 1. Detección de activación (F12) - Modo Dios
    // --------------------------------------------------------
    if (IsKeyPressed(KEY_F12))
    {
        showGodModeInput = !showGodModeInput;
        godModeInput.clear();
        return;
    }

    // 2. Si estamos escribiendo la contraseña...
    if (showGodModeInput)
    {
        int key = GetCharPressed();
        while (key > 0)
        {
            if ((key >= 32) && (key <= 125) && (godModeInput.length() < 10))
            {
                godModeInput.push_back((char)key);
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !godModeInput.empty())
            godModeInput.pop_back();

        if (IsKeyPressed(KEY_ENTER))
        {
            if (godModeInput == "IDDQD")
                toggleGodMode(!godMode);
            showGodModeInput = false;
            godModeInput.clear();
        }
        if (IsKeyPressed(KEY_ESCAPE))
            showGodModeInput = false;

        return;
    }

    // --------------------------------------------------------
    // Sistema de pausa (Toggle)
    // --------------------------------------------------------
    bool togglePause = IsKeyPressed(KEY_ESCAPE);
    if (IsGamepadAvailable(gpId))
    {
        if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_MIDDLE_RIGHT))
            togglePause = true;
    }

    if (togglePause)
    {
        // Si estamos Jugando O en Tutorial -> Pausar
        if (state == GameState::Playing || state == GameState::Tutorial)
        {
            pauseOrigin = state; // Guardamos el origen (Playing o Tutorial)
            state = GameState::Paused;
            pauseSelection = 0;
            PauseSound(sfxAmbient);
            return;
        }
        // Si estamos Pausados -> Reanudar
        else if (state == GameState::Paused)
        {
            state = pauseOrigin;

            // Solo reactivamos música si volvemos al juego normal (Tutorial es silencio/sfx)
            if (state == GameState::Playing)
                ResumeSound(sfxAmbient);
            return;
        }
        else if (state == GameState::OptionsMenu)
        {
            state = previousState;
            if (state == GameState::MainMenu)
                StopSound(sfxAmbient);
            return;
        }
    }

    // --------------------------------------------------------
    // 3. Input general (Salida directa en Game Over / Victory)
    // --------------------------------------------------------
    if (state == GameState::Victory || state == GameState::GameOver)
    {
        bool escPressed = IsKeyPressed(KEY_ESCAPE);
        if (IsGamepadAvailable(gpId))
        {
            escPressed = escPressed || IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                         IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_MIDDLE);
        }

        if (escPressed)
        {
            showHelp = false;
            gAttack.swinging = false;
            gAttack.lastTiles.clear();

            if (godMode)
            {
                godMode = false;
                map.setRevealAll(false);
            }
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

    // 1. Reinicio Teclado (Bloqueado en Tutorial)
    if (IsKeyPressed(KEY_R) && state != GameState::Tutorial)
    {
        restartRun();
        return;
    }

    // 2. Reinicio mando (Bloqueado en menús y Tutorial)
    if (state != GameState::MainMenu &&
        state != GameState::OptionsMenu &&
        state != GameState::Paused &&
        state != GameState::Tutorial &&
        IsGamepadAvailable(gpId) &&
        IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_UP))
    {
        restartRun();
        return;
    }

    if (IsKeyPressed(KEY_T))
    {
        moveMode = (moveMode == MovementMode::StepByStep) ? MovementMode::RepeatCooldown : MovementMode::StepByStep;
        moveCooldown = 0.0f;
    }

    const float dt = GetFrameTime();

    // --- RUTEO DE ESTADOS ---
    if (state == GameState::MainMenu)
    {
        handleMenuInput();
        return;
    }
    else if (state == GameState::Paused)
    {
        handlePauseInput();
        return;
    }
    else if (state == GameState::OptionsMenu)
    {
        handleOptionsInput();
        return;
    }
    else if (state == GameState::Tutorial)
    {
        // Si estamos en el Menú Final, usamos input de Menú
        if (tutorialStep == TutorialStep::FinishedMenu)
        {

            // 1. Navegación (Arriba/Abajo)
            bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
            bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
            bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

            if (IsGamepadAvailable(gpId))
            {
                if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_UP))
                    up = true;
                if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
                    down = true;
                if (IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                    enter = true; // A / Cruz

                // Stick analógico con "debounce" simple
                static bool stickNeutral = true;
                float ay = GetGamepadAxisMovement(gpId, GAMEPAD_AXIS_LEFT_Y);
                if (std::fabs(ay) < 0.35f)
                    stickNeutral = true;
                else if (stickNeutral)
                {
                    if (ay < 0.0f)
                        up = true;
                    else
                        down = true;
                    stickNeutral = false;
                }
            }

            if (up)
            {
                tutorialMenuSelection--;
                if (tutorialMenuSelection < 0)
                    tutorialMenuSelection = 2; // Ciclo
                PlaySound(sfxPickup);          // Feedback sonoro opcional
            }
            if (down)
            {
                tutorialMenuSelection++;
                if (tutorialMenuSelection > 2)
                    tutorialMenuSelection = 0; // Ciclo
                PlaySound(sfxPickup);
            }

            // 2. Acción
            if (enter)
            {
                if (tutorialMenuSelection == 0)
                { // Repetir
                    startTutorial();
                }
                else if (tutorialMenuSelection == 1)
                { // Jugar
                    newRun();
                }
                else if (tutorialMenuSelection == 2)
                { // Menú
                    state = GameState::MainMenu;
                    StopSound(sfxAmbient);
                }
                PlaySound(sfxPowerUp); // Sonido de confirmación
            }

            // Ratón (Opcional, para consistencia)
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (tutorialMenuSelection == 0)
                    startTutorial();
                else if (tutorialMenuSelection == 1)
                    newRun();
                else if (tutorialMenuSelection == 2)
                {
                    state = GameState::MainMenu;
                    StopSound(sfxAmbient);
                }
            }
        }
        else
        {
            // Juego normal durante el tutorial
            handlePlayingInput(dt);
            updateTutorial(dt);
        }
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
        if (panelW > 1500)
            panelW = 1500;
        if (panelH > 900)
            panelH = 900;
        int pxl = (screenW - panelW) / 2;
        int pyl = (screenH - panelH) / 2;

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
        {
            helpScroll -= (int)(wheel * 40);
            if (helpScroll < 0)
                helpScroll = 0;
        }

        if (IsGamepadAvailable(menuGamepad))
        {
            float ay = GetGamepadAxisMovement(menuGamepad, GAMEPAD_AXIS_LEFT_Y);
            const float dead = 0.25f;
            if (std::fabs(ay) > dead)
            {
                const float speed = 400.0f;
                helpScroll += (int)(ay * speed * GetFrameTime());
                if (helpScroll < 0)
                    helpScroll = 0;
            }
        }

        int margin = 24;
        int titleSize = (int)std::round(panelH * 0.06f);
        int top = pyl + margin + titleSize + 16;
        int backFs = std::max(16, (int)std::round(panelH * 0.048f));
        int footerH = backFs + 16;
        int viewportH = panelH - (top - pyl) - margin - footerH;
        int fontSize = (int)std::round(screenH * 0.022f);
        if (fontSize < 16)
            fontSize = 16;
        int lineH = (int)std::round(fontSize * 1.25f);
        int lines = 1;
        for (char c : helpText)
            if (c == '\n')
                ++lines;
        int maxScroll = std::max(0, lines * lineH - viewportH);
        if (helpScroll > maxScroll)
            helpScroll = maxScroll;

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

        if (IsGamepadAvailable(menuGamepad))
        {
            gpBack = IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
                     IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        }

        if (clickBack || escBack || gpBack)
        {
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
            previousState = GameState::MainMenu;
            state = GameState::OptionsMenu;
            mainMenuSelection = 0; // Reset selección para el siguiente menú
            pendingDifficulty = difficulty;
            return;
        }

        if (CheckCollisionPointRec(mp, playBtn))
        {
            newRun();
            return;
        }
        if (CheckCollisionPointRec(mp, readBtn))
        {
            startTutorial();
            return;
        }
        if (CheckCollisionPointRec(mp, quitBtn))
        {
            gQuitRequested = true;
            return;
        }
    }

    // Lógica de activación
    auto activateSelection = [&]()
    {
        if (mainMenuSelection == 0)
        {
            newRun();
        }
        else if (mainMenuSelection == 1)
        {
            startTutorial();
        }
        else if (mainMenuSelection == 2)
        {
            gQuitRequested = true;
        }
        else if (mainMenuSelection == 3)
        {
            previousState = GameState::MainMenu;
            state = GameState::OptionsMenu;
            mainMenuSelection = 0;
        }
    };

    // Navegación Teclado
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        mainMenuSelection = (mainMenuSelection + 4 - 1) % 4;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        mainMenuSelection = (mainMenuSelection + 1) % 4;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        activateSelection();
        return;
    }

    // Navegación Gamepad
    if (IsGamepadAvailable(menuGamepad))
    {
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_UP))
        {
            mainMenuSelection = (mainMenuSelection + 4 - 1) % 4;
        }
        if (IsGamepadButtonPressed(menuGamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        {
            mainMenuSelection = (mainMenuSelection + 1) % 4;
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
                mainMenuSelection = (mainMenuSelection + 4 - 1) % 4;
            else
                mainMenuSelection = (mainMenuSelection + 1) % 4;
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

void Game::handlePauseInput()
{
    // --------------------------------------------------------
    // 1. Navegación teclado / Gamepad
    // --------------------------------------------------------
    bool up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool back = false;

    const int gp = 0;
    if (IsGamepadAvailable(gp))
    {
        // Cruceta
        if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_UP))
            up = true;
        if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN))
            down = true;

        // Aceptación (A / Cruz)
        if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
            enter = true;

        // Botón Atrás (B / Círculo)
        if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
            back = true;

        // Joystick Izquierdo (con zona muerta)
        static bool stickNeutral = true;
        float ay = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_Y);
        const float dead = 0.35f;

        if (std::fabs(ay) < dead)
        {
            stickNeutral = true;
        }
        else if (stickNeutral)
        {
            if (ay < 0.0f)
                up = true;
            else
                down = true;
            stickNeutral = false;
        }
    }

    // Acción rápida: Volver (Reanudar)
    if (back)
    {
        state = pauseOrigin;
        if (state == GameState::Playing)
            ResumeSound(sfxAmbient);
        return;
    }

    // Actualizar selección cíclica
    if (up)
        pauseSelection = (pauseSelection + 3 - 1) % 3;
    if (down)
        pauseSelection = (pauseSelection + 1) % 3;

    // --------------------------------------------------------
    // 2. Lógica de ratón
    // --------------------------------------------------------
    int centerX = screenW / 2;
    int centerY = screenH / 2;
    // Ajuste visual para coincidir con el renderizado (startY = centerY - 50)
    int btnW = 300, btnH = 60, gap = 20, startY = centerY - 50;

    Rectangle btnResume = {(float)centerX - btnW / 2, (float)startY, (float)btnW, (float)btnH};
    Rectangle btnSettings = {(float)centerX - btnW / 2, (float)startY + btnH + gap, (float)btnW, (float)btnH};
    Rectangle btnExit = {(float)centerX - btnW / 2, (float)startY + (btnH + gap) * 2, (float)btnW, (float)btnH};

    Vector2 mp = GetMousePosition();

    if (CheckCollisionPointRec(mp, btnResume))
    {
        pauseSelection = 0;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            enter = true;
    }
    else if (CheckCollisionPointRec(mp, btnSettings))
    {
        pauseSelection = 1;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            enter = true;
    }
    else if (CheckCollisionPointRec(mp, btnExit))
    {
        pauseSelection = 2;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            enter = true;
    }

    // --------------------------------------------------------
    // 3. Ejecución de acción
    // --------------------------------------------------------
    if (enter)
    {
        switch (pauseSelection)
        {
        case 0: // Reanudar
            state = pauseOrigin;
            if (state == GameState::Playing)
                ResumeSound(sfxAmbient);
            break;

        case 1:                                // Ajustes
            previousState = GameState::Paused; // Recordar volver aquí
            state = GameState::OptionsMenu;
            mainMenuSelection = 0;
            pendingDifficulty = difficulty;
            break;

        case 2: // Salir al menú
            state = GameState::MainMenu;
            StopSound(sfxAmbient);
            mainMenuSelection = 0;

            // Limpieza de la partida
            enemies.clear();
            items.clear();
            projectiles.clear();
            particles.clear();
            floatingTexts.clear();
            isDashing = false;
            gAttack.swinging = false;
            showGodModeInput = false;

            std::cout << "[GAME] Partida terminada. Volviendo al Menu.\n";
            break;
        }
    }
}

void Game::handleOptionsInput()
{
    // --------------------------------------------------------
    // 1. Manejo de alerta de reinicio
    // --------------------------------------------------------
    if (showDifficultyWarning)
    {
        bool confirm = false;
        bool cancel = false;

        // Detección híbrida (Teclado / Mando)
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            confirm = true;
        if (IsKeyPressed(KEY_ESCAPE))
            cancel = true;

        const int gp = 0;
        if (IsGamepadAvailable(gp))
        {
            if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                confirm = true; // A (Aceptar)
            if (IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
                cancel = true; // B (Rechazar)
        }

        if (confirm)
        {
            // Aplicar cambios y reiniciar
            difficulty = pendingDifficulty; // Confirmamos el cambio
            newRun();
            state = GameState::Playing;
            ResumeSound(sfxAmbient);
            showDifficultyWarning = false;
            std::cout << "[GAME] Dificultad actualizada a " << (int)difficulty << ". Run reiniciada.\n";
        }
        else if (cancel)
        {
            showDifficultyWarning = false;
        }
        return;
    }

    // --------------------------------------------------------
    // 2. Navegación
    // --------------------------------------------------------
    bool back = false;
    if (IsKeyPressed(KEY_ESCAPE))
        back = true;

    const int gp0 = 0;
    if (IsGamepadAvailable(gp0) && IsGamepadButtonPressed(gp0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        back = true;

    // Lógica "Volver"
    if (back)
    {
        // ¿Ha cambiado la dificultad?
        if (pendingDifficulty != difficulty)
        {
            if (previousState == GameState::Paused)
            {
                // Si estamos en pausa y cambió -> Alerta
                showDifficultyWarning = true;
                return;
            }
            else
            {
                // Si estamos en menú principal -> Aplicar directamente
                difficulty = pendingDifficulty;
            }
        }

        // Salir normalmente
        state = previousState;
        if (state == GameState::MainMenu)
            mainMenuSelection = 0;
        return;
    }

    // --------------------------------------------------------
    // 3. Lógica de RATÓN
    // --------------------------------------------------------
    Vector2 mp = GetMousePosition();
    int btnW = screenW / 3;
    int btnH = screenH / 12;
    int centerX = screenW / 2;

    // Rectángulos de botones
    Rectangle diffRect = { (float)(centerX - btnW / 2), (float)(screenH / 3), (float)btnW, (float)btnH };
    Rectangle backRect = { (float)(centerX - btnW / 2), (float)(screenH - screenH / 4), (float)btnW, (float)btnH };

    // --- CÁLCULO EXACTO DEL SLIDER (Copiado de tu GameUI.cpp) ---
    float sliderMarginY = screenH * 0.09f; // <--- AQUÍ ESTABA EL ERROR (usabas 0.05f o 0.09f aleatoriamente)
    float sliderX = (float)(centerX - btnW / 2);
    float sliderY = diffRect.y + diffRect.height + sliderMarginY;
    
    // Definimos un área de colisión más alta (40px) para que sea fácil "agarrar" la barra
    Rectangle sliderRect = { sliderX, sliderY - 15, (float)btnW, 40.0f };

    // A) CLICK EN BOTONES
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mp, diffRect)) {
            pendingDifficulty = (Difficulty)(((int)pendingDifficulty + 1) % 3);
        }
        else if (CheckCollisionPointRec(mp, backRect)) {
            if (pendingDifficulty != difficulty && previousState == GameState::Paused) showDifficultyWarning = true;
            else { difficulty = pendingDifficulty; state = previousState; }
        }
    }

    // B) ARRASTRAR VOLUMEN (Down para fluidez)
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mp, sliderRect)) {
            // Calculamos cuánto se ha movido el ratón dentro de la barra
            float rel = (mp.x - sliderRect.x) / sliderRect.width;
            audioVolume = std::clamp(rel, 0.0f, 1.0f);
            SetMasterVolume(audioVolume); // Aplicamos el cambio al sistema de sonido
        }
    }
}

// ----------------------------------------------------------------------------------
// Input Juego (Combate y Movimiento)
// ----------------------------------------------------------------------------------
void Game::handlePlayingInput(float dt)
{
    // --------------------------------------------------------
    // 1. Detección de Dispositivo
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
    // 2. Lógica de juego (Zoom, Interacción)
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
    // 3. Dash (Esquiva)
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
    // 4. Movimiento normal
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
    // 5. Sistema de combate (Input)
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

    // A) Puños
    bool attackHands = IsKeyPressed(KEY_SPACE);
    if (IsGamepadAvailable(gpId) && IsGamepadButtonPressed(gpId, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))
        attackHands = true;

    if (attackHands && gAttack.cdTimer <= 0.f)
        performMeleeAttack();

    // B) Espada
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
            std::cout << "No tienes espada\n";
        }
    }

    // C) Pistola Plasma
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
            std::cout << "No tienes plasma\n";
        }
    }
}
