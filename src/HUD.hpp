#ifndef HUD_HPP
#define HUD_HPP

#include "Player.hpp"
#include <iostream>

class HUD {
public:
    void draw(const Player& player) const;
    void drawGameOver() const;
};

#endif
