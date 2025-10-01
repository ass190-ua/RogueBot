#include "Enemy.hpp"
#include <cmath>

Enemy::Enemy() : x(5), y(5) {}

void Enemy::update(const Player& player) {
    if (player.getX() > x) x++;
    else if (player.getX() < x) x--;

    if (player.getY() > y) y++;
    else if (player.getY() < y) y--;
}

bool Enemy::checkCollision(const Player& player) const {
    return (x == player.getX() && y == player.getY());
}

void Enemy::draw() const {
    std::cout << "Enemy at (" << x << "," << y << ")\n";
}
