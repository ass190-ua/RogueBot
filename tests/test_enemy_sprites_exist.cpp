#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace fs = std::filesystem;

  const std::vector<std::string> files = {
      // enemy1 (idle + 2 frames por dirección)
      "assets/sprites/enemies/enemy1.png",
      "assets/sprites/enemies/enemy1_up1.png",
      "assets/sprites/enemies/enemy1_up2.png",
      "assets/sprites/enemies/enemy1_down1.png",
      "assets/sprites/enemies/enemy1_down2.png",
      "assets/sprites/enemies/enemy1_left1.png",
      "assets/sprites/enemies/enemy1_left2.png",
      "assets/sprites/enemies/enemy1_right1.png",
      "assets/sprites/enemies/enemy1_right2.png",

      // enemy2 (idle + 2 frames por dirección)
      "assets/sprites/enemies/enemy2.png",
      "assets/sprites/enemies/enemy2_up1.png",
      "assets/sprites/enemies/enemy2_up2.png",
      "assets/sprites/enemies/enemy2_down1.png",
      "assets/sprites/enemies/enemy2_down2.png",
      "assets/sprites/enemies/enemy2_left1.png",
      "assets/sprites/enemies/enemy2_left2.png",
      "assets/sprites/enemies/enemy2_right1.png",
      "assets/sprites/enemies/enemy2_right2.png",
  };

  int missing = 0;
  for (const auto &p : files) {
    if (!fs::exists(p)) {
      std::cerr << "[MISSING] " << p << "\n";
      ++missing;
    }
  }

  if (missing > 0) {
    std::cerr << "[FAIL] Faltan " << missing << " sprites de enemigos.\n";
    return 1;
  }

  std::cout << "[OK] Sprites de enemy1/enemy2 presentes (" << files.size()
            << ")\n";
  return 0;
}
