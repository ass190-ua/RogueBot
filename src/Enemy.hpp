#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Player.hpp"
#include <iostream>

class Enemy {
public:
    Enemy();

    void update(const Player& player);  // IA básica de persecución
    bool checkCollision(const Player& player) const;

    void draw() const;

private:
    int x, y;
};

#endif
