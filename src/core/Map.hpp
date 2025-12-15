#ifndef MAP_HPP
#define MAP_HPP

#include <vector>
#include <cstdint>
#include "raylib.h"
#include <utility>

// Tipos de celda. Usamos uint8_t para ahorrar memoria (1 byte por tile).
enum Tile : uint8_t { 
    WALL = 0,  // Muro (impide paso y visión)
    FLOOR = 1, // Suelo transitable
    EXIT = 2   // Meta del nivel
};

// Estructura simple para definir una habitación rectangular
struct Room { int x, y, w, h; };

class Map {
public:
    Map();

    // Generación y dibujado
    // Crea un nuevo nivel procedimental (algoritmo BSP o aleatorio).
    void generate(int width, int height, unsigned seed = 0);

    // Genera un mapa lineal sencillo para el tutorial
    void generateTutorialMap(int W, int H);

    void setTile(int x, int y, Tile t);
    
    // Genera la arena del Boss (espacio abierto)
    void generateBossArena(int width, int height);

    // Renderiza el mapa usando Raylib (Tile por tile).
    void draw(int tileSize, int px, int py, int radius, 
              const Texture2D& wallTex, const Texture2D& floorTex) const;    // Sistema de visión (FOV, "FOG OF WAR")
    
    // Calcula qué celdas ve el jugador desde (px, py) con un radio 'radius'.
    // Actualiza los vectores 'm_visible' y 'm_discovered'.
    void computeVisibility(int px, int py, int radius);
    
    // Activa/Desactiva la niebla (útil para debug o modos fáciles).
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }

    void setRevealAll(bool reveal) { m_revealAll = reveal; }

    // Consultas de visión:
    // isVisible: ¿Lo veo AHORA mismo? (Iluminado)
    // isDiscovered: ¿Lo he visto ALGUNA vez? (Grisáceo/Memoria)
    bool isVisible(int x, int y) const { return m_revealAll || m_visible[y * m_w + x] != 0; }
    bool isDiscovered(int x, int y) const { return m_revealAll || m_discovered[y * m_w + x] != 0; }
    bool fogEnabled() const { return m_fogEnabled; }

    // Acceso a datos (Geometría)
    
    int width()  const { return m_w; }
    int height() const { return m_h; }

    // Acceso directo a un tile.
    // Fórmula de linealización 2D -> 1D: índice = y * ancho + x
    Tile at(int x, int y) const { return m_tiles[y * m_w + x]; }

    // Verifica si una celda es válida para caminar (dentro de límites y no es muro)
    bool isWalkable(int x, int y) const {
        return (x >= 0 && y >= 0 && x < m_w && y < m_h) && (m_tiles[y * m_w + x] != WALL);
    }

    // Busca linealmente dónde está la salida (Tile::EXIT).
    std::pair<int,int> findExitTile() const {
        for (int y = 0; y < m_h; ++y) {
            for (int x = 0; x < m_w; ++x) {
                if (m_tiles[y * m_w + x] == EXIT) return {x, y};
            }
        }
        // Fallback de seguridad (centro del mapa) si no se generó salida
        return {m_w / 2, m_h / 2};
    }

    // Helpers para obtener puntos de inicio (jugador) y fin (meta)
    Room firstRoom() const { return m_rooms.empty() ? Room{0,0,0,0} : m_rooms.front(); }
    Room lastRoom()  const { return m_rooms.empty() ? Room{0,0,0,0} : m_rooms.back(); }

private:
    int m_w = 0, m_h = 0;

    // Almacenamiento (Flattened Vectors)
    // Usamos vectores planos (1D) en lugar de vector<vector<T>> por eficiencia de caché.
    std::vector<Tile> m_tiles; // El mapa físico
    std::vector<Room> m_rooms; // Lista de habitaciones generadas
    
    // Arrays paralelos para la niebla de guerra:
    std::vector<uint8_t> m_visible;    // 1 si está en FOV actual, 0 si no.
    std::vector<uint8_t> m_discovered; // 1 si se ha explorado alguna vez.

    // Funfiones internas de generación (Dungeon Carving)
    // "Esculpe" una habitación (pone tiles FLOOR en un rectángulo de WALLs)
    void carveRoom(const Room& r);
    
    // Crea pasillos (Túneles) horizontales y verticales para conectar salas
    void carveHTunnel(int x1, int x2, int y, int thickness = 1);
    void carveVTunnel(int y1, int y2, int x, int thickness = 1);

    // Verifica si dos habitaciones se superponen (con margen de padding)
    static bool overlaps(const Room& a, const Room& b, int padding = 1);

    bool m_revealAll = false; 
    bool m_fogEnabled = true;
};

#endif
