#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <string>

// Game::renderMainMenu
void Game::renderMainMenu() {
    BeginDrawing();
    ClearBackground(BLACK);

    // Título con contorno escalado + sombra
    const char *title = "ROGUEBOT";
    int titleSize = (int)std::round(screenH * 0.14f);
    int titleW = MeasureText(title, titleSize);
    int titleX = (screenW - titleW) / 2;
    int titleY = (int)(screenH * 0.30f) - titleSize;

    int stroke = std::clamp(screenH / 180, 2, 8);
    int shadow = stroke * 2;
    DrawText(title, titleX + shadow, titleY + shadow, titleSize, Color{90, 0, 0, 255});
    DrawText(title, titleX - stroke, titleY, titleSize, BLACK);
    DrawText(title, titleX + stroke, titleY, titleSize, BLACK);
    DrawText(title, titleX, titleY - stroke, titleSize, BLACK);
    DrawText(title, titleX, titleY + stroke, titleSize, BLACK);
    DrawText(title, titleX - stroke, titleY - stroke, titleSize, BLACK);
    DrawText(title, titleX + stroke, titleY - stroke, titleSize, BLACK);
    DrawText(title, titleX - stroke, titleY + stroke, titleSize, BLACK);
    DrawText(title, titleX + stroke, titleY + stroke, titleSize, BLACK);
    DrawText(title, titleX, titleY, titleSize, RED);

    // Botones (ratón)
    int bw = (int)std::round(screenW * 0.46f);
    bw = std::clamp(bw, 360, 720);
    int bh = (int)std::round(screenH * 0.12f);
    bh = std::clamp(bh, 80, 128);
    int startY = (int)std::round(screenH * 0.52f);
    int gap = (int)std::round(screenH * 0.045f);

    Rectangle playBtn = {(float)((screenW - bw) / 2), (float)startY, (float)bw, (float)bh};
    Rectangle readBtn = {(float)((screenW - bw) / 2), (float)(startY + bh + gap), (float)bw, (float)bh};
    Rectangle quitBtn = {(float)((screenW - bw) / 2), (float)(startY + (bh + gap) * 2), (float)bw, (float)bh};

    Vector2 mp = GetMousePosition();

    auto drawOutlinedText = [&](int x, int y, const char *txt, int fs, Color c) {
        DrawText(txt, x - 1, y, fs, BLACK);
        DrawText(txt, x + 1, y, fs, BLACK);
        DrawText(txt, x, y - 1, fs, BLACK);
        DrawText(txt, x, y + 1, fs, BLACK);
        DrawText(txt, x, y, fs, c);
    };

    // Parte un texto en 2 líneas si es necesario para que quepa en 'maxW'
    auto splitTwoLines = [&](const std::string &s, int fs, int maxW) -> std::pair<std::string, std::string> {
        if (MeasureText(s.c_str(), fs) <= maxW)
            return {s, ""};
        // Busca el salto óptimo (por espacios)
        int bestIdx = -1, bestWidth = INT_MAX;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] != ' ')
                continue;
            std::string a = s.substr(0, i);
            std::string b = s.substr(i + 1);
            int wa = MeasureText(a.c_str(), fs);
            int wb = MeasureText(b.c_str(), fs);
            int worst = std::max(wa, wb);
            if (wa <= maxW && wb <= maxW && worst < bestWidth) {
                bestWidth = worst;
                bestIdx = (int)i;
            }
        }
        if (bestIdx >= 0) {
            return {s.substr(0, bestIdx), s.substr(bestIdx + 1)};
        }
        return {s, ""};
    };

    auto drawPixelButton = [&](Rectangle r, const char *label) {
        bool hover = CheckCollisionPointRec(mp, r);

        // Sombra
        DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width, (int)r.height, Color{0, 0, 0, 120});

        // Fondo
        Color baseBg = Color{25, 25, 30, 255};
        Color hoverBg = Color{40, 40, 46, 255};
        DrawRectangleRec(r, hover ? hoverBg : baseBg);

        // Bordes doble rojo
        Color outer = hover ? Color{200, 40, 40, 255} : Color{150, 25, 25, 255};
        Color inner = hover ? Color{255, 70, 70, 255} : Color{210, 45, 45, 255};
        DrawRectangleLinesEx(r, 4, outer);
        Rectangle innerR = {r.x + 4, r.y + 4, r.width - 8, r.height - 8};
        DrawRectangleLinesEx(innerR, 2, inner);

        // Texto: intenta 1 línea; si no cabe, 2 líneas; si no, reduce fuente
        const int padding = 18;
        int maxW = (int)r.width - padding * 2;

        int fs = (int)std::round(r.height * 0.40f); // base
        if (fs < 18) fs = 18;

        std::string s = label;
        auto lines = splitTwoLines(s, fs, maxW);
        while (((!lines.second.empty() && (MeasureText(lines.first.c_str(), fs) > maxW ||
                                           MeasureText(lines.second.c_str(), fs) > maxW)) ||
                (lines.second.empty() && MeasureText(lines.first.c_str(), fs) > maxW)) &&
               fs > 14) {
            fs -= 1;
            lines = splitTwoLines(s, fs, maxW);
        }

        // Dibujo centrado
        Color txtCol = RAYWHITE;
        if (lines.second.empty()) {
            int tw = MeasureText(lines.first.c_str(), fs);
            int tx = (int)(r.x + (r.width - tw) / 2);
            int ty = (int)(r.y + (r.height - fs) / 2);
            drawOutlinedText(tx, ty, lines.first.c_str(), fs, txtCol);
        }
        else {
            int tw1 = MeasureText(lines.first.c_str(), fs);
            int tw2 = MeasureText(lines.second.c_str(), fs);
            int totalH = fs * 2 + (int)(fs * 0.20f);
            int baseY = (int)(r.y + (r.height - totalH) / 2);
            int tx1 = (int)(r.x + (r.width - tw1) / 2);
            int tx2 = (int)(r.x + (r.width - tw2) / 2);
            drawOutlinedText(tx1, baseY, lines.first.c_str(), fs, txtCol);
            drawOutlinedText(tx2, baseY + fs + (int)(fs * 0.20f), lines.second.c_str(), fs, txtCol);
        }
    };

    drawPixelButton(playBtn, "JUGAR");
    drawPixelButton(readBtn, "LEER ANTES DE JUGAR");
    drawPixelButton(quitBtn, "SALIR");

    // Overlay de ayuda (si está abierto)
    if (showHelp)
        renderHelpOverlay();

    EndDrawing();
}

