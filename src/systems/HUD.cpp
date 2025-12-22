#include "HUD.hpp"
#include "Game.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include "I18n.hpp"

// Constantes de diseño UI
static constexpr int kSlotSize = 20;        // Tamaño en px de cada corazón
static constexpr int kSlotGap = 4;          // Espacio entre corazones
static constexpr int kMargin = 10;          // Margen general respecto a los bordes de la pantalla
static constexpr float kHpFxSeconds = 0.5f; // Duración de la animación de pérdida/ganancia de vida

// Dibuja un corazón visualmente compuesto por 2 círculos y un triángulo invertido.
// Se usa para representar la vida llena.
static void DrawHeartFull(float x, float y, float size, Color color) {
    float half = size / 2.0f;
    float qtr = size / 4.0f;

    // 1. Dibujar los dos lóbulos superiores del corazón
    DrawCircle(x + qtr, y + qtr, qtr, color);        // Lóbulo Izquierdo
    DrawCircle(x + size - qtr, y + qtr, qtr, color); // Lóbulo Derecho

    // 2. Dibujar la base (triángulo invertido)
    Vector2 v1 = { x, y + qtr };               // Vértice superior izq (debajo del círculo)
    Vector2 v2 = { x + half, y + size };       // Vértice inferior (punta del corazón)
    Vector2 v3 = { x + size, y + qtr };        // Vértice superior der (debajo del círculo)
    DrawTriangle(v1, v2, v3, color);
}

// Dibuja medio corazón (lado izquierdo).
// Útil para representar daño parcial (ej. medio punto de vida).
static void DrawHeartHalf(float x, float y, float size, Color color) {
    float half = size / 2.0f;
    float qtr = size / 4.0f;

    // 1. Solo dibujamos el lóbulo izquierdo
    DrawCircle(x + qtr, y + qtr, qtr, color);

    // 2. Triángulo rectángulo para la mitad izquierda de la base
    Vector2 v1 = { x, y + qtr };               // Superior Izq
    Vector2 v2 = { x + half, y + size };       // Punta inferior central
    Vector2 v3 = { x + half, y + qtr };        // Centro superior
    DrawTriangle(v1, v2, v3, color);
}

// Efecto de partículas (explosión pequeña) cuando se pierde un corazón.
// Usa coordenadas polares para distribuir partículas en círculo.
void HUD::DrawBurst(Vector2 center, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float tt = EaseOutCubic(t); // Función de easing para movimiento suave
    
    const int particles = 8;    // Número de partículas
    // maxR: radio de expansión, size: las partículas se hacen pequeñas al final
    const float maxR = 26.0f, size = 3.0f * (1.0f - t); 

    for (int i = 0; i < particles; ++i) {
        // Cálculo de coordenadas polares a cartesianas
        float a = (2.0f * PI / particles) * i; // Ángulo
        Vector2 p = {
            center.x + cosf(a) * maxR * tt,
            center.y + sinf(a) * maxR * tt
        };
        // Fade: Se vuelven transparentes al final
        DrawCircleV(p, size, Fade(RED, 1.0f - t)); 
    }
}

// Dibuja una superposición oscura con texto centrado (usado para Game Over y Victoria).
// Escala el texto dinámicamente según la resolución de pantalla.
static void DrawCenteredOverlay(const Game &g, const char *title,
                                Color titleCol, const char *tip) {
    const int sw = g.getScreenW(), sh = g.getScreenH();
    
    // Fondo oscuro semitransparente para bloquear la visión del juego
    DrawRectangle(0, 0, sw, sh, Color{0, 0, 0, 150});

    // Factor de escala basado en una resolución de referencia (720p)
    const float s = std::min(sw / 1280.0f, sh / 720.0f);
    
    // Tamaños escalados
    const int tSz = (int)std::round(64.0f * s);   // Tamaño Título
    const int tipSz = (int)std::round(24.0f * s); // Tamaño Consejo
    const int gap = (int)std::round(12.0f * s);   // Espacio entre líneas

    // Centrado de textos
    const int tW = MeasureText(title, tSz), tipW = MeasureText(tip, tipSz);
    const int totalH = tSz + gap + tipSz;
    const int tX = (sw - tW) / 2;
    const int tY = sh / 2 - totalH / 2;
    const int tipX = (sw - tipW) / 2;
    const int tipY = tY + tSz + gap;

    // Renderizado con sombra simple (+2px offset) para mejor legibilidad
    DrawText(title, tX + 2, tY + 2, tSz, Color{0, 0, 0, 180}); // Sombra
    DrawText(title, tX, tY, tSz, titleCol);                    // Texto principal
    DrawText(tip, tipX + 1, tipY + 1, tipSz, Color{0, 0, 0, 160}); // Sombra tip
    DrawText(tip, tipX, tipY, tipSz, RAYWHITE);                // Texto tip
}

