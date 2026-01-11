#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include "I18n.hpp"
#include <algorithm>
#include <cmath>
#include <climits>
#include <string>

// Renderizado del menu principal
void Game::renderMainMenu()
{
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
    auto drawOutlinedText = [&](int x, int y, const char *txt, int fs, Color c)
    {
        DrawText(txt, x - 1, y, fs, BLACK);
        DrawText(txt, x + 1, y, fs, BLACK);
        DrawText(txt, x, y - 1, fs, BLACK);
        DrawText(txt, x, y + 1, fs, BLACK);
        DrawText(txt, x, y, fs, c);
    };

    // Lambda: Algoritmo para partir texto en 2 líneas si no cabe en el ancho (maxW).
    // Busca el espacio ' ' más cercano al centro para dividir la frase equilibradamente.
    auto splitTwoLines = [&](const std::string &s, int fs, int maxW) -> std::pair<std::string, std::string>
    {
        if (MeasureText(s.c_str(), fs) <= maxW)
            return {s, ""}; // Cabe en una línea

        int bestIdx = -1, bestWidth = INT_MAX;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] != ' ')
                continue; // Solo partimos por espacios

            // Simulamos partir aquí
            std::string a = s.substr(0, i);
            std::string b = s.substr(i + 1);
            int wa = MeasureText(a.c_str(), fs);
            int wb = MeasureText(b.c_str(), fs);
            int worst = std::max(wa, wb);

            // Buscamos el corte que deje la línea más ancha lo más pequeña posible (equilbrio)
            if (wa <= maxW && wb <= maxW && worst < bestWidth)
            {
                bestWidth = worst;
                bestIdx = (int)i;
            }
        }
        if (bestIdx >= 0)
        {
            return {s.substr(0, bestIdx), s.substr(bestIdx + 1)};
        }
        return {s, ""}; // Fallback: no se pudo partir bien
    };

    // Lambda Principal: Dibuja un botón completo (Fondo, Borde, Texto centrado)
    auto drawPixelButton = [&](Rectangle r, const char *label, int index)
    {
        bool hover = CheckCollisionPointRec(mp, r);   // Ratón encima
        bool selected = (mainMenuSelection == index); // Teclado/Gamepad selecciona esto

        // Sombra sólida desplazada (+3px)
        DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width, (int)r.height, Color{0, 0, 0, 120});

        // Definición de colores según estado
        Color baseBg = Color{25, 25, 30, 255};
        Color hoverBg = Color{40, 40, 46, 255};
        Color selectedBg = Color{60, 60, 80, 255};

        Color bg = baseBg;
        if (hover)
            bg = hoverBg;
        if (selected)
            bg = selectedBg; // La selección por teclado tiene prioridad visual o se suma al hover

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
        if (fs < 18)
            fs = 18; // Mínimo legible

        // Intentamos ajustar el texto. Si no cabe, reducimos fuente o partimos en 2 líneas.
        std::string s = label;
        auto lines = splitTwoLines(s, fs, maxW);

        // Bucle "while": Reduce tamaño fuente mientras el texto se salga
        while (((!lines.second.empty() && (MeasureText(lines.first.c_str(), fs) > maxW ||
                                           MeasureText(lines.second.c_str(), fs) > maxW)) ||
                (lines.second.empty() && MeasureText(lines.first.c_str(), fs) > maxW)) &&
               fs > 14)
        {
            fs -= 1;
            lines = splitTwoLines(s, fs, maxW);
        }

        // Dibujar el texto centrado (1 o 2 líneas)
        Color txtCol = RAYWHITE;
        if (lines.second.empty())
        {
            // Una línea
            int tw = MeasureText(lines.first.c_str(), fs);
            int tx = (int)(r.x + (r.width - tw) / 2);
            int ty = (int)(r.y + (r.height - fs) / 2);
            drawOutlinedText(tx, ty, lines.first.c_str(), fs, txtCol);
        }
        else
        {
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
    drawPixelButton(playBtn, _("JUGAR"), 0);
    drawPixelButton(readBtn, _("TUTORIAL"), 1);
    drawPixelButton(quitBtn, _("SALIR"), 2);

    // 5. Botón de ajustes en la esquina inferior derecha
    // Calculamos un tamaño proporcional a la altura de la pantalla con límites para no desaparecer en resoluciones grandes o pequeñas.
    int settingsSize = (int)std::round(screenH * 0.08f);
    if (settingsSize < 48)
        settingsSize = 48;
    if (settingsSize > 120)
        settingsSize = 120;
    Rectangle settingsRect = {(float)(screenW - settingsSize - 20), (float)(screenH - settingsSize - 20), (float)settingsSize, (float)settingsSize};
    // Dibujamos el botón de ajustes usando el mismo estilo de botones. Como índice usamos 3 para que no interfiera con la selección de teclado.
    drawPixelButton(settingsRect, _("AJUSTES"), 3);

    // Si el usuario abrió la guía, dibujamos el overlay encima de todo
    if (showHelp)
        renderHelpOverlay();

    EndDrawing();
}

