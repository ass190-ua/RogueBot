#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Map.hpp"
#include "raylib.h"

// Clase Enemigo (Sistema de Rejilla)
// Esta clase representa a una entidad hostil en el juego.
// Clave de diseño: Almacena su posición en "Tiles" (x,y enteros), no en píxeles
// de pantalla.
class Enemy {
public:
  // Tipo de Enemigo
  enum Type { Melee, Shooter };

  // Constructor: Inicializa al enemigo en una celda específica.
  Enemy(int gx = 0, int gy = 0, Type t = Melee) : x(gx), y(gy), type(t) {
    lastX = gx;
    lastY = gy;
  }

  // Acceso a posición
  // Setters y Getters simples para mover al enemigo o leer dónde está.
  void setPos(int gx, int gy) {
    x = gx;
    y = gy;
    lastX = gx;
    lastY = gy;
    walkTimer = 0.0f;
    walkAnimTimer = 0.0f;
    walkIndex = 0;
  }

  int getX() const { return x; }
  int getY() const { return y; }

  // Getter del tipo
  Type getType() const { return type; }

  // Lógica de comportamiento (IA)
  // Detección: ¿Está el jugador dentro del "círculo de agresión"?
  // Se calcula usando distancia euclídea convertida a píxeles.
  bool inChaseRange(int px, int py, int tileSize, int radiusPx) const;

  // Movimiento: Ejecuta UN paso de la persecución.
  // La lógica interna (en el .cpp) usa un algoritmo "Greedy" (Voraz)
  // que intenta reducir la distancia en el eje más lejano primero.
  void stepChase(int px, int py, const Map &map);

  // Físicas y renderizado

  // Colisión exacta (Tile-perfect):
  // Como usamos enteros, la colisión es trivial: ¿Coinciden las coordenadas?
  bool collidesWith(int px, int py) const { return x == px && y == py; }

  // Renderizado Básico (Placeholder / Fallback):
  // Si no cargamos sprites, dibujamos un cuadrado rojo.
  // Recibe 'tileSize' para saber a qué píxel de pantalla corresponde la celda
  // (x,y).
  void draw(int tileSize) const {
    Color c = (type == Shooter) ? ORANGE : RED;
    DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, c);
  }

  // Método para actualizar la animación (lo llamaremos en Update)
  void updateAnimation(float dt) {
    animTime += dt * 5.0f; // Velocidad de respiración

    // Interpolación suave (Lerp) para la inclinación
    // Hacemos que el tilt vuelva a 0.0 poco a poco si no se mueve
    tiltAngle += (targetTilt - tiltAngle) * 10.0f * dt;

    // Decaimiento del target (el impulso de inclinación dura poco)
    targetTilt += (0.0f - targetTilt) * 5.0f * dt;

    const bool movedNow = (x != lastX || y != lastY);
    if (movedNow) {
      walkTimer = 0.25f;
      lastX = x;
      lastY = y;
    }

    if (walkTimer > 0.0f) {
      walkTimer -= dt;
      if (walkTimer < 0.0f)
        walkTimer = 0.0f;
    }

    if (isMoving()) {
      walkAnimTimer += dt;
      if (walkAnimTimer >= walkAnimInterval) {
        walkAnimTimer = 0.0f;
        walkIndex = 1 - walkIndex;
      }
    } else {
      walkAnimTimer = 0.0f;
      walkIndex = 0;
    }
  }

  // Setter para dar un "impulso" de rotación al moverse
  void addTilt(float angle) { targetTilt = angle; }

  // Getters para el dibujado
  float getAnimTime() const { return animTime; }
  float getTilt() const { return tiltAngle; }

  void notifyMoved() { walkTimer = 0.25f; }
  bool isMoving() const { return walkTimer > 0.0f; }

  int getWalkIndex() const { return walkIndex; } // 0 ó 1

private:
  // Variables para animación
  float animTime = 0.0f;   // Tiempo acumulado (para el seno)
  float tiltAngle = 0.0f;  // Ángulo de inclinación actual
  float targetTilt = 0.0f; // Ángulo objetivo (hacia donde nos movemos)

  int lastX = 0, lastY = 0;
  float walkTimer = 0.0f;
  float walkAnimTimer = 0.0f;     // acumulador (como Player)
  float walkAnimInterval = 0.12f; // igual que Player (~8 fps)
  int walkIndex = 0;

  // Estado interno: Coordenadas de la rejilla (Grid Coordinates)
  // x = Columna, y = Fila
  int x{0}, y{0};
  Type type = Melee; // Por defecto melee
};

#endif
