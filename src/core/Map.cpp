#include "Map.hpp"
#include <algorithm>
#include <random>
#include "raylib.h"

// Helper: Limita un valor entre un mínimo (lo) y un máximo (hi)
static inline int clampi(int v, int lo, int hi) { return std::max(lo, std::min(v, hi)); }

// Helper: Distancia Manhattan (|dx| + |dy|).
// Es más rápida que la Euclídea (Pitágoras) y funciona genial en grillas cuadradas.
static inline int manhattan(int ax,int ay,int bx,int by){
    return std::abs(ax-bx)+std::abs(ay-by);
}

// Verifica si dos rectángulos (Salas) se tocan o solapan.
// Usa AABB (Axis-Aligned Bounding Box) con un margen 'pad' extra.
bool Map::overlaps(const Room& a, const Room& b, int pad) {
    return !(a.x + a.w + pad <= b.x || b.x + b.w + pad <= a.x ||
             a.y + a.h + pad <= b.y || b.y + b.h + pad <= a.y);
}

Map::Map() {}

// Algoritmo de generación de mazmorras
void Map::generate(int W, int H, unsigned seed) {
    m_w = W; m_h = H;
    
    // Inicializar todo como muro sólido
    m_tiles.assign(W*H, WALL);
    m_visible.assign(W*H, 0);
    m_discovered.assign(W*H, 0);
    m_rooms.clear();

    // Semilla aleatoria (seed) para reproducibilidad
    std::mt19937 rng(seed ? seed : std::random_device{}());

    // 1. Configuración dinámica de tamaños
    // Las salas escalan según el tamaño total del mapa.
    auto clampi = [](int v,int lo,int hi){ return std::max(lo,std::min(v,hi)); };

    const int minRoomW = clampi(W/12, 4, std::max(4, W-2));
    const int maxRoomW = clampi(W/6,  minRoomW+2, std::max(6, W-2));
    const int minRoomH = clampi(H/12, 4, std::max(4, H-2));
    const int maxRoomH = clampi(H/6,  minRoomH+2, std::max(6, H-2));

    // Calculamos cuántas salas intentar crear basado en el área total
    // (area / 150) es una heurística de densidad empírica.
    const int area = W * H;
    const int targetRooms = clampi(area / 150, 6, 20);

    // 2. Colocación de salas (Rejection Sampling)
    int attempts = 0;
    const int maxAttempts = targetRooms * 25; // Límite para evitar bucles infinitos

    while ((int)m_rooms.size() < targetRooms && attempts < maxAttempts) {
        attempts++;

        // Generar dimensiones aleatorias
        std::uniform_int_distribution<int> rw(minRoomW, maxRoomW);
        std::uniform_int_distribution<int> rh(minRoomH, maxRoomH);
        Room r;
        r.w = rw(rng);
        r.h = rh(rng);

        // Generar posición aleatoria (respetando bordes)
        if (W - r.w - 2 <= 0 || H - r.h - 2 <= 0) continue; 
        std::uniform_int_distribution<int> rx(1, W - r.w - 1);
        std::uniform_int_distribution<int> ry(1, H - r.h - 1);
        r.x = rx(rng);
        r.y = ry(rng);

        // Chequeo de colisión con salas existentes
        bool ok = true;
        for (const auto& o : m_rooms)
            if (overlaps(r, o, /*pad*/1)) { ok = false; break; } // Padding 1 para dejar muros entre salas
        if (!ok) continue;

        // Si es válida, la "esculpimos" y guardamos
        carveRoom(r);
        m_rooms.push_back(r);
    }

    // 3. Conexión de pasillos (Tubo en 'L')
    std::uniform_int_distribution<int> coin(0,1);
    // Grosor dinámico de pasillos (más anchos en mapas grandes)
    const int thickness = clampi((int)std::round(std::min(W,H) / 50.0), 2, 2);

    for (size_t i = 1; i < m_rooms.size(); ++i) {
        // Conectar la sala actual (b) con la anterior (a)
        const Room& a = m_rooms[i-1];
        const Room& b = m_rooms[i];
        
        // Puntos centrales de cada sala
        int ax = a.x + a.w/2, ay = a.y + a.h/2;
        int bx = b.x + b.w/2, by = b.y + b.h/2;

        // Decisión aleatoria: ¿Primero horizontal y luego vertical, o al revés?
        if (coin(rng)) {
            carveHTunnel(std::min(ax,bx), std::max(ax,bx), ay, thickness);
            carveVTunnel(std::min(ay,by), std::max(ay,by), bx, thickness);
        } else {
            carveVTunnel(std::min(ay,by), std::max(ay,by), ax, thickness);
            carveHTunnel(std::min(ax,bx), std::max(ax,bx), by, thickness);
        }
    }

    // 4. Colocación de salida (Meta)
    if (!m_rooms.empty()) {
        const Room& start = m_rooms.front(); // El jugador empieza en la sala 0
        int sx = start.x + start.w/2;
        int sy = start.y + start.h/2;

        // Buscamos la sala más lejana en distancia Manhattan
        int bestIdx = 0, bestDist = -1;
        for (int i = 0; i < (int)m_rooms.size(); ++i) {
            const Room& r = m_rooms[i];
            int cx = r.x + r.w/2, cy = r.y + r.h/2;
            int d = std::abs(sx - cx) + std::abs(sy - cy); 
            if (d > bestDist) { bestDist = d; bestIdx = i; }
        }
        
        // Convertimos el centro de esa sala lejana en la Salida
        const Room& e = m_rooms[bestIdx];
        int cx = e.x + e.w/2, cy = e.y + e.h/2;
        m_tiles[cy * m_w + cx] = EXIT;
    }
}