// Renderizado del overlay (modal) de ayuda
void Game::renderHelpOverlay()
{
    // 1. Fondo semitransparente (Dimming) para oscurecer el menú de atrás
    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 180});

    // 2. Panel Central
    int panelW = (int)std::round(screenW * 0.86f);
    int panelH = (int)std::round(screenH * 0.76f);
    if (panelW > 1500)
        panelW = 1500; // Max width
    if (panelH > 900)
        panelH = 900; // Max height

    int px = (screenW - panelW) / 2;
    int py = (screenH - panelH) / 2;

    // Fondo gris oscuro y borde blanco
    DrawRectangle(px, py, panelW, panelH, Color{20, 20, 20, 255});
    DrawRectangleLines(px, py, panelW, panelH, Color{220, 220, 220, 255});

    // Título del Panel
    const char *title = _("Guia de objetos");
    int titleSize = (int)std::round(panelH * 0.06f);
    int tW = MeasureText(title, titleSize);
    DrawText(title, px + (panelW - tW) / 2, py + 12, titleSize, RAYWHITE);

    // 3. Área de Scroll (Viewport)
    int margin = 24;
    int top = py + margin + titleSize + 16; // Y donde empieza el texto
    int left = px + margin;
    int viewportW = panelW - margin * 2;

    int backFs = std::max(16, (int)std::round(panelH * 0.048f)); // Tamaño fuente botón volver
    int footerH = backFs + 16;                                   // Espacio reservado abajo para el botón "Volver"
    int viewportH = panelH - (top - py) - margin - footerH;

    // Scissor mode: Recorta todo lo que se dibuje fuera de este rectángulo.
    // Esencial para hacer scroll sin que el texto se salga del panel.
    BeginScissorMode(left, top, viewportW, viewportH);

    int fontSize = (int)std::round(screenH * 0.022f);
    if (fontSize < 16)
        fontSize = 16;
    int lineH = (int)std::round(fontSize * 1.25f);

    // Calcular altura total del texto para limitar el scroll
    int lines = 1;
    for (char c : helpText)
        if (c == '\n')
            ++lines;
    int totalH = lines * lineH;
    int maxScroll = std::max(0, totalH - viewportH);
    if (helpScroll > maxScroll)
        helpScroll = maxScroll; // Clamp scroll

    // Dibujado del texto línea a línea
    int y = top - helpScroll; // Aplicar desplazamiento negativo (scroll)
    std::string line;
    line.reserve(256);

    // Iteramos char a char para detectar saltos de línea manuales
    for (size_t i = 0; i <= helpText.size(); ++i)
    {
        if (i == helpText.size() || helpText[i] == '\n')
        {
            DrawText(line.c_str(), left, y, fontSize, Color{230, 230, 230, 255});
            y += lineH;
            line.clear();
        }
        else
            line.push_back(helpText[i]);
    }
    EndScissorMode(); // Fin del recorte

    // 4. Botón "VOLVER" (Footer)
    const char *backTxt = _("VOLVER");
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
    if (hover)
    {
        int underlineY = ty + backFs + 2;
        DrawLine(tx, underlineY, tx + tw, underlineY, link);
    }
}

