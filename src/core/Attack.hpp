#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

// Tamaño de la celda. Importante para convertir posiciones de mundo (píxeles) a rejilla lógica.
constexpr int TILE = 32; 

struct IVec2 { int x, y; };

// Convierte coordenada de píxeles (float) a coordenada de mapa (int)
inline IVec2 toTile(const Vector2& p) {
    return { (int)std::floor(p.x / TILE), (int)std::floor(p.y / TILE) };
}

// Definición de armas
struct Weapon {
    int   damage      = 1;     // Cuánta vida quita
    int   rangeTiles  = 1;     // Alcance en casillas (1 = melee corto, 2 = lanza/látigo)
    float cooldown    = 0.20f; // Tiempo de espera entre ataques (frenético vs pesado)
    float swingTime   = 0.10f; // Tiempo visual que dura el "gesto" del ataque
};

// Estado del sistema de ataque
struct AttackSystem {
    Weapon current{};          // Arma equipada actualmente
    float  cdTimer    = 0.f;   // Temporizador para poder volver a atacar
    float  swingTimer = 0.f;   // Temporizador de la animación de ataque
    bool   swinging   = false; // ¿Está atacando ahora mismo?

    // Memoria de dirección: Si el jugador se para (vel 0,0), recordamos dónde miraba.
    Vector2 lastDir   = {1,0};

    // Modos de ataque:
    // Cross: Ataca en cruz alrededor del jugador (fácil, estilo Bomberman/Zelda clásico).
    // Front: Ataca solo hacia donde miras (requiere apuntar, más técnico).
    enum Mode { Cross, Front } mode = Front; 
};

// Determina la dirección cardinal (N, S, E, O) basada en un vector de movimiento libre.
// Usa el "Eje Dominante": si te mueves más en X que en Y, mira horizontalmente.
inline IVec2 facingFromDir(Vector2 d) {
    if (std::fabs(d.x) >= std::fabs(d.y)) 
        return { (d.x >= 0) ? 1 : -1, 0 }; // Derecha o Izquierda
    else                                  
        return { 0, (d.y >= 0) ? 1 : -1 }; // Abajo o Arriba
}

// Lógica de impacto (Hit Detection)
// Usamos template <class EnemyT> para que esta función acepte cualquier clase de enemigo
// siempre que tenga propiedades .pos, .hp y .alive.
template <class EnemyT>
int PerformMeleeHit(AttackSystem& a, const Vector2& playerPos, std::vector<EnemyT>& enemies)
{
    const IVec2 pt = toTile(playerPos); // Posición del jugador en la rejilla
    int hits = 0;

    // 1. Calcular casillas afectadas (Hitbox)
    std::vector<IVec2> targets;
    
    if (a.mode == AttackSystem::Cross) {
        // Modo Cruz: Añade casillas en las 4 direcciones
        for (int t=1; t<=a.current.rangeTiles; ++t) {
            targets.push_back({pt.x + t, pt.y}); // Derecha
            targets.push_back({pt.x - t, pt.y}); // Izquierda
            targets.push_back({pt.x, pt.y + t}); // Abajo
            targets.push_back({pt.x, pt.y - t}); // Arriba
        }
    } else { // Front
        // Modo Frontal: Calcula el vector de dirección y proyecta casillas en esa línea
        IVec2 f = facingFromDir(a.lastDir);
        for (int t=1; t<=a.current.rangeTiles; ++t)
            targets.push_back({pt.x + f.x*t, pt.y + f.y*t});
    }

    // Lambda para verificar si una coordenada está en la lista de objetivos
    auto isTarget = [&](IVec2 et){
        for (auto tt: targets) if (tt.x==et.x && tt.y==et.y) return true;
        return false;
    };

    // 2. Verificar colisiones con enemigos
    for (auto& e : enemies) {
        if (!e.alive) continue;
        
        // Convertimos posición del enemigo a Tile para comparar
        IVec2 et = toTile(e.pos);
        
        if (isTarget(et)) {
            // Impacto confirmado
            e.hp -= a.current.damage;
            if (e.hp <= 0) e.alive = false;
            ++hits;
        }
    }
    return hits; // Devuelve a cuántos enemigos golpeamos (útil para sonidos/puntos)
}

// Bucle de actualización (Update)
template <class EnemyT>
void UpdateAttack(AttackSystem& a, float dt, const Vector2& playerPos, const Vector2& playerVel, std::vector<EnemyT>& enemies)
{
    // 1. Gestión de tiempo (Cooldown)
    a.cdTimer = std::max(0.f, a.cdTimer - dt);
    
    // 2. Actualizar dirección
    // Solo actualizamos si hay movimiento real, para no perder la orientación al soltar las teclas.
    if (playerVel.x!=0 || playerVel.y!=0) a.lastDir = playerVel; 

    // 3. Input
    bool pressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    // 4. Ejecución del ataque
    // Se permite atacar si se pulsa la tecla Y el cooldown ha terminado
    if (pressed && a.cdTimer<=0.f) {
        a.swinging   = true;
        a.swingTimer = a.current.swingTime;  // Iniciar animación visual
        a.cdTimer    = a.current.cooldown;   // Resetear cooldown

        // Aplicamos el daño inmediatamente (estilo arcade rápido)
        // Si se quisiera realismo, se llamaría a esto cuando swingTimer vaya por la mitad.
        PerformMeleeHit(a, playerPos, enemies);
    }

    // 5. Gestión de la animación
    if (a.swinging) {
        a.swingTimer -= dt;
        if (a.swingTimer <= 0.f) a.swinging = false;
    }
}
