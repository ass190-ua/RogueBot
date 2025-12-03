#include "Game.hpp"
#include "GameUtils.hpp"
#include "raylib.h"
#include <algorithm>
#include <iostream>

// Renderizado de Ítems
void Game::drawItems() const {
    // Lambda para Niebla de Guerra (FOV, "Fog of War")
    // Evita que el jugador vea ítems a través de paredes o zonas no exploradas
    auto isVisible = [&](int x, int y) {
        if (map.fogEnabled()) return map.isVisible(x, y);
        return true;
    };

    // UI adaptativa (input contextual)
    // Detectamos qué dispositivo usó el jugador por última vez para mostrar
    // el prompt correcto: "(A)" para mando o "E" para Teclado.
    const char* promptText = (lastInput == InputDevice::Gamepad) ? "(A)" : "E";

    for (const auto &it : items) {
        // 1. Si está en la niebla, no se dibuja (anti-cheat visual)
        if (!isVisible(it.tile.x, it.tile.y)) continue;
        
        // 2. Dibujar el sprite del objeto en el suelo
        drawItemSprite(it);
        
        // 3. UI flotante (Solo si el jugador está encima)
        // La Llave Maestra no necesita texto porque se recoge sola (Auto-Pickup).
        if (it.tile.x == px && it.tile.y == py && it.type != ItemType::LlaveMaestra) {
            // Centrado del texto sobre el tile
            int txtW = MeasureText(promptText, 10);
            int txtX = it.tile.x * tileSize + (tileSize - txtW) / 2;
            int txtY = it.tile.y * tileSize - 12; // Flotando un poco arriba

            // Renderizado con sombra simple (negro abajo derecha) para legibilidad
            DrawText(promptText, txtX + 1, txtY + 1, 10, BLACK); 
            DrawText(promptText, txtX, txtY, 10, YELLOW);
        }
    }
}

// Lógica de recogida automática (Ato-Pickup)
// Se llama en cada frame. Solo aplica a objetos críticos para el flujo (Llaves)
// para no interrumpir el movimiento del jugador.
void Game::tryAutoPickup() {
    for (size_t i = 0; i < items.size(); ++i) {
        // Colisión simple basada en Coordenadas de Rejilla (Grid-based collision)
        if (items[i].tile.x == px && items[i].tile.y == py) {
            
            if (items[i].type == ItemType::LlaveMaestra) {
                onPickup(items[i]);             // Aplicar efecto
                items.erase(items.begin() + i); // Eliminar del mundo
                return; // Importante: Salimos para evitar errores de iterador al borrar
            }
        }
    }
}

// Lógica de recogida manual (Interacción)
// Se llama solo cuando el jugador pulsa 'E' o 'A'.
// Aplica a consumibles y armas (donde el jugador decide si los quiere o no).
void Game::tryManualPickup() {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].tile.x == px && items[i].tile.y == py) {
            
            // Ignoramos la llave aquí porque ya la gestionó el AutoPickup
            if (items[i].type != ItemType::LlaveMaestra) {
                onPickup(items[i]);
                items.erase(items.begin() + i);
                return; // Solo recogemos 1 objeto por pulsación
            }
        }
    }
}

