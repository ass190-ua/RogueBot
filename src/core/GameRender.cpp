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

    EndDrawing();
}