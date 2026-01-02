#include <iostream>

#include "Map.hpp"

int main() {
  Map m;

  struct Case {
    int W;
    int H;
    unsigned seed;
  };
  const Case cases[] = {
      {40, 25, 1u}, {20, 15, 42u}, {60, 30, 12345u}, {10, 10, 0u}};

  for (const auto &c : cases) {
    m.generate(c.W, c.H, c.seed);

    auto [ex, ey] = m.findExitTile();

    // Seguridad: in-bounds
    if (ex < 0 || ey < 0 || ex >= m.width() || ey >= m.height()) {
      std::cerr << "[FAIL] findExitTile fuera de rango: " << ex << "," << ey
                << " en generate(" << c.W << "," << c.H << "," << c.seed
                << ")\n";
      return 1;
    }

    // Check principal: el tile devuelto ES EXIT
    if (m.at(ex, ey) != EXIT) {
      std::cerr << "[FAIL] La celda devuelta por findExitTile no es EXIT\n";
      std::cerr << "  pos: (" << ex << "," << ey
                << ")  tile=" << int(m.at(ex, ey)) << "\n";
      return 1;
    }
  }

  std::cout << "[OK] findExitTile apunta a un tile EXIT\n";
  return 0;
}
