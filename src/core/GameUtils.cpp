#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include "GameUtils.hpp"

// Definición del estado global del ataque
AttackRuntime gAttack{};

// Utilidades
IVec2 dominantAxis(IVec2 d) {
    if (std::abs(d.x) >= std::abs(d.y))
        return {(d.x >= 0) ? 1 : -1, 0};
    else
        return {0, (d.y >= 0) ? 1 : -1};
}

std::vector<IVec2> computeMeleeTiles(IVec2 center, IVec2 lastDir, int range, bool frontOnly) {
    std::vector<IVec2> out;
    out.reserve(frontOnly ? range : range * 4);
    if (frontOnly) {
        IVec2 f = dominantAxis((lastDir.x == 0 && lastDir.y == 0) ? IVec2{0, 1} : lastDir);
        for (int t = 1; t <= range; ++t)
            out.push_back({center.x + f.x * t, center.y + f.y * t});
    }
    else {
        for (int t = 1; t <= range; ++t) {
            out.push_back({center.x + t, center.y});
            out.push_back({center.x - t, center.y});
            out.push_back({center.x, center.y + t});
            out.push_back({center.x, center.y - t});
        }
    }
    return out;
}

std::vector<IVec2> computeMeleeTilesOccluded(IVec2 center, IVec2 lastDir, int range, bool frontOnly, const Map& map) {
    auto walkable = [&](int x, int y) -> bool {
        return x >= 0 && y >= 0 && x < map.width() && y < map.height() &&
               map.isWalkable(x, y);
    };

    std::vector<IVec2> out;
    out.reserve(range * (frontOnly ? 1 : 4));

    auto pushRay = [&](IVec2 dir) {
        for (int t = 1; t <= range; ++t) {
            int tx = center.x + dir.x * t;
            int ty = center.y + dir.y * t;
            if (!walkable(tx, ty))
                break; // pared/obstáculo: cortamos y NO añadimos
            out.push_back({tx, ty});
        }
    };

    if (frontOnly) {
        IVec2 f = dominantAxis((lastDir.x == 0 && lastDir.y == 0) ? IVec2{0, 1} : lastDir);
        pushRay(f);
    }
    else {
        pushRay({1, 0});
        pushRay({-1, 0});
        pushRay({0, 1});
        pushRay({0, -1});
    }
    return out;
}

bool isAdjacent4(int ax, int ay, int bx, int by) {
    return (std::abs(ax - bx) + std::abs(ay - by)) == 1;
}

IVec2 facingToDir(Game::EnemyFacing f) {
    switch (f) {
        case Game::EnemyFacing::Up:    return { 0, -1};
        case Game::EnemyFacing::Down:  return { 0,  1};
        case Game::EnemyFacing::Left:  return {-1,  0};
        case Game::EnemyFacing::Right: return { 1,  0};
    }
    return {0, 1};
}
