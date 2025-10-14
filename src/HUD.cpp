#include "HUD.hpp"
#include "Game.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>


// FX: Explosión simple (pierdes vida)
void HUD::DrawBurst(Vector2 center, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  float tt = EaseOutCubic(t);
  const int particles = 8;
  float maxR = 26.0f;
  float size = 3.0f * (1.0f - t);

  for (int i = 0; i < particles; ++i) {
    float a = (2.0f * PI / particles) * i;
    Vector2 p = {center.x + cosf(a) * maxR * tt,
                 center.y + sinf(a) * maxR * tt};
    DrawCircleV(p, size, Fade(RED, 1.0f - t));
  }
}

// HUD principal
void HUD::drawPlaying(const Game &game) const {
  // Ayudas de control (arriba a la izquierda)
  DrawText("WASD mover | T toggle | R reinicia | ESC salir", 10, 10, 20,
           RAYWHITE);
  DrawText(TextFormat("Modo: %s", game.movementModeText()), 10, 35, 20,
           RAYWHITE);
  DrawText(TextFormat("RunSeed: %u", game.getRunSeed()), 10, 60, 20, GRAY);
  DrawText(TextFormat("Level: %d/%d  SeedNivel: %u", game.getCurrentLevel(),
                      game.getMaxLevels(), game.getLevelSeed()),
           10, 85, 20, RAYWHITE);
  DrawText("Objetivo: llega al tile VERDE (EXIT)", 10, 110, 20, RAYWHITE);

  const int hpCurrent =
      std::clamp(game.getHP(), 0, game.getHPMax()); // vida actual
  const int hpMax = game.getHPMax();                // vida máxima

  // Detecta cambio de vida y genera efectos
  if (prevHp < 0)
    prevHp = hpCurrent;
  if (hpCurrent != prevHp) {
    if (hpCurrent < prevHp) {
      for (int i = hpCurrent; i < prevHp; ++i)
        hpFx.push_back({HpFx::Type::Lose, i, 0.0f});
    } else {
      for (int i = prevHp; i < hpCurrent; ++i)
        hpFx.push_back({HpFx::Type::Gain, i, 0.0f});
    }
    prevHp = hpCurrent;
  }

  // Geometría de la barra (arriba a la derecha)
  const int hpSlots = hpMax; // nº de segmentos (uno por punto de vida)
  const int slotW = 18;      // ancho del segmento
  const int slotH = 10;      // alto  del segmento
  const int slotGap = 4;     // espacio entre segmentos
  const int hudMargin = 10;  // margen a bordes de pantalla

  const int barWidth = hpSlots * slotW + (hpSlots - 1) * slotGap;
  const int barX = GetScreenWidth() - hudMargin - barWidth;
  const int barY = hudMargin;

  float dtSeconds = GetFrameTime();
  for (auto &e : hpFx)
    e.t += dtSeconds / 0.5f;

  // Marca qué slots están en animación de "ganar vida" y su progreso
  std::vector<bool> slotIsGaining(hpSlots, false);
  std::vector<float> slotGainPct(hpSlots, 0.0f);

  for (const auto &e : hpFx) {
    if (e.type == HpFx::Type::Gain && e.index >= 0 && e.index < hpSlots &&
        e.t < 1.0f) {
      slotIsGaining[e.index] = true;
      float f = std::clamp(EaseOutCubic(e.t), 0.0f, 1.0f);
      slotGainPct[e.index] = std::max(slotGainPct[e.index], f);
    }
  }

  for (int i = 0; i < hpSlots; ++i) {
    int x = barX + i * (slotW + slotGap);
    Rectangle slotRect{(float)x, (float)barY, (float)slotW, (float)slotH};

    bool shouldBeFull = (i < hpCurrent);
    bool animatingGain = slotIsGaining[i];

    if (shouldBeFull && !animatingGain) {
      DrawRectangleRec(slotRect, RED);
      DrawRectangleLinesEx(slotRect, 1.0f, BLACK);
    } else {
      DrawRectangleLinesEx(slotRect, 1.0f, WHITE);
    }
  }

  for (const auto &e : hpFx) {
    int x = barX + e.index * (slotW + slotGap);
    Rectangle slotRect{(float)x, (float)barY, (float)slotW, (float)slotH};
    Vector2 center{slotRect.x + slotRect.width * 0.5f,
                   slotRect.y + slotRect.height * 0.5f};

    if (e.type == HpFx::Type::Gain) {
      // Animación de ganar vida
      float f = std::clamp(EaseOutCubic(e.t), 0.0f, 1.0f);
      int h = (int)(slotH * f);
      int y = barY + (slotH - h);
      DrawRectangle(x, y, slotW, h, Fade(RED, 0.90f));
      DrawRectangleLinesEx(slotRect, 1.0f, BLACK);
    } else {
      // Explosión al perder vida
      DrawBurst(center, e.t);
    }
  }

  hpFx.erase(std::remove_if(hpFx.begin(), hpFx.end(),
                            [](const HpFx &a) { return a.t >= 1.0f; }),
             hpFx.end());

  const int fontSize = 22;
  int textW =
      MeasureText(TextFormat("Vida: %d/%d", hpCurrent, hpMax), fontSize);
  DrawText(TextFormat("Vida: %d/%d", hpCurrent, hpMax),
           GetScreenWidth() - hudMargin - textW, barY + slotH + 6, fontSize,
           RAYWHITE);

  // MINIMAPA
  const Map &m = game.getMap();
  const int mmS = 2;         // tamaño pixel
  int offX = 10, offY = 140; // posición en pantalla

  // Marco del minimapa
  DrawRectangleLines(offX - 2, offY - 2, m.width() * mmS + 4,
                     m.height() * mmS + 4, Color{80, 80, 80, 255});

  for (int y = 0; y < m.height(); ++y) {
    for (int x = 0; x < m.width(); ++x) {
      if (!m.isDiscovered(x, y))
        continue; // solo tiles vistos
      Color c = (m.at(x, y) == WALL) ? Color{60, 60, 60, 255}
                                     : Color{180, 180, 180, 255};
      if (m.at(x, y) == EXIT)
        c = GREEN;
      if (!m.isVisible(x, y))
        c = Color{120, 120, 120, 255}; // fuera del FOV
      DrawRectangle(offX + x * mmS, offY + y * mmS, mmS, mmS, c);
    }
  }

  // Jugador en el minimapa
  DrawRectangle(offX + game.getPlayerX() * mmS, offY + game.getPlayerY() * mmS,
                mmS, mmS, YELLOW);
}

