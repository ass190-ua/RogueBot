#include "HUD.hpp"

void HUD::draw(const Player& player) const {
    std::cout << "HUD >> Health: " << player.getHealth() << std::endl;
}

void HUD::drawGameOver() const {
    std::cout << "=== GAME OVER ===" << std::endl;
    std::cout << "Press R to restart or Q to quit." << std::endl;
}
