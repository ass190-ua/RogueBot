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

    // CAMBIO: Ahora miramos el ÚLTIMO dispositivo usado, no si está conectado
    const char* promptText = (lastInput == InputDevice::Gamepad) ? "(A)" : "E";

    for (const auto &it : items) {
        if (!isVisible(it.tile.x, it.tile.y)) continue;
        drawItemSprite(it);
        
        // Dibujar el indicador si el jugador está encima y NO es la llave
        if (it.tile.x == px && it.tile.y == py && it.type != ItemType::LlaveMaestra) {
            int txtW = MeasureText(promptText, 10);
            int txtX = it.tile.x * tileSize + (tileSize - txtW) / 2;
            int txtY = it.tile.y * tileSize - 12;

            DrawText(promptText, txtX + 1, txtY + 1, 10, BLACK); 
            DrawText(promptText, txtX, txtY, 10, YELLOW);
        }
    }
}

void Game::tryAutoPickup() {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].tile.x == px && items[i].tile.y == py) {
            if (items[i].type == ItemType::LlaveMaestra) {
                onPickup(items[i]);
                items.erase(items.begin() + i);
                return; // Solo una por frame
            }
        }
    }
}

void Game::tryManualPickup() {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].tile.x == px && items[i].tile.y == py) {
            // La llave se ignora aquí porque es automática
            if (items[i].type != ItemType::LlaveMaestra) {
                onPickup(items[i]);
                items.erase(items.begin() + i);
                return;
            }
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
            shieldTimer = 60.0f; // Reinicia a 60s siempre
            std::cout << "[Pickup] Escudo activado (60s).\n";
            break;

        case ItemType::BateriaVidaExtra:
            hasBattery = true;
            std::cout << "[Pickup] Bateria vida extra guardada.\n";
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
            if (hp < hpMax) {
                hp = std::min(hpMax, hp + 2); // +2 = Recupera 1 Corazón entero
                std::cout << "[Pickup] Pila Buena (+1 Corazón).\n";
            } else {
                std::cout << "[Pickup] Vida llena.\n";
            }
            break;
            
        case ItemType::PilaMala:
            hp = std::max(0, hp - 2); // -2 = Quita 1 Corazón entero
            std::cout << "[Pickup] Pila Mala (-1 Corazón).\n";
            break;

        case ItemType::Gafas3DBuenas:
            glassesTimer = 20.0f;
            glassesFovMod = 5; // Aumenta FOV considerablemente
            recomputeFovIfNeeded();
            std::cout << "[Pickup] Gafas 3D Buenas (+FOV 20s).\n";
            break;

        case ItemType::Gafas3DMalas:
            glassesTimer = 20.0f;
            glassesFovMod = -4; // Reduce FOV (miope)
            recomputeFovIfNeeded();
            std::cout << "[Pickup] Gafas 3D Malas (-FOV 20s).\n";
            break;
    }
}
