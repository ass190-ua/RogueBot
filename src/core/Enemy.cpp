#include "Enemy.hpp"
#include <cmath>

// Función auxiliar "Signum" (Signo).
// Devuelve:
//  1 si v > 0
// -1 si v < 0
//  0 si v == 0
// Fundamental para convertir una distancia (ej: -5 tiles) en un paso de movimiento (-1 tile).
static inline int sgn(int v){ return (v>0) - (v<0); }

// Comprueba si el jugador está dentro del radio de visión (en píxeles)
bool Enemy::inChaseRange(int px, int py, int tileSize, int radiusPx) const {
    // Diferencia en coordenadas de rejilla (Grid coordinates)
    int dx = px - x;
    int dy = py - y;
    
    // Teorema de Pitágoras para distancia real (Euclídea).
    // Multiplicamos por tileSize para convertir la distancia de "baldosas" a "píxeles".
    float distPx = std::sqrt(float(dx*dx + dy*dy)) * float(tileSize);
    
    return distPx <= float(radiusPx);
}

// Lógica de movimiento paso a paso
void Enemy::stepChase(int px, int py, const Map& map) {
    int dx = px - x;
    int dy = py - y;
    
    // Si ya estamos en la misma casilla, no hacer nada (evita división por cero o lógica extra)
    if (dx == 0 && dy == 0) return;

    // Lambda local: Intenta mover al enemigo a (x+mx, y+my)
    // Retorna 'true' si el movimiento fue exitoso (no había pared).
    auto tryMove = [&](int mx, int my)->bool {
        int nx = x + mx, ny = y + my;
        
        // Verificación de límites del mapa y colisiones (Muros)
        if (nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height()
            && map.isWalkable(nx, ny)) {
            // Actualizamos la posición del enemigo
            x = nx; y = ny;
            return true;
        }
        return false;
    };

    // Algoritmo Greedy con deslizamiento (Sliding)
    // Priorizamos movernos en el eje donde la distancia es mayor.
    // Esto hace que el movimiento se vea más natural y menos "en escalera".
    
    if (std::abs(dx) >= std::abs(dy)) {
        // 1. Intenta mover en horizontal (Eje dominante)
        // Usamos 'sgn(dx)' para saber si ir a izquierda (-1) o derecha (1).
        if (!tryMove(sgn(dx), 0)) {
            // 2. Si hay pared, intenta mover en vertical (Eje secundario)
            tryMove(0, sgn(dy));
        }
    } else {
        // 1. Intenta mover en vertical (Eje dominante)
        if (!tryMove(0, sgn(dy))) {
            // 2. Si hay pared, intenta mover en horizontal
            tryMove(sgn(dx), 0);
        }
    }
}