// Selección de sprites (Visuales)
void Game::drawItemSprite(const ItemSpawn &it) const {
    const Texture2D *tex = nullptr;

    switch (it.type) {
        case ItemType::LlaveMaestra:        tex = &itemSprites.keycard; break;
        case ItemType::Escudo:              tex = &itemSprites.shield;  break;
        
        // Mismo sprite para pilas buenas/malas (Factor riesgo/sorpresa)
        case ItemType::PilaBuena:
        case ItemType::PilaMala:            tex = &itemSprites.pila;    break;
        
        case ItemType::Gafas3DBuenas:
        case ItemType::Gafas3DMalas:        tex = &itemSprites.glasses; break;
        case ItemType::BateriaVidaExtra:    tex = &itemSprites.battery; break;

        // Visualización de Tiers de Armas
        case ItemType::EspadaPickup: {
            // Lógica visual inteligente:
            // Muestra el sprite del nivel que OBTENDRÁS, no necesariamente el que es el ítem.
            // Ej: Si tengo nivel 0 y el item es nivel 3, veo una espada de nivel 1 (azul).
            // Esto comunica "Este objeto te subirá al siguiente nivel".
            int displayTier = std::min(it.tierSugerido, runCtx.espadaMejorasObtenidas + 1);
            displayTier = std::clamp(displayTier, 1, 3); // Asegurar rango válido de array/texturas
            
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

    // Coordenadas de pantalla
    const int pxl = it.tile.x * tileSize;
    const int pyl = it.tile.y * tileSize;

    if (tex && tex->id != 0) {
        // Dibujado seguro con textura
        Rectangle src{0, 0, (float)tex->width, (float)tex->height};
        Rectangle dst{(float)pxl, (float)pyl, (float)tileSize, (float)tileSize};
        Vector2 origin{0, 0};
        DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
    }
    else {
        // Fallback: Cuadrado blanco si falta la textura (Debug visual)
        DrawRectangle(pxl, pyl, tileSize, tileSize, WHITE);
        DrawRectangleLines(pxl, pyl, tileSize, tileSize, BLACK);
    }
}

// Efectos de los objetos (Gameplay)
void Game::onPickup(const ItemSpawn &it) {
    bool isPowerUp = false; // Flag para decidir qué sonido reproducir (Épico vs Normal)

    switch (it.type) {
        case ItemType::LlaveMaestra:
            hasKey = true;
            isPowerUp = true; // Objeto de misión importante
            std::cout << "[Pickup] Llave maestra obtenida.\n";
            break;

        case ItemType::Escudo:
            hasShield = true;
            shieldTimer = 60.0f; // 1 minuto de protección
            std::cout << "[Pickup] Escudo activado (60s).\n";
            break;

        case ItemType::BateriaVidaExtra:
            hasBattery = true; // Vida extra en caso de morir, restaura la mitad de la vida
            isPowerUp = true;
            std::cout << "[Pickup] Bateria vida extra guardada.\n";
            break;

        // Progresión lineal de armas
        // No permitimos saltar tiers. Si tienes Espada Nivel 1 y recoges una Nivel 3,
        // el código `min(tierSugerido, swordTier + 1)` te sube a Nivel 2.
        // Esto mantiene la curva de dificultad controlada.
        case ItemType::EspadaPickup: {
            int real = std::min(it.tierSugerido, swordTier + 1);
            if (real > swordTier) swordTier = real;
            runCtx.espadaMejorasObtenidas = swordTier; // Persistencia entre niveles
            isPowerUp = true; 
            std::cout << "[Pickup] Espada nivel " << swordTier << ".\n";
            break;
        }

        case ItemType::PistolaPlasmaPickup: {
            int real = std::min(it.tierSugerido, plasmaTier + 1);
            if (real > plasmaTier) plasmaTier = real;
            runCtx.plasmaMejorasObtenidas = plasmaTier;
            isPowerUp = true;
            std::cout << "[Pickup] Plasma nivel " << plasmaTier << ".\n";
            break;
        }

        case ItemType::PilaBuena:
            if (hp < hpMax) {
                hp = std::min(hpMax, hp + 2); // +2 HP = 1 Corazón completo
                std::cout << "[Pickup] Pila Buena (+1 Corazón).\n";
            } else {
                std::cout << "[Pickup] Vida llena.\n";
            }
            break;
            
        case ItemType::PilaMala:
            // "Item Trampa": Resta vida. Por eso requiere pickup manual (pulsar E),
            // para que el jugador tenga que prestar atención y no solo pase por encima.
            hp = std::max(0, hp - 2); 
            std::cout << "[Pickup] Pila Mala (-1 Corazón).\n";
            break;

        case ItemType::Gafas3DBuenas:
            glassesTimer = 20.0f;
            glassesFovMod = 5; // Aumenta el campo de visión (reduce la niebla)
            recomputeFovIfNeeded();
            std::cout << "[Pickup] Gafas 3D Buenas (+FOV 20s).\n";
            break;

        case ItemType::Gafas3DMalas:
            glassesTimer = 20.0f;
            glassesFovMod = -4; // Cierra el campo de visión (ceguera temporal)
            recomputeFovIfNeeded();
            std::cout << "[Pickup] Gafas 3D Malas (-FOV 20s).\n";
            break;
    }

    // Feedback sonoro
    if (isPowerUp) {
        PlaySound(sfxPowerUp); // Sonido "Tadaaa!"
    } else {
        PlaySound(sfxPickup);  // Sonido "Bip" simple
    }
}
