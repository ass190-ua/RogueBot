#include "Enemy.hpp"
#include <cmath>

static inline int sgn(int v){ return (v>0) - (v<0); }

bool Enemy::inChaseRange(int px, int py, int tileSize, int radiusPx) const {
    int dx = px - x;
    int dy = py - y;
    // Distancia euclídea en PÍXELES
    float distPx = std::sqrt(float(dx*dx + dy*dy)) * float(tileSize);
    return distPx <= float(radiusPx);
}

void Enemy::stepChase(int px, int py, const Map& map) {
    int dx = px - x;
    int dy = py - y;
    if (dx == 0 && dy == 0) return;

    auto tryMove = [&](int mx, int my)->bool {
        int nx = x + mx, ny = y + my;
        // Usa el mapa para bloquear paredes
        if (nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height()
            && map.isWalkable(nx, ny)) {
            x = nx; y = ny;
            return true;
        }
        return false;
    };

    // Greedy: prioriza el eje con mayor diferencia; si bloquea, prueba el otro
    if (std::abs(dx) >= std::abs(dy)) {
        if (!tryMove(sgn(dx), 0)) tryMove(0, sgn(dy));
    } else {
        if (!tryMove(0, sgn(dy))) tryMove(sgn(dx), 0);
    }
}
