#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Map.hpp"
#include "raylib.h"

// Enemigo en coordenadas de TILE (x,y)
class Enemy {
public:
    Enemy(int gx = 0, int gy = 0) : x(gx), y(gy) {}

    // Posición en tiles
    void setPos(int gx, int gy) { x = gx; y = gy; }
    int  getX() const { return x; }
    int  getY() const { return y; }

    // Detección por radio en píxeles (no guarda estado; se decidirá fuera)
    bool inChaseRange(int px, int py, int tileSize, int radiusPx) const;

    // Un paso de persecución (greedy por rejilla, sin diagonales)
    void stepChase(int px, int py, const Map& map);

    // Colisión exacta con jugador (misma celda)
    bool collidesWith(int px, int py) const { return x == px && y == py; }

    // Dibujado simple (placeholder): un tile rojo
    void draw(int tileSize) const {
        DrawRectangle(x*tileSize, y*tileSize, tileSize, tileSize, RED);
    }

private:
    int x{0}, y{0};
};

#endif
