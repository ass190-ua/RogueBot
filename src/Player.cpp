#include "Player.hpp"

Player::Player() : x(0), y(0), health(3) {}

void Player::move(char direction) {
    switch(direction) {
        case 'w': y--; break;
        case 's': y++; break;
        case 'a': x--; break;
        case 'd': x++; break;
        default: break;
    }
}

void Player::takeDamage(int dmg) {
    health -= dmg;
    std::cout << "Player takes " << dmg << " damage! Health: " << health << "\n";
}

bool Player::isDead() const {
    return health <= 0;
}

void Player::draw() const {
    std::cout << "Player at (" << x << "," << y << ") Health: " << health << "\n";
}