// Overlays de fin de run
void HUD::drawVictory(const Game &game) const {
  DrawRectangle(0, 0, game.getScreenW(), game.getScreenH(),
                Color{0, 0, 0, 150});
  const char *win = "VICTORIA: ¡Has completado los 3 niveles!";
  int tw = MeasureText(win, 32);
  DrawText(win, (game.getScreenW() - tw) / 2, game.getScreenH() / 2 - 16, 32,
           LIME);
  DrawText("Pulsa R para empezar un nuevo run", (game.getScreenW() - 400) / 2,
           game.getScreenH() / 2 + 24, 20, RAYWHITE);
}

void HUD::drawGameOver(const Game &game) const {
  const int sw = game.getScreenW();
  const int sh = game.getScreenH();

  DrawRectangle(0, 0, sw, sh,
                Color{0, 0, 0, 150});

  const float scale = std::min(sw / 1280.0f, sh / 720.0f);
  const int titleSize = (int)std::round(64.0f * scale);
  const int tipSize = (int)std::round(24.0f * scale);
  const int spacing = (int)std::round(12.0f * scale);

  const char* TITLE = "GAME OVER";
  const char* TIP = "Pulsa R para comenzar de nuevo";

  const int titleW = MeasureText(TITLE, titleSize);
  const int tipW = MeasureText(TIP, tipSize);

  const int totalH = titleSize + spacing + tipSize;
  const int titleX = (sw - titleW) / 2;
  const int titleY = sh / 2 - totalH / 2;
  const int tipX = (sw - tipW) / 2;
  const int tipY = titleY + titleSize + spacing;

  DrawText(TITLE, titleX + 2, titleY + 2, titleSize, (Color){0, 0, 0, 180});
  DrawText(TITLE, titleX, titleY, titleSize, RED);

  DrawText(TIP, tipX + 1, tipY + 1, tipSize, (Color){0, 0, 0, 160});
  DrawText(TIP, tipX, tipY, tipSize, RAYWHITE);
}
