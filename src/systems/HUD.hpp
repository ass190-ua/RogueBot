#ifndef HUD_HPP
#define HUD_HPP

#include <raylib.h>
#include <vector>

// Declaración anticipada (Forward Declaration)
// Declaramos que existe una clase 'Game', pero no incluimos "Game.hpp" aquí.
// Esto rompe la dependencia circular, ya que 'Game' incluye 'HUD'.
class Game;

class HUD {
public:
    // Métodos de dibujado.
    // Reciben una referencia constante a Game porque el HUD solo lee el estado,
    // no modifica la lógica del juego.
    
    // Dibuja la interfaz principal: Barra de vida, minimapa, cooldowns, texto de ayuda.
    void drawPlaying(const Game& game) const;   

    // Dibuja la superposición de pantalla de victoria.
    void drawVictory(const Game& game) const;   

    // Dibuja la superposición de pantalla de derrota.
    void drawGameOver(const Game& game) const;  

private:
    // Estado interno de UI (animaciones)
    // 'mutable': Permite modificar esta variable incluso dentro de métodos 'const'.
    // Necesario porque 'drawPlaying' es const (no cambia el juego), pero el HUD
    // necesita recordar cuánta vida había en el frame anterior para detectar
    // si debe disparar una animación de daño o curación.
    mutable int prevHp = -1;

    // Estructura interna para gestionar efectos visuales temporales en la barra de vida.
    struct HpFx {
        enum class Type { Lose, Gain }; // Tipo: Perder corazón (explosión) o ganar (crecer)
        Type  type;
        int   index; // Índice del corazón afectado (0, 1, 2...)
        float t;     // Tiempo normalizado de la animación (0.0 a 1.0)
    };

    // Lista de efectos activos actualmente. También es 'mutable' para poder
    // añadir/eliminar efectos dentro del bucle de renderizado 'const'.
    mutable std::vector<HpFx> hpFx;

    // Dibuja una pequeña explosión de partículas (usada cuando se rompe un corazón).
    static void DrawBurst(Vector2 center, float t);

    // Función matemática de "Easing" (Suavizado).
    // Convierte un movimiento lineal en uno más natural (rápido al inicio, lento al final).
    // Input x: 0.0 a 1.0. Output: Valor suavizado.
    static float EaseOutCubic(float x) { 
        float a = 1.0f - x; 
        return 1.0f - a * a * a; 
    }
};

#endif
