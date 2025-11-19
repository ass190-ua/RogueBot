#include "Player.hpp"
#include <stdexcept>
#include <iostream>
#include "AssetPath.hpp"

static Texture2D loadTexPoint(const std::string& rel) {
    const std::string full = assetPath(rel);
    Texture2D t = LoadTexture(full.c_str());
    if (t.id == 0) {
        std::cerr << "[ASSETS] FALLBACK tex point para: " << full << "\n";
        Image white = GenImageColor(32, 32, WHITE);
        t = LoadTextureFromImage(white);
        UnloadImage(white);
    }
    return t;
}

void Player::load(const std::string& baseDir) {
    if (loaded) return;
    // Orden: down, up, left, right
    const char* dirs[4] = {"down","up","left","right"};
    const char* frames[3] = {"idle","walk1","walk2"};

    for (int d = 0; d < 4; ++d) {
        for (int f = 0; f < 3; ++f) {
            std::string path = baseDir + "/robot_" + dirs[d] + std::string("_") + frames[f] + ".png";
            tex[d][f] = loadTexPoint(path);
            if (tex[d][f].id == 0) {
                // No abortamos el juego, pero podrías lanzar excepción si quieres estricto
                // throw std::runtime_error("No se pudo cargar " + path);
            }
        }
    }
    loaded = true;
}

void Player::unload() {
    if (!loaded) return;
    for (int d = 0; d < 4; ++d)
        for (int f = 0; f < 3; ++f)
            if (tex[d][f].id != 0) UnloadTexture(tex[d][f]);
    loaded = false;
}

void Player::setGridPos(int x, int y) { 
    gx = x; gy = y; 
}   

int  Player::getX() const { 
    return gx; 
}

int  Player::getY() const { 
    return gy; 
}

void Player::setDirectionFromDelta(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    if (dx > 0)      dir = Direction::Right;
    else if (dx < 0) dir = Direction::Left;
    else if (dy > 0) dir = Direction::Down;
    else if (dy < 0) dir = Direction::Up;
}

void Player::update(float dt, bool isMoving) {
    if (isMoving) {
        animTimer += dt;
        if (animTimer >= animInterval) {
            animTimer = 0.0f;
            walkIndex = 1 - walkIndex; // alterna 0/1 → walk1/walk2
        }
    } else {
        // Parado → idle
        animTimer = 0.0f;
        walkIndex = 0;
    }
}

void Player::draw(int tileSize, int px, int py) const {
    int d = dirIndex(dir);
    int frame = (walkIndex == 0) ? 1 : 2; // caminando
    int drawFrame = (animTimer == 0.0f && walkIndex == 0) ? 0 : frame;

    const Texture2D& t = tex[d][drawFrame];
    const float sx = (float)(px * tileSize);
    const float sy = (float)(py * tileSize);

    // Ajuste para centrar sprite en el tile (sprites 16x16 → exacto)
    DrawTextureEx(t, {sx, sy}, 0.0f, (float)tileSize / (float)t.width, WHITE);
}
