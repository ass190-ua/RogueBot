#ifndef RESOURCEMANAGER_HPP
#define RESOURCEMANAGER_HPP

#include "raylib.h"
#include <string>
#include <map>

// Clase Singleton: Garantiza una única instancia en todo el programa
class ResourceManager {
public:
    // Método estático para acceder a la única instancia (Global)
    static ResourceManager& getInstance() {
        static ResourceManager instance; // Se crea la primera vez que se llama
        return instance;
    }

    // Eliminar constructor de copia para evitar duplicados accidentales
    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    // Obtener textura: Si ya existe, la devuelve. Si no, la carga del disco.
    Texture2D getTexture(const std::string& path);

    // Liberar toda la memoria al cerrar el juego
    void clear();

private:
    ResourceManager() {} // Constructor privado (Nadie puede hacer 'new ResourceManager')
    
    // Caché: Mapa que asocia "ruta del archivo" -> "Textura cargada"
    std::map<std::string, Texture2D> textures;
};

#endif
