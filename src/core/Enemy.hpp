#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Map.hpp"
#include "raylib.h"

// Clase Enemigo (Sistema de Rejilla)
// Esta clase representa a una entidad hostil en el juego.
// Clave de diseño: Almacena su posición en "Tiles" (x,y enteros), no en píxeles de pantalla.
class Enemy {
public:
    // Constructor: Inicializa al enemigo en una celda específica.
    Enemy(int gx = 0, int gy = 0) : x(gx), y(gy) {}

    // Acceso a posición
    // Setters y Getters simples para mover al enemigo o leer dónde está.
    void setPos(int gx, int gy) { x = gx; y = gy; }
    int  getX() const { return x; }
    int  getY() const { return y; }

    // Lógica de comportamiento (IA)
    
    // Detección: ¿Está el jugador dentro del "círculo de agresión"?
    // Se calcula usando distancia euclídea convertida a píxeles.
    bool inChaseRange(int px, int py, int tileSize, int radiusPx) const;

    // Movimiento: Ejecuta UN paso de la persecución.
    // La lógica interna (en el .cpp) usa un algoritmo "Greedy" (Voraz)
    // que intenta reducir la distancia en el eje más lejano primero.
    void stepChase(int px, int py, const Map& map);

    // Físicas y renderizado

    // Colisión exacta (Tile-perfect):
    // Como usamos enteros, la colisión es trivial: ¿Coinciden las coordenadas?
    bool collidesWith(int px, int py) const { return x == px && y == py; }

    // Renderizado Básico (Placeholder / Fallback):
    // Si no cargamos sprites, dibujamos un cuadrado rojo.
    // Recibe 'tileSize' para saber a qué píxel de pantalla corresponde la celda (x,y).
    void draw(int tileSize) const {
        DrawRectangle(x*tileSize, y*tileSize, tileSize, tileSize, RED);
    }

private:
    // Estado interno: Coordenadas de la rejilla (Grid Coordinates)
    // x = Columna, y = Fila
    int x{0}, y{0};
};

#endif
