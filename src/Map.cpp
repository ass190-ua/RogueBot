#include "Map.hpp"
#include <algorithm>
#include <random>

extern "C" {
    #include "raylib.h"
}

static inline int clampi(int v, int lo, int hi) { return std::max(lo, std::min(v, hi)); }

static inline int manhattan(int ax,int ay,int bx,int by){
    return std::abs(ax-bx)+std::abs(ay-by);
}

bool Map::overlaps(const Room& a, const Room& b, int pad) {
    // AABB con padding para separar salas
    return !(a.x + a.w + pad <= b.x || b.x + b.w + pad <= a.x ||
             a.y + a.h + pad <= b.y || b.y + b.h + pad <= a.y);
}

Map::Map() {}

void Map::generate(int W, int H, unsigned seed) {
    m_w = W; m_h = H;
    m_tiles.assign(W * H, WALL);
    m_visible.assign(W * H, 0);
    m_discovered.assign(W * H, 0);
    m_rooms.clear();

    std::mt19937 rng(seed ? seed : std::random_device{}());
    std::uniform_int_distribution<int> rw(6, 12);
    std::uniform_int_distribution<int> rh(5, 10);
    std::uniform_int_distribution<int> rx(1, W - 14);
    std::uniform_int_distribution<int> ry(1, H - 12);

    const int targetRooms = 10;
    int attempts = 0;
    while ((int)m_rooms.size() < targetRooms && attempts < targetRooms * 20) {
        attempts++;
        Room r { rx(rng), ry(rng), rw(rng), rh(rng) };
        // clamp para no salir de límites
        r.w = clampi(r.w, 3, W-2);
        r.h = clampi(r.h, 3, H-2);
        r.x = clampi(r.x, 1, W - r.w - 1);
        r.y = clampi(r.y, 1, H - r.h - 1);

        bool ok = true;
        for (auto& o : m_rooms) if (overlaps(r, o, /*padding*/1)) { ok = false; break; }
        if (!ok) continue;

        carveRoom(r);
        m_rooms.push_back(r);
    }

    // Conectar salas en orden con túneles (grosor 1–2)
    std::uniform_int_distribution<int> coin(0,1);
    for (size_t i = 1; i < m_rooms.size(); ++i) {
        const Room& a = m_rooms[i-1];
        const Room& b = m_rooms[i];
        int ax = a.x + a.w/2, ay = a.y + a.h/2;
        int bx = b.x + b.w/2, by = b.y + b.h/2;
        int t = 2;

        if (coin(rng)) {
            carveHTunnel(std::min(ax,bx), std::max(ax,bx), ay, t);
            carveVTunnel(std::min(ay,by), std::max(ay,by), bx, t);
        } else {
            carveVTunnel(std::min(ay,by), std::max(ay,by), ax, t);
            carveHTunnel(std::min(ax,bx), std::max(ax,bx), by, t);
        }
    }

    // Marcar EXIT en el centro de la última sala (objetivo del piso)
    if (!m_rooms.empty()) {
        const Room& start = m_rooms.front();
        int sx = start.x + start.w/2;
        int sy = start.y + start.h/2;

        int bestIdx = 0, bestDist = -1;
        for (int i = 0; i < (int)m_rooms.size(); ++i) {
            const Room& r = m_rooms[i];
            int cx = r.x + r.w/2;
            int cy = r.y + r.h/2;
            int d = manhattan(sx, sy, cx, cy);
            if (d > bestDist) { bestDist = d; bestIdx = i; }
        }
        const Room& e = m_rooms[bestIdx];
        int cx = e.x + e.w/2, cy = e.y + e.h/2;
        m_tiles[cy * m_w + cx] = EXIT;
    }
}

void Map::carveRoom(const Room& r) {
    for (int y = r.y; y < r.y + r.h && y < m_h; ++y)
        for (int x = r.x; x < r.x + r.w && x < m_w; ++x)
            m_tiles[y * m_w + x] = FLOOR;
}

void Map::carveHTunnel(int x1, int x2, int y, int thickness) {
    for (int x = x1; x <= x2 && x < m_w; ++x)
        for (int t = 0; t < thickness; ++t) {
            int yy = y + t;
            if (yy >= 0 && yy < m_h) m_tiles[yy * m_w + x] = FLOOR;
        }
}

void Map::carveVTunnel(int y1, int y2, int x, int thickness) {
    for (int y = y1; y <= y2 && y < m_h; ++y)
        for (int t = 0; t < thickness; ++t) {
            int xx = x + t;
            if (xx >= 0 && xx < m_w) m_tiles[y * m_w + xx] = FLOOR;
        }
}

void Map::computeVisibility(int px, int py, int radius) {
    std::fill(m_visible.begin(), m_visible.end(), 0);
    int r2 = radius * radius;

    int x0 = std::max(0, px - radius);
    int x1 = std::min(m_w - 1, px + radius);
    int y0 = std::max(0, py - radius);
    int y1 = std::min(m_h - 1, py + radius);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            int dx = x - px, dy = y - py;
            if (dx*dx + dy*dy <= r2) {
                m_visible[y * m_w + x] = 1;
                m_discovered[y * m_w + x] = 1; // se queda “recordado”
            }
        }
    }
}

void Map::draw(int tileSize) const {
    for (int y = 0; y < m_h; ++y) {
        for (int x = 0; x < m_w; ++x) {
            // color base por tipo de tile
            Color c = DARKGRAY; // WALL
            if (at(x,y) == FLOOR) c = (Color){35,35,35,255};
            else if (at(x,y) == EXIT) c = (Color){0,120,80,255};

            // niebla: si nunca se ha visto -> casi negro; si no visible ahora -> más oscuro
            bool disc = m_discovered[y * m_w + x] != 0;
            bool vis  = m_visible[y * m_w + x] != 0;
            if (!disc) {
                c = (Color){10,10,10,255};
            } else if (!vis) {
                c = (Color){20,20,20,255};
            }

            DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, c);
        }
    }
}
