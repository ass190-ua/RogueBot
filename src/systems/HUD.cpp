#include "HUD.hpp"
#include "Game.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

static constexpr int kSlotSize = 20; 
static constexpr int kSlotGap = 4;
static constexpr int kMargin = 10;
static constexpr float kHpFxSeconds = 0.5f;

// Dibuja un corazón COMPLETO
static void DrawHeartFull(float x, float y, float size, Color color) {
    float half = size / 2.0f;
    float qtr = size / 4.0f;

    DrawCircle(x + qtr, y + qtr, qtr, color);         // Círculo Izq
    DrawCircle(x + size - qtr, y + qtr, qtr, color);  // Círculo Der

    // Triángulo abajo
    Vector2 v1 = { x, y + qtr };
    Vector2 v2 = { x + half, y + size };
    Vector2 v3 = { x + size, y + qtr };
    DrawTriangle(v1, v2, v3, color);
}

// Dibuja MEDIO corazón (Solo la mitad izquierda)
static void DrawHeartHalf(float x, float y, float size, Color color) {
    float half = size / 2.0f;
    float qtr = size / 4.0f;

    // Solo círculo izquierdo
    DrawCircle(x + qtr, y + qtr, qtr, color);

    // Triángulo rectángulo izquierdo
    Vector2 v1 = { x, y + qtr };
    Vector2 v2 = { x + half, y + size };
    Vector2 v3 = { x + half, y + qtr };
    DrawTriangle(v1, v2, v3, color);
}

void HUD::DrawBurst(Vector2 center, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  float tt = EaseOutCubic(t);
  const int particles = 8;
  const float maxR = 26.0f, size = 3.0f * (1.0f - t);
  for (int i = 0; i < particles; ++i) {
    float a = (2.0f * PI / particles) * i;
    Vector2 p = {center.x + cosf(a) * maxR * tt,
                 center.y + sinf(a) * maxR * tt};
    DrawCircleV(p, size, Fade(RED, 1.0f - t));
  }
}

static void DrawCenteredOverlay(const Game &g, const char *title,
                                Color titleCol, const char *tip) {
  const int sw = g.getScreenW(), sh = g.getScreenH();
  DrawRectangle(0, 0, sw, sh, Color{0, 0, 0, 150});
  const float s = std::min(sw / 1280.0f, sh / 720.0f);
  const int tSz = (int)std::round(64.0f * s),
            tipSz = (int)std::round(24.0f * s),
            gap = (int)std::round(12.0f * s);
  const int tW = MeasureText(title, tSz), tipW = MeasureText(tip, tipSz);
  const int totalH = tSz + gap + tipSz, tX = (sw - tW) / 2,
            tY = sh / 2 - totalH / 2, tipX = (sw - tipW) / 2,
            tipY = tY + tSz + gap;
  DrawText(title, tX + 2, tY + 2, tSz, Color{0, 0, 0, 180});
  DrawText(title, tX, tY, tSz, titleCol);
  DrawText(tip, tipX + 1, tipY + 1, tipSz, Color{0, 0, 0, 160});
  DrawText(tip, tipX, tipY, tipSz, RAYWHITE);
}

// --- ARCHIVO: src/systems/HUD.cpp ---

// --- ARCHIVO: src/systems/HUD.cpp ---

