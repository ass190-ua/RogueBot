#include "ResourceManager.hpp"
#include "AssetPath.hpp" // Para usar tu función assetPath
#include <iostream>

Texture2D ResourceManager::getTexture(const std::string& relativePath) {
    // 1. CHEQUEO DE CACHÉ
    // Buscamos si esta ruta ya está en nuestro mapa
    auto it = textures.find(relativePath);
    if (it != textures.end()) {
        // ¡ENCONTRADO! Devolvemos la textura ya cargada. 0 lecturas de disco.
        return it->second;
    }

    // 2. CARGA DESDE DISCO (Solo si no estaba en caché)
    std::string fullPath = assetPath(relativePath);
    Texture2D tex = LoadTexture(fullPath.c_str());

    // Manejo de errores (Textura magenta si falla)
    if (tex.id == 0) {
        std::cerr << "[ResourceManager] ERROR: No se pudo cargar " << fullPath << "\n";
        Image errImg = GenImageColor(32, 32, MAGENTA);
        tex = LoadTextureFromImage(errImg);
        UnloadImage(errImg);
    } else {
        std::cout << "[ResourceManager] CARGANDO disco: " << relativePath << "\n";
    }

    // 3. GUARDAR EN CACHÉ
    textures[relativePath] = tex;
    
    return tex;
}

void ResourceManager::clear() {
    // Recorremos el mapa y descargamos todo
    for (auto& pair : textures) {
        UnloadTexture(pair.second);
    }
    textures.clear();
    std::cout << "[ResourceManager] Memoria liberada.\n";
}