// Cambia celdas de WALL a FLOOR en el rectángulo dado
void Map::carveRoom(const Room& r) {
    for (int y = r.y; y < r.y + r.h && y < m_h; ++y)
        for (int x = r.x; x < r.x + r.w && x < m_w; ++x)
            m_tiles[y * m_w + x] = FLOOR;
}

// Crea túnel horizontal
void Map::carveHTunnel(int x1, int x2, int y, int thickness) {
    for (int x = x1; x <= x2 && x < m_w; ++x)
        for (int t = 0; t < thickness; ++t) {
            int yy = y + t;
            if (yy >= 0 && yy < m_h) m_tiles[yy * m_w + x] = FLOOR;
        }
}

// Crea túnel vertical
void Map::carveVTunnel(int y1, int y2, int x, int thickness) {
    for (int y = y1; y <= y2 && y < m_h; ++y)
        for (int t = 0; t < thickness; ++t) {
            int xx = x + t;
            if (xx >= 0 && xx < m_w) m_tiles[y * m_w + xx] = FLOOR;
        }
}

// Calcula campo de visión simple (Círculo relleno)
void Map::computeVisibility(int px, int py, int radius) {
    std::fill(m_visible.begin(), m_visible.end(), 0); // Resetear visión del frame anterior
    int r2 = radius * radius; // Usamos distancia cuadrada para evitar raíz cuadrada (más rápido)

    // Optimización: Solo iterar en el cuadrado que contiene el círculo (Bounding Box)
    int x0 = std::max(0, px - radius);
    int x1 = std::min(m_w - 1, px + radius);
    int y0 = std::max(0, py - radius);
    int y1 = std::min(m_h - 1, py + radius);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - px, dy = y - py;
            if (dx*dx + dy*dy <= r2) {
                // Si está dentro del círculo, es visible
                m_visible[y * m_w + x] = 1;
                // Y queda descubierto para siempre en el minimapa
                m_discovered[y * m_w + x] = 1;
            }
        }
    }
}

// Renderizado
void Map::draw(int tileSize, int px, int py, int radius, 
               const Texture2D& wallTex, const Texture2D& floorTex) const {
               
    for (int y = 0; y < m_h; ++y) {
        for (int x = 0; x < m_w; ++x) {
            
            // Si no está descubierto, Negro absoluto
            if (!m_revealAll && m_discovered[y * m_w + x] == 0) continue;

            // --- CÁLCULO DE ILUMINACIÓN ---
            Color tint = WHITE;
            
            if (!m_revealAll && m_fogEnabled) {
                // 1. ZONA DE MEMORIA
                if (m_visible[y * m_w + x] == 0) {
                     tint = { 40, 40, 50, 255 }; 
                } 
                // 2. ZONA VISIBLE (Antorcha)
                else {
                    // Calculamos distancia al jugador
                    float dx = (float)(x - px);
                    float dy = (float)(y - py);
                    float dist = std::sqrt(dx*dx + dy*dy);
                    
                    // Factor de luz (1.0 en el centro, 0.0 en el borde del radio)
                    // El "+ 1.0f" es para suavizar el borde
                    float light = 1.0f - (dist / (float)(radius + 1));
                    light = std::clamp(light, 0.0f, 1.0f);
                    
                    // Curva de luz para que el centro sea muy brillante y caiga rápido
                    // (Efecto linterna)
                    light = powf(light, 0.5f); 

                    // Aplicamos la luz, pero asegurando un mínimo para que se vea
                    unsigned char val = (unsigned char)(255.0f * light);
                    if (val < 60) val = 60; // Mínimo de luz en zona visible
                    
                    tint = { val, val, val, 255 };
                }
            }

            // --- DIBUJADO (Igual que antes) ---
            Rectangle dest = { 
                (float)(x * tileSize), 
                (float)(y * tileSize), 
                (float)tileSize, 
                (float)tileSize 
            };
            Vector2 origin = { 0, 0 };
            Tile t = at(x, y);

            if (t == WALL) {
                Rectangle src = { 0, 0, (float)wallTex.width, (float)wallTex.height };
                DrawTexturePro(wallTex, src, dest, origin, 0.0f, tint);
            } 
            else if (t == FLOOR || t == EXIT) {
                Rectangle src = { 0, 0, (float)floorTex.width, (float)floorTex.height };
                DrawTexturePro(floorTex, src, dest, origin, 0.0f, tint);

                if (t == EXIT) {
                    DrawRectangleRec(dest, Fade(GREEN, 0.4f));
                    DrawRectangleLinesEx(dest, 2, LIME);
                }
            }
        }
    }
}
