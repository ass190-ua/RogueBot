#include "HUD.hpp"
#include "Game.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>

static constexpr int kSlotW = 18, kSlotH = 10, kSlotGap = 4, kMargin = 10;
static constexpr float kHpFxSeconds = 0.5f;

// FX: Explosión simple (pierdes vida)
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

void HUD::drawPlaying(const Game &game) const {
  // Ayudas de control
  DrawText("WASD mover | T toggle | R reinicia | ESC salir", 10, 10, 20,
           RAYWHITE);
  DrawText(TextFormat("Modo: %s", game.movementModeText()), 10, 35, 20,
           RAYWHITE);
  DrawText(TextFormat("RunSeed: %u", game.getRunSeed()), 10, 60, 20, GRAY);
  DrawText(TextFormat("Level: %d/%d  SeedNivel: %u", game.getCurrentLevel(),
                      game.getMaxLevels(), game.getLevelSeed()),
           10, 85, 20, RAYWHITE);
  DrawText("Objetivo: llega al tile VERDE (EXIT)", 10, 110, 20, RAYWHITE);

  // Vida actual y FX
  const int hpMax = game.getHPMax();
  const int hpCurrent = std::clamp(game.getHP(), 0, hpMax);
  if (prevHp < 0)
    prevHp = hpCurrent;
  if (hpCurrent != prevHp) {
    if (hpCurrent < prevHp)
      for (int i = hpCurrent; i < prevHp; ++i)
        hpFx.push_back({HpFx::Type::Lose, i, 0.0f});
    else
      for (int i = prevHp; i < hpCurrent; ++i)
        hpFx.push_back({HpFx::Type::Gain, i, 0.0f});
    prevHp = hpCurrent;
  }
  for (auto &e : hpFx)
    e.t += GetFrameTime() / kHpFxSeconds;

  // Barra de vida
  const int slots = hpMax;
  const int barW = slots * kSlotW + (slots - 1) * kSlotGap;
  const int barX = GetScreenWidth() - kMargin - barW, barY = kMargin;

  for (int i = 0; i < slots; ++i) {
    const int x = barX + i * (kSlotW + kSlotGap);
    const Rectangle r{(float)x, (float)barY, (float)kSlotW, (float)kSlotH};
    bool animGain = false;
    for (const auto &e : hpFx)
      if (e.type == HpFx::Type::Gain && e.index == i && e.t < 1.0f) {
        animGain = true;
        break;
      }
    if (i < hpCurrent && !animGain) {
      DrawRectangleRec(r, RED);
      DrawRectangleLinesEx(r, 1.0f, BLACK);
    } else {
      DrawRectangleLinesEx(r, 1.0f, WHITE);
    }
  }

  for (const auto &e : hpFx) {
    const int x = barX + e.index * (kSlotW + kSlotGap);
    const Rectangle r{(float)x, (float)barY, (float)kSlotW, (float)kSlotH};
    if (e.type == HpFx::Type::Gain) {
      float f = std::clamp(EaseOutCubic(e.t), 0.0f, 1.0f);
      const int h = (int)(kSlotH * f), y = barY + (kSlotH - h);
      DrawRectangle(x, y, kSlotW, h, Fade(RED, 0.90f));
      DrawRectangleLinesEx(r, 1.0f, BLACK);
    } else {
      Vector2 c{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
      DrawBurst(c, e.t);
    }
  }
  hpFx.erase(std::remove_if(hpFx.begin(), hpFx.end(),
                            [](const HpFx &a) { return a.t >= 1.0f; }),
             hpFx.end());

  const int fs = 22,
            txtW = MeasureText(TextFormat("Vida: %d/%d", hpCurrent, hpMax), fs);
  DrawText(TextFormat("Vida: %d/%d", hpCurrent, hpMax),
           GetScreenWidth() - kMargin - txtW, barY + kSlotH + 6, fs, RAYWHITE);

  // Minimap
  const Map &m = game.getMap();
  const int mmS = 2, offX = 10, offY = 140;
  DrawRectangleLines(offX - 2, offY - 2, m.width() * mmS + 4,
                     m.height() * mmS + 4, Color{80, 80, 80, 255});
  for (int y = 0; y < m.height(); ++y)
    for (int x = 0; x < m.width(); ++x) {
      if (!m.isDiscovered(x, y))
        continue;
      Color c = (m.at(x, y) == WALL) ? Color{60, 60, 60, 255}
                                     : Color{180, 180, 180, 255};
      if (m.at(x, y) == EXIT)
        c = GREEN;
      if (!m.isVisible(x, y))
        c = Color{120, 120, 120, 255};
      DrawRectangle(offX + x * mmS, offY + y * mmS, mmS, mmS, c);
    }
  DrawRectangle(offX + game.getPlayerX() * mmS, offY + game.getPlayerY() * mmS,
                mmS, mmS, YELLOW);
}

void HUD::drawVictory(const Game &game) const {
  DrawCenteredOverlay(game, "VICTORIA", LIME, "Pulsa R para comenzar de nuevo");
}

void HUD::drawGameOver(const Game &game) const {
  DrawCenteredOverlay(game, "GAME OVER", RED, "Pulsa R para comenzar de nuevo");
}