// Bucle principal de dibujo del HUD durante el juego
void HUD::drawPlaying(const Game &game) const {
    // 1. Texto informativo (Esquina superior izquierda)
    DrawText(_("WASD mover | I/O Zoom | E/(A) Recoger | T Toggle mode"), 10, 10, 20, RAYWHITE);
    DrawText(_("1/(RB) Espada | 2/(RT) Plasma | SHIFT/(LB/LT) Dash"), 10, 35, 20, RAYWHITE);
    DrawText(TextFormat(_("Level: %d/%d"), game.getCurrentLevel(), game.getMaxLevels()),
             10, 60, 20, GRAY);

    // 2. Sistema de vida (Corazones) - (Esquina superior derecha)
    const int hpMax = game.getHPMax(); 
    const int hpCurrent = std::clamp(game.getHP(), 0, hpMax);

    // Detección de cambios de HP para generar animaciones (Gain/Lose)
    if (prevHp < 0) prevHp = hpCurrent; // Inicialización
    if (hpCurrent != prevHp) {
        if (hpCurrent < prevHp)
            // Daño recibido: crear animaciones de pérdida
            for (int i = hpCurrent; i < prevHp; ++i) hpFx.push_back({HpFx::Type::Lose, i, 0.0f});
        else
            // Salud recuperada: crear animaciones de ganancia
            for (int i = prevHp; i < hpCurrent; ++i) hpFx.push_back({HpFx::Type::Gain, i, 0.0f});
        prevHp = hpCurrent;
    }
    // Actualizar temporizadores de animaciones
    for (auto &e : hpFx) e.t += GetFrameTime() / kHpFxSeconds;

    // Configuración de la barra de vida
    const int heartSlots = hpMax / 2; // Cada corazón vale 2 HP
    const int barW = heartSlots * kSlotSize + (heartSlots - 1) * kSlotGap;
    const int barX = GetScreenWidth() - kMargin - barW; // Alineado a la derecha
    const int barY = kMargin;

    // Bucle para dibujar cada contenedor de corazón
    for (int i = 0; i < heartSlots; ++i) {
        const int x = barX + i * (kSlotSize + kSlotGap);
        int hpValueForThisHeart = (i + 1) * 2;       // Valor si este corazón está lleno
        int hpValueForHalfHeart = hpValueForThisHeart - 1; // Valor si está a la mitad

        // Fondo del slot (gris oscuro)
        DrawHeartFull((float)x, (float)barY, (float)kSlotSize, Color{40, 40, 40, 200});

        // Verificar si este corazón se está animando (Gain) para no dibujarlo estático todavía
        bool animGain = false;
        for (const auto &e : hpFx) {
             if (e.type == HpFx::Type::Gain && (e.index / 2) == i && e.t < 1.0f) {
                 animGain = true; break;
             }
        }

        // Si no se está animando, dibujamos el estado actual (Lleno o Medio)
        if (!animGain) {
            if (hpCurrent >= hpValueForThisHeart) {
                DrawHeartFull((float)x, (float)barY, (float)kSlotSize, RED);
                // Brillo especular (punto blanco)
                DrawCircle(x + kSlotSize/4 - 1, barY + kSlotSize/4 - 1, 2, WHITE); 
            } 
            else if (hpCurrent >= hpValueForHalfHeart) {
                DrawHeartHalf((float)x, (float)barY, (float)kSlotSize, RED);
                DrawCircle(x + kSlotSize/4 - 1, barY + kSlotSize/4 - 1, 2, WHITE); 
            }
        }
    }

    // Dibujar las animaciones superpuestas (Explosiones o Corazones creciendo)
    for (const auto &e : hpFx) {
        int heartIdx = e.index / 2;
        const int x = barX + heartIdx * (kSlotSize + kSlotGap);
        if (e.type == HpFx::Type::Gain) {
            // Efecto de crecimiento (Scale Up)
            float f = std::clamp(HUD::EaseOutCubic(e.t), 0.0f, 1.0f);
            float currentSize = kSlotSize * f;
            float offset = (kSlotSize - currentSize) / 2.0f;
            DrawHeartFull((float)x + offset, (float)barY + offset, currentSize, Fade(RED, 0.8f));
        } else {
            // Efecto de explosión
            Vector2 c{x + kSlotSize * 0.5f, barY + kSlotSize * 0.5f};
            DrawBurst(c, e.t);
        }
    }
    // Limpieza de animaciones terminadas
    hpFx.erase(std::remove_if(hpFx.begin(), hpFx.end(), [](const HpFx &a) { return a.t >= 1.0f; }), hpFx.end());

    // 3. Minimapa (Izquirda debajo del texto informativo)
    const Map &m = game.getMap();
    const float mapX = 10.0f;
    const float mapY = 90.0f; // Debajo del texto de ayuda
    const float scale = 4.0f; // Cada tile del juego son 4px en el mapa
    const float mapW = m.width() * scale;
    const float mapH = m.height() * scale;

    // Fondo y borde del mapa
    DrawRectangle((int)mapX - 2, (int)mapY - 2, (int)mapW + 4, (int)mapH + 4, Fade(BLACK, 0.7f));
    DrawRectangleLines((int)mapX - 2, (int)mapY - 2, (int)mapW + 4, (int)mapH + 4, GRAY);

    float baseX = mapX;
    float baseY = mapY;

    // Renderizado de Tiles (Lógica de FOV "Fog of War" / Niebla de Guerra)
    for (int y = 0; y < m.height(); ++y) {
        for (int x = 0; x < m.width(); ++x) {
            if (!m.isDiscovered(x, y)) continue; // Si no descubierto, no dibujar nada
            
            Color c = BLANK; 
            if (m.at(x, y) == WALL) c = Color{80, 80, 80, 255}; // Muro
            else if (m.at(x, y) == EXIT) c = LIME;              // Salida
            else {
                // Diferencia entre visible actualmente (claro) vs recordado (oscuro)
                 if (m.isVisible(x, y)) c = Color{200, 200, 200, 50}; 
                 else c = Color{100, 100, 100, 30}; 
            }
            if (c.a > 0) DrawRectangle((int)(baseX + x * scale), (int)(baseY + y * scale), (int)scale, (int)scale, c);
        }
    }
    
    // Renderizado de Items (Azul Cielo)
    for (const auto& it : game.getItems()) {
         if (m.isDiscovered(it.tile.x, it.tile.y)) {
             DrawRectangle((int)(baseX + it.tile.x * scale), (int)(baseY + it.tile.y * scale), (int)scale, (int)scale, SKYBLUE);
         }
    }
    // Renderizado de Enemigos (Rojo) - Solo si son visibles actualmente
    for (const auto& e : game.getEnemies()) {
         if (m.isVisible(e.getX(), e.getY())) {
             DrawRectangle((int)(baseX + e.getX() * scale), (int)(baseY + e.getY() * scale), (int)scale, (int)scale, RED);
         }
    }
    // Renderizado del Jugador (Amarillo)
    DrawRectangle((int)(baseX + game.getPlayerX() * scale), (int)(baseY + game.getPlayerY() * scale), (int)scale, (int)scale, YELLOW);

    // 4. Estados (Stack vertical) - bajo el minimapa
    // Esta sección apila barras de estado dinámicamente. Si una habilidad no se tiene,
    // las siguientes suben para ocupar su lugar.
    
    int statusX = 10;
    int statusY = (int)(mapY + mapH + 20); // Margen de 20px bajo el mapa
    const int statusGap = 40;              // Salto vertical entre barras

    // A) Dash (Siempre visible, muestra Cooldown)
    {
        float cd = game.getDashCooldown();
        if (cd > 0.0f) {
            // En enfriamiento (Barra Naranja disminuyendo)
            DrawText(_("DASH"), statusX, statusY, 10, GRAY);
            DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
            float pct = 1.0f - (cd / 2.0f); // Asumiendo 2.0s de CD total
            DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, ORANGE);
        } else {
            // Disponible (Caja Amarilla brillante)
            DrawText(_("DASH LISTO"), statusX, statusY + 5, 10, YELLOW);
            DrawRectangleLines(statusX, statusY + 16, 60, 2, YELLOW);
        }
        statusY += statusGap; // Mover cursor abajo
    }

    // B) Escudo (Solo si activo)
    if (game.isShieldActive()) {
        DrawText(_("ESCUDO"), statusX, statusY, 10, SKYBLUE);
        DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
        // Calcula porcentaje de tiempo restante (60.0f segundos de duración base)
        float pct = std::clamp(game.getShieldTime() / 60.0f, 0.0f, 1.0f);
        DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, SKYBLUE);
        
        statusY += statusGap;
    }
    
    // C) Gafas 3D (Solo si activo)
    if (game.getGlassesTime() > 0.0f) {
        DrawText(_("GAFAS 3D"), statusX, statusY, 10, PURPLE);
        DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
        float pct = std::clamp(game.getGlassesTime() / 20.0f, 0.0f, 1.0f);
        DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, PURPLE);
        
        statusY += statusGap;
    }

    if (game.isGodMode()) {
        // Dibujarlo centrado arriba o cerca de la vida
        const char* text = _("GOD MODE");
        int w = MeasureText(text, 20);
        int x = (GetScreenWidth() - w) / 2;
        
        // Efecto parpadeo simple con el tiempo
        if ((int)(GetTime() * 2) % 2 == 0) {
            DrawText(text, x, 40, 20, YELLOW); // Color dorado
        }
    }
}

// Pantalla de Victoria
void HUD::drawVictory(const Game &game) const {
    DrawCenteredOverlay(game, _("VICTORIA"), LIME, _("Pulsa R para comenzar de nuevo"));
}

// Pantalla de Derrota
void HUD::drawGameOver(const Game &game) const {
    DrawCenteredOverlay(game, _("GAME OVER"), RED, _("Pulsa R para comenzar de nuevo"));
}
