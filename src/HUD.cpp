#include "HUD.hpp"
#include "Game.hpp" 
#include "raylib.h"

void HUD::drawPlaying(const Game& game) const {
    DrawText("WASD mover | T toggle | R reinicia | ESC salir", 10, 10, 20, RAYWHITE);
    DrawText(TextFormat("Modo: %s", game.movementModeText()), 10, 35, 20, RAYWHITE);
    DrawText(TextFormat("RunSeed: %u", game.getRunSeed()), 10, 60, 20, GRAY);
    DrawText(TextFormat("Level: %d/%d  SeedNivel: %u",
                        game.getCurrentLevel(), game.getMaxLevels(), game.getLevelSeed()),
             10, 85, 20, RAYWHITE);
    DrawText("Objetivo: llega al tile VERDE (EXIT)", 10, 110, 20, RAYWHITE);

    // --- MINIMAPA ---
    const Map& m = game.getMap();
    const int mmS = 2;                 // tamaño pixel por tile en minimapa
    int offX = 10, offY = 140;         // posición en pantalla (ajusta si quieres)

    // marco del minimapa (opcional)
    DrawRectangleLines(offX-2, offY-2, m.width()*mmS + 4, m.height()*mmS + 4, Color{80,80,80,255});

    for (int y = 0; y < m.height(); ++y) {
        for (int x = 0; x < m.width(); ++x) {
            if (!m.isDiscovered(x,y)) continue; // solo tiles vistos
            Color c = (m.at(x,y) == WALL) ? Color{60,60,60,255} : Color{180,180,180,255};
            if (m.at(x,y) == EXIT) c = GREEN;
            if (!m.isVisible(x,y)) { c = Color{120,120,120,255}; } // más claro si fuera del FOV
            DrawRectangle(offX + x*mmS, offY + y*mmS, mmS, mmS, c);
        }
    }
    // jugador en el minimapa
    DrawRectangle(offX + game.getPlayerX()*mmS, offY + game.getPlayerY()*mmS, mmS, mmS, YELLOW);

}

void HUD::drawVictory(const Game& game) const {
    DrawRectangle(0, 0, game.getScreenW(), game.getScreenH(), Color{0,0,0,150});
    const char* win = "VICTORIA: ¡Has completado los 3 niveles!";
    int tw = MeasureText(win, 32);
    DrawText(win, (game.getScreenW() - tw)/2, game.getScreenH()/2 - 16, 32, LIME);
    DrawText("Pulsa R para empezar un nuevo run",
             (game.getScreenW() - 400)/2, game.getScreenH()/2 + 24, 20, RAYWHITE);
}

void HUD::drawGameOver(const Game& game) const {
    DrawRectangle(0, 0, game.getScreenW(), game.getScreenH(), Color{0,0,0,150});
    const char* txt = "GAME OVER";
    int tw = MeasureText(txt, 32);
    DrawText(txt, (game.getScreenW() - tw)/2, game.getScreenH()/2 - 16, 32, RED);
    DrawText("Pulsa R para reintentar", (game.getScreenW() - 320)/2, game.getScreenH()/2 + 24, 20, RAYWHITE);
}