void Game::renderOptionsMenu()
{
    BeginDrawing();
    ClearBackground(BLACK);

    // 1. Título "OPCIONES"
    const char *title = _("OPCIONES");
    int titleFontSize = screenH / 10;
    int titleWidth = MeasureText(title, titleFontSize);
    int titleX = (screenW - titleWidth) / 2;
    int titleY = screenH / 8;

    DrawText(title, titleX, titleY, titleFontSize, RED);

    // 2. Configuración de Botones
    int btnW = screenW / 3;
    int btnH = screenH / 12;
    int centerX = screenW / 2;
    Vector2 mp = GetMousePosition();

    // Lambda para dibujar botones
    auto drawSimpleButton = [&](Rectangle r, const char *label, int index)
    {
        // IMPORTANTE: Si hay alerta (showDifficultyWarning), desactivamos el hover visual
        // de los botones de fondo para que no distraigan.
        bool hover = !showDifficultyWarning && CheckCollisionPointRec(mp, r);
        bool selected = !showDifficultyWarning && (mainMenuSelection == index);

        // Sombra
        DrawRectangle((int)r.x + 3, (int)r.y + 3, (int)r.width, (int)r.height, Color{0, 0, 0, 120});

        // Colores dinámicos
        Color bg = Color{25, 25, 30, 255};
        if (hover)
            bg = Color{40, 40, 46, 255};
        if (selected)
            bg = Color{60, 60, 80, 255};

        Color outer = (hover || selected) ? Color{200, 40, 40, 255} : Color{150, 25, 25, 255};
        Color inner = (hover || selected) ? Color{255, 70, 70, 255} : Color{210, 45, 45, 255};

        // Dibujado
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, 4, outer);
        Rectangle innerR = {r.x + 4, r.y + 4, r.width - 8, r.height - 8};
        DrawRectangleLinesEx(innerR, 2, inner);

        // Texto centrado
        int fs = (int)std::round(r.height * 0.40f);
        if (fs < 18)
            fs = 18;
        int tw = MeasureText(label, fs);
        int tx = (int)(r.x + (r.width - tw) / 2);
        int ty = (int)(r.y + (r.height - fs) / 2);

        DrawText(label, tx + 1, ty + 1, fs, BLACK);
        DrawText(label, tx, ty, fs, RAYWHITE);
    };

    // Botón DIFICULTAD (índice 0)
    Rectangle diffRect = {(float)(centerX - btnW / 2), (float)(screenH / 3), (float)btnW, (float)btnH};
    drawSimpleButton(diffRect, getDifficultyLabel(pendingDifficulty), 0);

    // Botón IDIOMA (índice 1) -> Nuevo
    Rectangle langRect = {(float)(centerX - btnW / 2),
                          diffRect.y + diffRect.height + btnH * 0.2f, // separación vertical similar a la usada con el slider
                          (float)btnW, (float)btnH};
    drawSimpleButton(langRect, getLanguageLabel().c_str(), 1);

    // Botón VOLVER (índice 2)
    Rectangle backRect = {(float)(centerX - btnW / 2),
                          (float)(screenH - screenH / 4),
                          (float)btnW, (float)btnH};
    drawSimpleButton(backRect, _("VOLVER"), 2);

    // 3. OVERLAY DE ADVERTENCIA
    if (showDifficultyWarning)
    {
        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 220});

        int boxW = (int)(screenW * 0.65f); // Un poco más ancho para textos largos
        int boxH = (int)(screenH * 0.4f);
        int boxX = (screenW - boxW) / 2;
        int boxY = (screenH - boxH) / 2;

        DrawRectangle(boxX, boxY, boxW, boxH, Color{40, 10, 10, 255});
        DrawRectangleLinesEx({(float)boxX, (float)boxY, (float)boxW, (float)boxH}, 4, RED);

        const char *h1 = _("ADVERTENCIA");
        const char *h2 = _("Cambiar la dificultad reiniciara la partida.");
        const char *h3 = _("Se perdera todo el progreso actual.");

        // CAMBIO 2: Texto dinámico según el último input
        const char *h4;
        if (lastInput == InputDevice::Gamepad)
        {
            h4 = _("(A) ACEPTAR      (B) CANCELAR");
        }
        else
        {
            h4 = _("[ENTER] ACEPTAR      [ESC] CANCELAR");
        }

        int fs1 = 40, fs2 = 20, fs3 = 20, fs4 = 20;

        DrawText(h1, boxX + (boxW - MeasureText(h1, fs1)) / 2, boxY + 40, fs1, RED);
        DrawText(h2, boxX + (boxW - MeasureText(h2, fs2)) / 2, boxY + 110, fs2, RAYWHITE);
        DrawText(h3, boxX + (boxW - MeasureText(h3, fs3)) / 2, boxY + 140, fs3, ORANGE);

        if ((int)(GetTime() * 2) % 2 == 0)
        {
            DrawText(h4, boxX + (boxW - MeasureText(h4, fs4)) / 2, boxY + boxH - 50, fs4, YELLOW);
        }
        else
        {
            DrawText(h4, boxX + (boxW - MeasureText(h4, fs4)) / 2, boxY + boxH - 50, fs4, GRAY);
        }
    }

    // ---------- BARRA DE VOLUMEN ----------
    float sliderMarginY = screenH * 0.09f; // separación entre botón dificultad y barra
    float sliderW = (float)btnW;
    float sliderH = btnH / 8.0f; // pista finita
    float sliderX = (float)(centerX - btnW / 2);
    float sliderY = langRect.y + langRect.height + sliderMarginY;

    Rectangle sliderRect = {sliderX, sliderY, sliderW, sliderH};

    // Fondo de la pista
    DrawRectangle(sliderRect.x, sliderRect.y,
                  sliderRect.width, sliderRect.height,
                  Color{25, 25, 30, 255});

    // Porción rellena según audioVolume
    int fillW = (int)std::round(sliderRect.width * audioVolume);
    DrawRectangle(sliderRect.x, sliderRect.y,
                  fillW, sliderRect.height,
                  Color{210, 45, 45, 255});

    // Borde de la pista
    DrawRectangleLines(sliderRect.x, sliderRect.y,
                       sliderRect.width, sliderRect.height,
                       Color{150, 25, 25, 255});

    // Manejador (handle) del slider
    int handleW = (int)std::round(sliderRect.height * 1.8f);
    int handleH = (int)std::round(sliderRect.height * 2.5f);
    float handleX = sliderRect.x + fillW - handleW / 2.0f;
    float handleY = sliderRect.y - (handleH - sliderRect.height) / 2.0f;
    handleX = std::clamp(handleX,
                         sliderRect.x - handleW / 2.0f,
                         sliderRect.x + sliderRect.width - handleW / 2.0f);

    DrawRectangle(handleX, handleY, handleW, handleH, Color{60, 60, 80, 255});
    DrawRectangleLines(handleX, handleY, handleW, handleH, Color{200, 40, 40, 255});

    // Texto "Volumen: XX%" encima de la barra
    std::string volStr = getVolumeLabel();
    int volFs = (int)std::round(btnH * 0.40f);
    if (volFs < 16)
        volFs = 16;
    int volW = MeasureText(volStr.c_str(), volFs);
    int volX = centerX - volW / 2;
    int volY = (int)(sliderRect.y - volFs - 8);

    DrawText(volStr.c_str(), volX + 1, volY + 1, volFs, Color{0, 0, 0, 160});
    DrawText(volStr.c_str(), volX, volY, volFs, RAYWHITE);

    EndDrawing();
}

