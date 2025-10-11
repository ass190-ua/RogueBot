#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>

constexpr int TILE = 32; // <-- cámbialo si usáis otro tamaño

struct IVec2 { int x, y; };
inline IVec2 toTile(const Vector2& p) {
    return { (int)std::floor(p.x / TILE), (int)std::floor(p.y / TILE) };
}

struct Weapon {
    int   damage      = 1;     // daño por golpe
    int   rangeTiles  = 1;     // 1 o 2 casillas
    float cooldown    = 0.20f; // segundos entre golpes
    float swingTime   = 0.10f; // duración del gesto
};

struct AttackSystem {
    Weapon current{};
    float  cdTimer    = 0.f;
    float  swingTimer = 0.f;
    bool   swinging   = false;

    // Para "frente" usamos la última dirección del jugador
    Vector2 lastDir   = {1,0};

    enum Mode { Cross, Front } mode = Front; // Cross: 4 adyacentes; Front: solo delante
};

// --- Helpers ---
inline IVec2 facingFromDir(Vector2 d) {
    // toma el eje dominante para elegir N/S/E/O
    if (std::fabs(d.x) >= std::fabs(d.y)) return { (d.x >= 0) ? 1 : -1, 0 };
    else                                   return { 0, (d.y >= 0) ? 1 : -1 };
}

template <class EnemyT>
int PerformMeleeHit(AttackSystem& a, const Vector2& playerPos, std::vector<EnemyT>& enemies)
{
    const IVec2 pt = toTile(playerPos);
    int hits = 0;

    // tiles objetivo según modo
    std::vector<IVec2> targets;
    if (a.mode == AttackSystem::Cross) {
        for (int t=1; t<=a.current.rangeTiles; ++t) {
            targets.push_back({pt.x + t, pt.y});
            targets.push_back({pt.x - t, pt.y});
            targets.push_back({pt.x, pt.y + t});
            targets.push_back({pt.x, pt.y - t});
        }
    } else { // Front
        IVec2 f = facingFromDir(a.lastDir);
        for (int t=1; t<=a.current.rangeTiles; ++t)
            targets.push_back({pt.x + f.x*t, pt.y + f.y*t});
    }

    auto isTarget = [&](IVec2 et){
        for (auto tt: targets) if (tt.x==et.x && tt.y==et.y) return true;
        return false;
    };

    for (auto& e : enemies) {
        if (!e.alive) continue;
        IVec2 et = toTile(e.pos);
        if (isTarget(et)) {
            e.hp -= a.current.damage;
            if (e.hp <= 0) e.alive = false;
            ++hits;
        }
    }
    return hits;
}

// Llamar cada frame
template <class EnemyT>
void UpdateAttack(AttackSystem& a, float dt, const Vector2& playerPos, const Vector2& playerVel, std::vector<EnemyT>& enemies)
{
    a.cdTimer    = std::max(0.f, a.cdTimer - dt);
    if (playerVel.x!=0 || playerVel.y!=0) a.lastDir = playerVel; // actualizar "frente"

    // input: SPACE o click izq
    bool pressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    // iniciar gesto aunque no haya enemigos
    if (pressed && a.cdTimer<=0.f) {
        a.swinging   = true;
        a.swingTimer = a.current.swingTime;
        a.cdTimer    = a.current.cooldown;

        // aplicar daño inmediatamente al empezar el gesto
        PerformMeleeHit(a, playerPos, enemies);
    }

    if (a.swinging) {
        a.swingTimer -= dt;
        if (a.swingTimer <= 0.f) a.swinging = false;
    }
}