// Game::renderHelpOverlay
void Game::renderHelpOverlay() {
    // Fondo oscuro translúcido
    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 180});

    // Panel más ancho/alto
    int panelW = (int)std::round(screenW * 0.86f);
    int panelH = (int)std::round(screenH * 0.76f);
    if (panelW > 1500) panelW = 1500;
    if (panelH > 900)  panelH = 900;
    int px = (screenW - panelW) / 2;
    int py = (screenH - panelH) / 2;

    DrawRectangle(px, py, panelW, panelH, Color{20, 20, 20, 255});
    DrawRectangleLines(px, py, panelW, panelH, Color{220, 220, 220, 255});

    // Título
    const char *title = "Guia de objetos";
    int titleSize = (int)std::round(panelH * 0.06f);
    int tW = MeasureText(title, titleSize);
    DrawText(title, px + (panelW - tW) / 2, py + 12, titleSize, RAYWHITE);

    // Márgenes y viewport (reservando footer para el enlace)
    int margin = 24;
    int top = py + margin + titleSize + 16;
    int left = px + margin;
    int viewportW = panelW - margin * 2;

    int backFs = std::max(16, (int)std::round(panelH * 0.048f));
    int footerH = backFs + 16;
    int viewportH = panelH - (top - py) - margin - footerH;

    // Texto scrollable
    BeginScissorMode(left, top, viewportW, viewportH);

    int fontSize = (int)std::round(screenH * 0.022f);
    if (fontSize < 16) fontSize = 16;
    int lineH = (int)std::round(fontSize * 1.25f);

    // Clamp de scroll
    int lines = 1;
    for (char c : helpText) if (c == '\n') ++lines;
    int totalH = lines * lineH;
    int maxScroll = std::max(0, totalH - viewportH);
    if (helpScroll > maxScroll) helpScroll = maxScroll;

    int y = top - helpScroll;
    std::string line; line.reserve(256);
    for (size_t i = 0; i <= helpText.size(); ++i) {
        if (i == helpText.size() || helpText[i] == '\n') {
            DrawText(line.c_str(), left, y, fontSize, Color{230, 230, 230, 255});
            y += lineH;
            line.clear();
        }
        else line.push_back(helpText[i]);
    }
    EndScissorMode();

    // Enlace "VOLVER"
    const char *backTxt = "VOLVER";
    int tw = MeasureText(backTxt, backFs);
    int tx = px + panelW - tw - 16;
    int ty = py + panelH - backFs - 12;
    Rectangle backHit = {(float)(tx - 6), (float)(ty - 4),
                         (float)(tw + 12), (float)(backFs + 8)};

    bool hover = CheckCollisionPointRec(GetMousePosition(), backHit);
    Color link = hover ? Color{255, 100, 100, 255} : Color{230, 60, 60, 255};

    DrawText(backTxt, tx + 1, ty + 1, backFs, Color{0, 0, 0, 160});
    DrawText(backTxt, tx, ty, backFs, link);

    if (hover) {
        int underlineY = ty + backFs + 2;
        DrawLine(tx, underlineY, tx + tw, underlineY, link);
    }
}