void Game::renderPauseMenu()
{
    // 1. Overlay Semitransparente (Oscurecer el juego)
    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 160});

    // 2. Título "PAUSA"
    const char *title = _("PAUSA");
    int tSize = 60;
    int tW = MeasureText(title, tSize);
    int centerX = screenW / 2;
    int centerY = screenH / 2;

    DrawText(title, centerX - tW / 2, centerY - 150, tSize, RAYWHITE);

    // 3. Botones (Reutilizando lógica o haciendo simple)
    // Definir rectángulos
    int btnW = 300;
    int btnH = 60;
    int gap = 20;
    int startY = centerY - 50;

    Rectangle btnResume = {(float)centerX - btnW / 2, (float)startY, (float)btnW, (float)btnH};
    Rectangle btnSettings = {(float)centerX - btnW / 2, (float)startY + btnH + gap, (float)btnW, (float)btnH};
    Rectangle btnExit = {(float)centerX - btnW / 2, (float)startY + (btnH + gap) * 2, (float)btnW, (float)btnH};

    // Lambda simple de dibujado (puedes copiar la compleja de renderMainMenu si quieres consistencia total)
    auto drawBtn = [&](Rectangle r, const char *txt, int idx)
    {
        bool selected = (pauseSelection == idx);
        // Mouse Hover
        if (CheckCollisionPointRec(GetMousePosition(), r))
        {
            pauseSelection = idx; // El ratón toma prioridad visual
            selected = true;
        }

        Color col = selected ? Color{60, 60, 80, 255} : Color{25, 25, 30, 255};
        Color border = selected ? RED : MAROON;

        DrawRectangleRec(r, col);
        DrawRectangleLinesEx(r, 3, border);

        int fs = 30;
        int txtW = MeasureText(txt, fs);
        DrawText(txt, r.x + (r.width - txtW) / 2, r.y + (r.height - fs) / 2, fs, RAYWHITE);
    };

    drawBtn(btnResume, _("REANUDAR"), 0);
    drawBtn(btnSettings, _("AJUSTES"), 1);
    drawBtn(btnExit, _("SALIR AL MENU"), 2);
}
