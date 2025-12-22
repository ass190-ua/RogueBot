#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "I18n.hpp"


Rectangle Game::uiCenterRect(float w, float h) const {
    return {(screenW - w) * 0.5f, (screenH - h) * 0.5f, w, h};
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

void Game::render() {
    // --------------------------------------------------------
    // 1. Menús de pantalla (Salida anticipada)
    // --------------------------------------------------------
    if (state == GameState::MainMenu) {
        renderMainMenu();
        return;
    }

    if (state == GameState::OptionsMenu) {
        renderOptionsMenu();
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    // ----------------------------------------------------------
    // 2. Mundo de juego (Capa 2D)
    // Se dibuja siempre (Jugando, Tutorial, Pausa, GameOver...)
    // ----------------------------------------------------------
    BeginMode2D(camera);

        // 2.1 Mapa (Suelo y Paredes con iluminación)
        map.draw(tileSize, px, py, getFovRadius(), itemSprites.wall, itemSprites.floor);
        
        // 2.2 Entidades
        drawItems();
        drawEnemies(); // Dibuja enemigos normales
        drawBoss();    // Dibuja al Boss (si está activo)
        
        // 2.3 Jugador
        player.draw(tileSize, px, py);
        
        // 2.4 Efectos Visuales y Partículas
        if (slashActive) drawSlash(); // Estela del ataque melee
        drawProjectiles();            // Balas de plasma
        drawParticles();              // Explosiones y sangre
        drawFloatingTexts();          // Números de daño flotantes

    EndMode2D();

    // --------------------------------------------------------
    // 3. Interfaz de usuario (HUD y Menús superpuestos)
    // --------------------------------------------------------

    // Caso A: Tutorial
    if (state == GameState::Tutorial) {
        hud.drawPlaying(*this); // Mostramos vida para ver efecto de pociones
        renderTutorialUI();     // Instrucciones amarillas arriba
    }
    // Caso B: Jugando normal
    else if (state == GameState::Playing) {
        hud.drawPlaying(*this);
    }
    // Caso C: Pausa (Juego congelado de fondo + Menú)
    else if (state == GameState::Paused) {
        hud.drawPlaying(*this); // Mantenemos el HUD visible
        renderPauseMenu();
    }
    // Caso D: Pantallas finales
    else if (state == GameState::Victory) {
        hud.drawVictory(*this);
    }
    else if (state == GameState::GameOver) {
        hud.drawGameOver(*this);
    }

    // --------------------------------------------------------
    // 4. Overlays globales (Siempre encima de todo)
    // --------------------------------------------------------
    
    // Consola modo Dios (Terminal Hacker)
    if (showGodModeInput) {
        // Fondo Dimmer
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f)); 

        int panelW = 500;
        int panelH = 200;
        int cx = (screenW - panelW) / 2;
        int cy = (screenH - panelH) / 2;

        float time = (float)GetTime();
        float pulse = (sinf(time * 4.0f) + 1.0f) * 0.5f;
        Color borderColor = ColorAlpha(LIME, 0.6f + (0.4f * pulse)); 
        Color glowColor   = ColorAlpha(LIME, 0.2f * pulse);

        // Panel estilo retro
        DrawRectangle(cx + 10, cy + 10, panelW, panelH, Color{0, 0, 0, 200}); // Sombra
        DrawRectangle(cx, cy, panelW, panelH, Color{10, 30, 10, 255});        // Fondo
        DrawRectangleLinesEx({(float)cx, (float)cy, (float)panelW, (float)panelH}, 4, borderColor);
        DrawRectangle(cx, cy, panelW, panelH, glowColor);

        // Scanlines
        for (int i = 0; i < panelH; i += 4) {
            DrawRectangle(cx, cy + i, panelW, 1, Fade(LIME, 0.1f));
        }

        // Título
        const char* title = _("/// SYSTEM OVERRIDE ///");
        int titleW = MeasureText(title, 20);
        DrawRectangle(cx, cy, panelW, 30, borderColor);
        DrawText(title, cx + (panelW - titleW) / 2, cy + 5, 20, BLACK);

        // Input Box
        DrawText(_("ENTER ACCESS CODE:"), cx + 30, cy + 50, 20, LIME);
        DrawRectangle(cx + 30, cy + 80, panelW - 60, 40, BLACK);
        DrawRectangleLines(cx + 30, cy + 80, panelW - 60, 40, borderColor);

        // Texto con cursor
        std::string displayTxt = godModeInput;
        if ((int)(time * 2.0f) % 2 == 0) displayTxt += "_"; 
        DrawText(displayTxt.c_str(), cx + 45, cy + 90, 20, LIME);

        // Footer
        DrawText(_("STATUS: WAITING FOR INPUT..."), cx + 30, cy + 140, 10, Fade(LIME, 0.7f));
        DrawText(_("[ENTER] EXECUTE   [ESC] ABORT"), cx + panelW - 180, cy + 170, 10, Fade(LIME, 0.5f));
    }

    EndDrawing();
}
