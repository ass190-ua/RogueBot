#ifndef MAP_HPP
#define MAP_HPP

#include <vector>
#include <cstdint>

enum Tile : uint8_t { WALL = 0, FLOOR = 1, EXIT = 2 };

struct Room { int x, y, w, h; };

class Map {
public:
    Map();

    void generate(int width, int height, unsigned seed = 0);
    void draw(int tileSize) const;
    void computeVisibility(int px, int py, int radius);

    bool isVisible(int x, int y) const { return m_visible[y * m_w + x] != 0; }
    bool isDiscovered(int x, int y) const { return m_discovered[y * m_w + x] != 0; }

    int width()  const { return m_w; }
    int height() const { return m_h; }

    Tile at(int x, int y) const { return m_tiles[y * m_w + x]; }

    Room firstRoom() const { return m_rooms.empty() ? Room{0,0,0,0} : m_rooms.front(); }
    Room lastRoom()  const { return m_rooms.empty() ? Room{0,0,0,0} : m_rooms.back(); }

private:
    int m_w = 0, m_h = 0;
    
    std::vector<Tile> m_tiles;
    std::vector<Room> m_rooms;
    std::vector<uint8_t> m_visible;    // 0/1 visible este frame
    std::vector<uint8_t> m_discovered; // 0/1 visto alguna vez

    void carveRoom(const Room& r);
    void carveHTunnel(int x1, int x2, int y, int thickness = 1);
    void carveVTunnel(int y1, int y2, int x, int thickness = 1);

    static bool overlaps(const Room& a, const Room& b, int padding = 1);
};

#endif
