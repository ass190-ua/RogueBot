#include "Game.hpp"
#include <algorithm> // std::min, std::clamp
#include <cmath>     // std::floor

int Game::getFovRadius() const {
    int r = fovTiles;
    if (glassesTimer > 0.0f) {
        r += glassesFovMod;
    }
    return std::clamp(r, 2, 30);
}

int Game::defaultFovFromViewport() const {
    int tilesX = screenW / tileSize;
    int tilesY = screenH / tileSize;
    int r = static_cast<int>(std::floor(std::min(tilesX, tilesY) * 0.15f));
    return std::clamp(r, 3, 20);
}

void Game::clampCameraToMap() {
    const float worldW = map.width() * (float)tileSize;
    const float worldH = map.height() * (float)tileSize;
    const float viewW = screenW / camera.zoom;
    const float viewH = screenH / camera.zoom;
    const float halfW = viewW * 0.5f;
    const float halfH = viewH * 0.5f;

    if (worldW <= viewW) camera.target.x = worldW * 0.5f;
    else camera.target.x = std::clamp(camera.target.x, halfW, worldW - halfW);

    if (worldH <= viewH) camera.target.y = worldH * 0.5f;
    else camera.target.y = std::clamp(camera.target.y, halfH, worldH - halfH);
}

void Game::centerCameraOnPlayer() {
    camera.target = { px * (float)tileSize + tileSize / 2.0f,
                      py * (float)tileSize + tileSize / 2.0f };
    clampCameraToMap();
}

void Game::recomputeFovIfNeeded() {
    if (map.fogEnabled()) {
        map.computeVisibility(px, py, getFovRadius());
    }
}

