#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "raylib.h"
#include <string>

// Enum fuertemente tipado para las 4 direcciones cardinales.
// Útil para hacer el código más legible que usar 'int 0, 1, 2, 3'.
enum class Direction { Down = 0, Up = 1, Left = 2, Right = 3 };

class Player {
public:
    // Gestión de recursos (Sprites)
    // Carga las texturas desde disco.
    // Se espera que 'baseDir' contenga imágenes como "robot_down_idle.png", etc.
    void load(const std::string& baseDir = "assets/sprites/player");
    
    // Libera la memoria de vídeo (VRAM) de las texturas cargadas.
    void unload();

    // Lógica de actualización y animación
    // Recibe el cambio de posición (dx, dy) y actualiza la dirección 'dir'.
    // Ej: si dx=1, mira a la Derecha. Si dy=-1, mira Arriba.
    void setDirectionFromDelta(int dx, int dy);

    // Avanza el estado de la animación basado en el tiempo delta (dt).
    // - isMoving = false: Fuerza el frame 0 (Idle/Parado).
    // - isMoving = true:  Alterna entre frames 1 y 2 (Caminar).
    void update(float dt, bool isMoving);

    // Dibuja el sprite correspondiente en pantalla.
    // Recibe (px, py) que son coordenadas de REJILLA, y las multiplica por 'tileSize'.
    void draw(int tileSize, int px, int py) const;

    // Configuración de velocidad: define cada cuánto tiempo cambia el frame de caminar.
    // Default 0.12s = ~8 frames por segundo (animación fluida tipo RPG clásico).
    void setAnimInterval(float s) { animInterval = s; }

    // Posición en el mundo (Grid)
    // Teletransporta al jugador a una casilla específica (x, y).
    void setGridPos(int x, int y);

    // Getters de la posición lógica actual.
    int  getX() const;
    int  getY() const;

private:
    // Datos internos
    // Matriz de Sprites: [Dirección 0..3][Frame 0..2]
    // Estructura:
    //   [dir][0] = IDLE (Quieto)
    //   [dir][1] = WALK 1 (Pie izquierdo)
    //   [dir][2] = WALK 2 (Pie derecho)
    Texture2D tex[4][3] = {}; 
    bool loaded = false; // Bandera de seguridad para no dibujar basura si no se cargó

    // Posición Lógica (Grid Coordinates)
    int gx = 0, gy = 0;

    // Máquina de Estados de Animación
    Direction dir = Direction::Down; // Hacia dónde mira actualmente
    float animTimer = 0.0f;          // Acumulador de tiempo para el siguiente frame
    float animInterval = 0.12f;      // Frecuencia de cambio de sprite
    int walkIndex = 0;               // Alternador (0 ó 1) para saber qué pie mover

    // Helper estático: Convierte el Enum Direction a un entero (0-3) para usar como índice de array.
    static int dirIndex(Direction d) {
        switch (d) {
            case Direction::Down:  return 0;
            case Direction::Up:    return 1;
            case Direction::Left:  return 2;
            case Direction::Right: return 3;
        }
        return 0; // Fallback seguro
    }

    // Variables de Animación Procedural ("Juice")
    float animTime = 0.0f;
    float tiltAngle = 0.0f;
    float targetTilt = 0.0f;
};

#endif
