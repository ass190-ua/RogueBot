#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <iostream>

// ITEMS: Dibujo
void Game::drawItems() const {
    auto isVisible = [&](int x, int y) {
        if (map.fogEnabled()) return map.isVisible(x, y);
        return true;
    };

    for (const auto &it : items) {
        if (!isVisible(it.tile.x, it.tile.y)) continue;
        drawItemSprite(it);
    }
}

void Game::tryPickupHere() {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].tile.x == px && items[i].tile.y == py) {
            onPickup(items[i]);
            items.erase(items.begin() + i);
            break;
        }
    }
}

void Game::drawItemSprite(const ItemSpawn &it) const {
    const Texture2D *tex = nullptr;

    switch (it.type) {
        case ItemType::LlaveMaestra:        tex = &itemSprites.keycard; break;
        case ItemType::Escudo:              tex = &itemSprites.shield;  break;
        case ItemType::PilaBuena:
        case ItemType::PilaMala:            tex = &itemSprites.pila;    break;
        case ItemType::Gafas3DBuenas:
        case ItemType::Gafas3DMalas:        tex = &itemSprites.glasses; break;
        case ItemType::BateriaVidaExtra:    tex = &itemSprites.battery; break;
        case ItemType::EspadaPickup: {
            int displayTier = std::min(it.tierSugerido, runCtx.espadaMejorasObtenidas + 1);
            displayTier = std::clamp(displayTier, 1, 3);
            tex = (displayTier == 1) ? &itemSprites.swordBlue
                 : (displayTier == 2) ? &itemSprites.swordGreen
                                      : &itemSprites.swordRed;
            break;
        }
        case ItemType::PistolaPlasmaPickup: {
            int displayTier = std::min(it.tierSugerido, runCtx.plasmaMejorasObtenidas + 1);
            displayTier = std::clamp(displayTier, 1, 2);
            tex = (displayTier == 1) ? &itemSprites.plasma1 : &itemSprites.plasma2;
            break;
        }
    }

    const int pxl = it.tile.x * tileSize;
    const int pyl = it.tile.y * tileSize;

    if (tex && tex->id != 0) {
        Rectangle src{0, 0, (float)tex->width, (float)tex->height};
        Rectangle dst{(float)pxl, (float)pyl, (float)tileSize, (float)tileSize};
        Vector2 origin{0, 0};
        DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
    }
    else {
        DrawRectangle(pxl, pyl, tileSize, tileSize, WHITE);
        DrawRectangleLines(pxl, pyl, tileSize, tileSize, BLACK);
    }
}

// ITEMS: pickup
void Game::onPickup(const ItemSpawn &it) {
    switch (it.type) {
        case ItemType::LlaveMaestra:
            hasKey = true;
            std::cout << "[Pickup] Llave maestra obtenida.\n";
            break;

        case ItemType::Escudo:
            hasShield = true;
            std::cout << "[Pickup] Escudo preparado.\n";
            break;

        case ItemType::BateriaVidaExtra:
            hasBattery = true;
            std::cout << "[Pickup] Batería extra lista.\n";
            break;

        case ItemType::EspadaPickup: {
            int real = std::min(it.tierSugerido, swordTier + 1);
            if (real > swordTier) swordTier = real;
            runCtx.espadaMejorasObtenidas = swordTier;
            std::cout << "[Pickup] Espada nivel " << swordTier << ".\n";
            break;
        }

        case ItemType::PistolaPlasmaPickup: {
            int real = std::min(it.tierSugerido, plasmaTier + 1);
            if (real > plasmaTier) plasmaTier = real;
            runCtx.plasmaMejorasObtenidas = plasmaTier;
            std::cout << "[Pickup] Plasma nivel " << plasmaTier << ".\n";
            break;
        }

        case ItemType::PilaBuena:
            std::cout << "[Pickup] Pila buena (sin efecto por ahora).\n";
            break;
        case ItemType::PilaMala:
            std::cout << "[Pickup] Pila mala (sin efecto por ahora).\n";
            break;
        case ItemType::Gafas3DBuenas:
            std::cout << "[Pickup] Gafas 3D buenas (20s, sin aplicar aún).\n";
            break;
        case ItemType::Gafas3DMalas:
            std::cout << "[Pickup] Gafas 3D malas (20s, sin aplicar aún).\n";
            break;
    }
}
