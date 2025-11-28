#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <string>

// Renderizado del menu principal
void Game::renderMainMenu() {
    BeginDrawing();
    ClearBackground(BLACK);

    // 1. Título del juego
    // Escalado dinámico: El título ocupa el 14% de la altura de la pantalla
    const char *title = "ROGUEBOT";
    int titleSize = (int)std::round(screenH * 0.14f);
    int titleW = MeasureText(title, titleSize);
    
    // Centrado horizontal (X) y posición vertical (Y) al 30% de la pantalla
    int titleX = (screenW - titleW) / 2;
    int titleY = (int)(screenH * 0.30f) - titleSize;

    // Efecto "Stroke" (Borde): Dibujamos el texto negro 8 veces alrededor
    // para simular un borde grueso.
    int stroke = std::clamp(screenH / 180, 2, 8); // Grosor dinámico (min 2px, max 8px)
    int shadow = stroke * 2;
    
    // Sombra (Desplazada abajo-derecha)
    DrawText(title, titleX + shadow, titleY + shadow, titleSize, Color{90, 0, 0, 255});
    
    // Contorno (8 direcciones)
    DrawText(title, titleX - stroke, titleY, titleSize, BLACK); // Izq
    DrawText(title, titleX + stroke, titleY, titleSize, BLACK); // Der
    DrawText(title, titleX, titleY - stroke, titleSize, BLACK); // Arriba
    DrawText(title, titleX, titleY + stroke, titleSize, BLACK); // Abajo
    // Diagonales...
    DrawText(title, titleX - stroke, titleY - stroke, titleSize, BLACK);
    DrawText(title, titleX + stroke, titleY - stroke, titleSize, BLACK);
    DrawText(title, titleX - stroke, titleY + stroke, titleSize, BLACK);
    DrawText(title, titleX + stroke, titleY + stroke, titleSize, BLACK);
    
    // Texto Principal (Rojo) encima de todo
    DrawText(title, titleX, titleY, titleSize, RED);

    // 2. Dimensiones de los botones
    // Calculamos ancho (bw) y alto (bh) relativos a la pantalla, con límites (clamp)
    // para que no se vean ni diminutos en 4k ni enormes en 800x600.
    int bw = (int)std::round(screenW * 0.46f);
    bw = std::clamp(bw, 360, 720);
    
    int bh = (int)std::round(screenH * 0.12f);
    bh = std::clamp(bh, 80, 128);
    
    int startY = (int)std::round(screenH * 0.52f); // Empiezan un poco más abajo del centro
    int gap = (int)std::round(screenH * 0.045f);   // Espacio vertical entre botones

    // Definición de las áreas de colisión (Rectangles)
    Rectangle playBtn = {(float)((screenW - bw) / 2), (float)startY, (float)bw, (float)bh};
    Rectangle readBtn = {(float)((screenW - bw) / 2), (float)(startY + bh + gap), (float)bw, (float)bh};
    Rectangle quitBtn = {(float)((screenW - bw) / 2), (float)(startY + (bh + gap) * 2), (float)bw, (float)bh};

    Vector2 mp = GetMousePosition();

    // 3. Lambdas de utilidad (UI Logic)
    // Lambda: Dibuja texto con un borde negro fino de 1px
    auto drawOutlinedText = [&](int x, int y, const char *txt, int fs, Color c) {
        DrawText(txt, x - 1, y, fs, BLACK);
        DrawText(txt, x + 1, y, fs, BLACK);
        DrawText(txt, x, y - 1, fs, BLACK);
        DrawText(txt, x, y + 1, fs, BLACK);
        DrawText(txt, x, y, fs, c);
    };

    // Lambda: Algoritmo para partir texto en 2 líneas si no cabe en el ancho (maxW).
    // Busca el espacio ' ' más cercano al centro para dividir la frase equilibradamente.
    auto splitTwoLines = [&](const std::string &s, int fs, int maxW) -> std::pair<std::string, std::string> {
        if (MeasureText(s.c_str(), fs) <= maxW)
            return {s, ""}; // Cabe en una línea
            
        int bestIdx = -1, bestWidth = INT_MAX;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] != ' ') continue; // Solo partimos por espacios
            
            // Simulamos partir aquí
            std::string a = s.substr(0, i);
            std::string b = s.substr(i + 1);
            int wa = MeasureText(a.c_str(), fs);
            int wb = MeasureText(b.c_str(), fs);
            int worst = std::max(wa, wb);
            
            // Buscamos el corte que deje la línea más ancha lo más pequeña posible (equilbrio)
            if (wa <= maxW && wb <= maxW && worst < bestWidth) {
                bestWidth = worst;
                bestIdx = (int)i;
            }
        }
        if (bestIdx >= 0) {
            return {s.substr(0, bestIdx), s.substr(bestIdx + 1)};
        }
        return {s, ""}; // Fallback: no se pudo partir bien
    };

    // Lambda Principal: Dibuja un botón completo (Fondo, Borde, Texto centrado)
    auto drawPixelButton = [&](Rectangle r, const char *label, int index) {
        bool hover    = CheckCollisionPointRec(mp, r);    // Ratón encima
        bool selected = (mainMenuSelection == index);     // Teclado/Gamepad selecciona esto

        // Sombra sólida desplazada (+3px)
        DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width, (int)r.height, Color{0, 0, 0, 120});

        // Definición de colores según estado
        Color baseBg     = Color{25, 25, 30, 255};
        Color hoverBg    = Color{40, 40, 46, 255};
        Color selectedBg = Color{60, 60, 80, 255};

        Color bg = baseBg;
        if (hover)    bg = hoverBg;
        if (selected) bg = selectedBg;  // La selección por teclado tiene prioridad visual o se suma al hover

        DrawRectangleRec(r, bg);

        // Bordes: Doble línea. Roja intensa si está activo, oscura si no.
        bool active = hover || selected;
        Color outer = active ? Color{200, 40, 40, 255} : Color{150, 25, 25, 255};
        Color inner = active ? Color{255, 70, 70, 255} : Color{210, 45, 45, 255};
        
        DrawRectangleLinesEx(r, 4, outer); // Borde grueso externo
        Rectangle innerR = {r.x + 4, r.y + 4, r.width - 8, r.height - 8};
        DrawRectangleLinesEx(innerR, 2, inner); // Borde fino interno

        // Lógica de texto adaptativo
        const int padding = 18;
        int maxW = (int)r.width - padding * 2;
        int fs = (int)std::round(r.height * 0.40f); // Fuente base = 40% altura botón
        if (fs < 18) fs = 18; // Mínimo legible

        // Intentamos ajustar el texto. Si no cabe, reducimos fuente o partimos en 2 líneas.
        std::string s = label;
        auto lines = splitTwoLines(s, fs, maxW);
        
        // Bucle "while": Reduce tamaño fuente mientras el texto se salga
        while (((!lines.second.empty() && (MeasureText(lines.first.c_str(), fs) > maxW ||
                                           MeasureText(lines.second.c_str(), fs) > maxW)) ||
                (lines.second.empty() && MeasureText(lines.first.c_str(), fs) > maxW)) &&
               fs > 14) {
            fs -= 1;
            lines = splitTwoLines(s, fs, maxW);
        }

        // Dibujar el texto centrado (1 o 2 líneas)
        Color txtCol = RAYWHITE;
        if (lines.second.empty()) {
            // Una línea
            int tw = MeasureText(lines.first.c_str(), fs);
            int tx = (int)(r.x + (r.width - tw) / 2);
            int ty = (int)(r.y + (r.height - fs) / 2);
            drawOutlinedText(tx, ty, lines.first.c_str(), fs, txtCol);
        }
        else {
            // Dos líneas
            int tw1 = MeasureText(lines.first.c_str(), fs);
            int tw2 = MeasureText(lines.second.c_str(), fs);
            int totalH = fs * 2 + (int)(fs * 0.20f); // Altura total + pequeño gap
            int baseY = (int)(r.y + (r.height - totalH) / 2);
            
            int tx1 = (int)(r.x + (r.width - tw1) / 2);
            int tx2 = (int)(r.x + (r.width - tw2) / 2);
            
            drawOutlinedText(tx1, baseY, lines.first.c_str(), fs, txtCol);
            drawOutlinedText(tx2, baseY + fs + (int)(fs * 0.20f), lines.second.c_str(), fs, txtCol);
        }
    };

    // 4. Dibujado de elementos
    drawPixelButton(playBtn, "JUGAR", 0);
    drawPixelButton(readBtn, "LEER ANTES DE JUGAR", 1);
    drawPixelButton(quitBtn, "SALIR", 2);

    // Si el usuario abrió la guía, dibujamos el overlay encima de todo
    if (showHelp)
        renderHelpOverlay();

    EndDrawing();
}

