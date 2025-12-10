#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>


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
    if (state == GameState::MainMenu) {
        renderMainMenu();
        return;
    }

    // 2) Menú de opciones (nuevo)
    if (state == GameState::OptionsMenu) {
        renderOptionsMenu();   // <--- AQUÍ USAMOS LA FUNCIÓN NUEVA
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

    // 7. GOD MODE UI (Dibujado al final para que quede encima de todo)
    if (showGodModeInput) {
        // --- 1. Fondo de Pantalla Oscurecido (Dimmer) ---
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f)); 

        // --- 2. Configuración del Panel ---
        int panelW = 500;
        int panelH = 200;
        int cx = (screenW - panelW) / 2;
        int cy = (screenH - panelH) / 2;

        // --- 3. Animación de "Respiración" para el borde ---
        float time = (float)GetTime();
        float pulse = (sinf(time * 4.0f) + 1.0f) * 0.5f; // Va de 0.0 a 1.0
        // Color oscila entre Verde Oscuro y Verde Neón
        Color borderColor = ColorAlpha(LIME, 0.6f + (0.4f * pulse)); 
        Color glowColor   = ColorAlpha(LIME, 0.2f * pulse);

        // --- 4. Dibujado del Panel Principal ---
        // Sombra sólida estilo retro (offset)
        DrawRectangle(cx + 10, cy + 10, panelW, panelH, Color{0, 0, 0, 200});
        
        // Fondo del terminal (Verde muy oscuro)
        DrawRectangle(cx, cy, panelW, panelH, Color{10, 30, 10, 255});
        
        // Resplandor interior (Glow)
        DrawRectangleLinesEx({(float)cx, (float)cy, (float)panelW, (float)panelH}, 4, borderColor);
        DrawRectangle(cx, cy, panelW, panelH, glowColor);

        // --- 5. Decoración "Scanlines" (Líneas de monitor viejo) ---
        for (int i = 0; i < panelH; i += 4) {
            DrawRectangle(cx, cy + i, panelW, 1, Fade(LIME, 0.1f));
        }

        // --- 6. Cabecera del Sistema ---
        const char* title = "/// SYSTEM OVERRIDE ///";
        int titleW = MeasureText(title, 20);
        DrawRectangle(cx, cy, panelW, 30, borderColor); // Barra superior
        DrawText(title, cx + (panelW - titleW) / 2, cy + 5, 20, BLACK); // Texto en negro sobre verde

        // --- 7. Campo de Contraseña ---
        DrawText("ENTER ACCESS CODE:", cx + 30, cy + 50, 20, LIME);
        
        // Caja de input
        DrawRectangle(cx + 30, cy + 80, panelW - 60, 40, BLACK);
        DrawRectangleLines(cx + 30, cy + 80, panelW - 60, 40, borderColor);

        // --- 8. Texto con Cursor Parpadeante ---
        std::string displayTxt = godModeInput;
        // El cursor aparece cada medio segundo
        if ((int)(time * 2.0f) % 2 == 0) {
            displayTxt += "_"; 
        }
        
        DrawText(displayTxt.c_str(), cx + 45, cy + 90, 20, LIME);

        // --- 9. Pie de página ---
        DrawText("STATUS: WAITING FOR INPUT...", cx + 30, cy + 140, 10, Fade(LIME, 0.7f));
        DrawText("[ENTER] EXECUTE   [ESC] ABORT", cx + panelW - 180, cy + 170, 10, Fade(LIME, 0.5f));
    }

    EndDrawing();
}
