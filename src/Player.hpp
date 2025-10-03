#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "raylib.h"
#include <string>

enum class Direction { Down = 0, Up = 1, Left = 2, Right = 3 };

class Player {
public:
    void load(const std::string& baseDir = "assets/sprites/player");
    void unload();

    // Notificar estado de movimiento/dirección y avanzar animación
    void setDirectionFromDelta(int dx, int dy);
    void update(float dt, bool isMoving);

    // Dibuja el sprite en la celda (px,py) del grid
    void draw(int tileSize, int px, int py) const;

    // Opcional: ajustar velocidad de animación (por defecto 0.12s entre frames)
    void setAnimInterval(float s) { animInterval = s; }

    void setGridPos(int x, int y);

    int  getX() const;
    int  getY() const;

private:
    // tex[dir][frame] con frame: 0=idle, 1=walk1, 2=walk2
    Texture2D tex[4][3] = {};
    bool loaded = false;

    int gx = 0, gy = 0;

    Direction dir = Direction::Down;
    float animTimer = 0.0f;
    float animInterval = 0.12f;   // 120ms por frame (combina bien con tu cooldown)
    int walkIndex = 0;            // 0/1 para alternar walk1/walk2 cuando te mueves

    static int dirIndex(Direction d) {
        switch (d) {
            case Direction::Down:  return 0;
            case Direction::Up:    return 1;
            case Direction::Left:  return 2;
            case Direction::Right: return 3;
        }
        return 0;
    }
};

#endif
