#ifndef HUD_HPP
#define HUD_HPP
#include <raylib.h>
#include <vector>

// Evitamos dependencias circulares
class Game;

class HUD {
public:
    void drawPlaying(const Game& game) const;   // HUD durante la partida
    void drawVictory(const Game& game) const;   // Pantalla de victoria
    void drawGameOver(const Game& game) const;  // Pantalla de Game Over

private:
    // Vida anterior vista por el HUD (para detectar cambios)
    mutable int prevHp = -1;

    struct HpFx {
        enum class Type { Lose, Gain };
        Type  type;
        int   index;
        float t;
    };
    mutable std::vector<HpFx> hpFx; // lista de efectos activos

    // FX: explosión al perder vida
    static void DrawBurst(Vector2 center, float t);

    // Curva de aceleración/suavizado para animaciones del HUD
    static float EaseOutCubic(float x) { float a = 1.0f - x; return 1.0f - a*a*a; }
};

#endif