// Renderizado del overlay (modal) de ayuda
void Game::renderHelpOverlay() {
    // 1. Fondo semitransparente (Dimming) para oscurecer el menú de atrás
    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 180});

    // 2. Panel Central
    int panelW = (int)std::round(screenW * 0.86f);
    int panelH = (int)std::round(screenH * 0.76f);
    if (panelW > 1500) panelW = 1500; // Max width
    if (panelH > 900)  panelH = 900;  // Max height
    
    int px = (screenW - panelW) / 2;
    int py = (screenH - panelH) / 2;

    // Fondo gris oscuro y borde blanco
    DrawRectangle(px, py, panelW, panelH, Color{20, 20, 20, 255});
    DrawRectangleLines(px, py, panelW, panelH, Color{220, 220, 220, 255});

    // Título del Panel
    const char *title = "Guia de objetos";
    int titleSize = (int)std::round(panelH * 0.06f);
    int tW = MeasureText(title, titleSize);
    DrawText(title, px + (panelW - tW) / 2, py + 12, titleSize, RAYWHITE);

    // 3. Área de Scroll (Viewport)
    int margin = 24;
    int top = py + margin + titleSize + 16; // Y donde empieza el texto
    int left = px + margin;
    int viewportW = panelW - margin * 2;

    int backFs = std::max(16, (int)std::round(panelH * 0.048f)); // Tamaño fuente botón volver
    int footerH = backFs + 16; // Espacio reservado abajo para el botón "Volver"
    int viewportH = panelH - (top - py) - margin - footerH;

    // Scissor mode: Recorta todo lo que se dibuje fuera de este rectángulo.
    // Esencial para hacer scroll sin que el texto se salga del panel.
    BeginScissorMode(left, top, viewportW, viewportH);

    int fontSize = (int)std::round(screenH * 0.022f);
    if (fontSize < 16) fontSize = 16;
    int lineH = (int)std::round(fontSize * 1.25f);

    // Calcular altura total del texto para limitar el scroll
    int lines = 1;
    for (char c : helpText) if (c == '\n') ++lines;
    int totalH = lines * lineH;
    int maxScroll = std::max(0, totalH - viewportH);
    if (helpScroll > maxScroll) helpScroll = maxScroll; // Clamp scroll

    // Dibujado del texto línea a línea
    int y = top - helpScroll; // Aplicar desplazamiento negativo (scroll)
    std::string line; line.reserve(256);
    
    // Iteramos char a char para detectar saltos de línea manuales
    for (size_t i = 0; i <= helpText.size(); ++i) {
        if (i == helpText.size() || helpText[i] == '\n') {
            DrawText(line.c_str(), left, y, fontSize, Color{230, 230, 230, 255});
            y += lineH;
            line.clear();
        }
        else line.push_back(helpText[i]);
    }
    EndScissorMode(); // Fin del recorte

    // 4. Botón "VOLVER" (Footer)
    const char *backTxt = "VOLVER";
    int tw = MeasureText(backTxt, backFs);
    // Posicionado abajo a la derecha
    int tx = px + panelW - tw - 16;
    int ty = py + panelH - backFs - 12;
    
    // Hitbox para el ratón
    Rectangle backHit = {(float)(tx - 6), (float)(ty - 4),
                         (float)(tw + 12), (float)(backFs + 8)};

    bool hover = CheckCollisionPointRec(GetMousePosition(), backHit);
    Color link = hover ? Color{255, 100, 100, 255} : Color{230, 60, 60, 255}; // Rojo brillante si hover

    DrawText(backTxt, tx + 1, ty + 1, backFs, Color{0, 0, 0, 160}); // Sombra
    DrawText(backTxt, tx, ty, backFs, link);

    // Subrayado al pasar el ratón (estilo hipervínculo)
    if (hover) {
        int underlineY = ty + backFs + 2;
        DrawLine(tx, underlineY, tx + tw, underlineY, link);
    }
}