void HUD::drawPlaying(const Game &game) const {
  // 1. TEXTO AYUDA
  DrawText("WASD mover | I/O Zoom | E/(A) Recoger | T Toggle mode", 10, 10, 20, RAYWHITE);
  DrawText("1/(RB) Espada | 2/(RT) Plasma | SHIFT/(LB/LT) Dash", 10, 35, 20, RAYWHITE);
  DrawText(TextFormat("Level: %d/%d", game.getCurrentLevel(), game.getMaxLevels()),
           10, 60, 20, GRAY);

  // 2. CORAZONES (HP) - ARRIBA DERECHA
  const int hpMax = game.getHPMax(); 
  const int hpCurrent = std::clamp(game.getHP(), 0, hpMax);

  if (prevHp < 0) prevHp = hpCurrent;
  if (hpCurrent != prevHp) {
    if (hpCurrent < prevHp)
      for (int i = hpCurrent; i < prevHp; ++i) hpFx.push_back({HpFx::Type::Lose, i, 0.0f});
    else
      for (int i = prevHp; i < hpCurrent; ++i) hpFx.push_back({HpFx::Type::Gain, i, 0.0f});
    prevHp = hpCurrent;
  }
  for (auto &e : hpFx) e.t += GetFrameTime() / kHpFxSeconds;

  const int heartSlots = hpMax / 2; 
  const int barW = heartSlots * kSlotSize + (heartSlots - 1) * kSlotGap;
  const int barX = GetScreenWidth() - kMargin - barW; 
  const int barY = kMargin;

  for (int i = 0; i < heartSlots; ++i) {
    const int x = barX + i * (kSlotSize + kSlotGap);
    int hpValueForThisHeart = (i + 1) * 2;
    int hpValueForHalfHeart = hpValueForThisHeart - 1;

    DrawHeartFull((float)x, (float)barY, (float)kSlotSize, Color{40, 40, 40, 200});

    bool animGain = false;
    for (const auto &e : hpFx) {
         if (e.type == HpFx::Type::Gain && (e.index / 2) == i && e.t < 1.0f) {
             animGain = true; break;
         }
    }

    if (!animGain) {
        if (hpCurrent >= hpValueForThisHeart) {
            DrawHeartFull((float)x, (float)barY, (float)kSlotSize, RED);
            DrawCircle(x + kSlotSize/4 - 1, barY + kSlotSize/4 - 1, 2, WHITE); 
        } 
        else if (hpCurrent >= hpValueForHalfHeart) {
            DrawHeartHalf((float)x, (float)barY, (float)kSlotSize, RED);
            DrawCircle(x + kSlotSize/4 - 1, barY + kSlotSize/4 - 1, 2, WHITE); 
        }
    }
  }

  for (const auto &e : hpFx) {
    int heartIdx = e.index / 2;
    const int x = barX + heartIdx * (kSlotSize + kSlotGap);
    if (e.type == HpFx::Type::Gain) {
      float f = std::clamp(HUD::EaseOutCubic(e.t), 0.0f, 1.0f);
      float currentSize = kSlotSize * f;
      float offset = (kSlotSize - currentSize) / 2.0f;
      DrawHeartFull((float)x + offset, (float)barY + offset, currentSize, Fade(RED, 0.8f));
    } else {
      Vector2 c{x + kSlotSize * 0.5f, barY + kSlotSize * 0.5f};
      DrawBurst(c, e.t);
    }
  }
  hpFx.erase(std::remove_if(hpFx.begin(), hpFx.end(), [](const HpFx &a) { return a.t >= 1.0f; }), hpFx.end());

  // 3. MINIMAPA TÁCTICO (IZQUIERDA)
  const Map &m = game.getMap();
  const float mapX = 10.0f;
  const float mapY = 90.0f;
  const float scale = 4.0f; 
  const float mapW = m.width() * scale;
  const float mapH = m.height() * scale;

  DrawRectangle((int)mapX - 2, (int)mapY - 2, (int)mapW + 4, (int)mapH + 4, Fade(BLACK, 0.7f));
  DrawRectangleLines((int)mapX - 2, (int)mapY - 2, (int)mapW + 4, (int)mapH + 4, GRAY);

  float baseX = mapX;
  float baseY = mapY;

  // Dibujo del mapa (Terreno, Items, Enemies, Player) - IDÉNTICO AL ANTERIOR
  for (int y = 0; y < m.height(); ++y) {
    for (int x = 0; x < m.width(); ++x) {
      if (!m.isDiscovered(x, y)) continue;
      Color c = BLANK; 
      if (m.at(x, y) == WALL) c = Color{80, 80, 80, 255}; 
      else if (m.at(x, y) == EXIT) c = LIME; 
      else {
          if (m.isVisible(x, y)) c = Color{200, 200, 200, 50}; 
          else c = Color{100, 100, 100, 30}; 
      }
      if (c.a > 0) DrawRectangle((int)(baseX + x * scale), (int)(baseY + y * scale), (int)scale, (int)scale, c);
    }
  }
  for (const auto& it : game.getItems()) {
      if (m.isDiscovered(it.tile.x, it.tile.y)) {
          DrawRectangle((int)(baseX + it.tile.x * scale), (int)(baseY + it.tile.y * scale), (int)scale, (int)scale, SKYBLUE);
      }
  }
  for (const auto& e : game.getEnemies()) {
      if (m.isVisible(e.getX(), e.getY())) {
          DrawRectangle((int)(baseX + e.getX() * scale), (int)(baseY + e.getY() * scale), (int)scale, (int)scale, RED);
      }
  }
  DrawRectangle((int)(baseX + game.getPlayerX() * scale), (int)(baseY + game.getPlayerY() * scale), (int)scale, (int)scale, YELLOW);

  // -----------------------------------------------------------------------
  // 4. ESTADOS (STACK VERTICAL - TOP DOWN) - BAJO EL MINIMAPA
  // -----------------------------------------------------------------------
  
  // Empezamos justo debajo del mapa con un margen de 20 píxeles
  int statusX = 10;
  int statusY = (int)(mapY + mapH + 20); 
  const int statusGap = 40; // Espacio entre cada barra

  // A) DASH (Siempre el primero)
  // -----------------------------------------------------------
  // (Si tienes el getter getDashCooldown(), descomenta el bloque)
  {
      float cd = game.getDashCooldown();
      if (cd > 0.0f) {
          DrawText("DASH", statusX, statusY, 10, GRAY);
          DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
          float pct = 1.0f - (cd / 2.0f); 
          DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, ORANGE);
      } else {
          DrawText("DASH LISTO", statusX, statusY + 5, 10, YELLOW);
          DrawRectangleLines(statusX, statusY + 16, 60, 2, YELLOW);
      }
      // Bajamos el cursor
      statusY += statusGap;
  }

  // B) ESCUDO (Debajo del Dash)
  // -----------------------------------------------------------
  if (game.isShieldActive()) {
      DrawText("ESCUDO", statusX, statusY, 10, SKYBLUE);
      DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
      float pct = std::clamp(game.getShieldTime() / 60.0f, 0.0f, 1.0f);
      DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, SKYBLUE);
      
      // Bajamos el cursor
      statusY += statusGap;
  }
  
  // C) GAFAS (Debajo del Escudo o del Dash)
  // -----------------------------------------------------------
  if (game.getGlassesTime() > 0.0f) {
      DrawText("GAFAS 3D", statusX, statusY, 10, PURPLE);
      DrawRectangleLines(statusX, statusY + 12, 60, 6, GRAY);
      float pct = std::clamp(game.getGlassesTime() / 20.0f, 0.0f, 1.0f);
      DrawRectangle(statusX + 1, statusY + 13, (int)(58 * pct), 4, PURPLE);
      
      statusY += statusGap;
  }
}

void HUD::drawVictory(const Game &game) const {
  DrawCenteredOverlay(game, "VICTORIA", LIME, "Pulsa R para comenzar de nuevo");
}

void HUD::drawGameOver(const Game &game) const {
  DrawCenteredOverlay(game, "GAME OVER", RED, "Pulsa R para comenzar de nuevo");
}