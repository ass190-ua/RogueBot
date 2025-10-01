#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <iostream>

class Player {
public:
    Player();

    void move(char direction);   // movimiento básico con input
    void takeDamage(int dmg);
    bool isDead() const;

    void draw() const;

    // getters
    int getX() const { return x; }
    int getY() const { return y; }
    int getHealth() const { return health; }

private:
    int x, y;
    int health;
};

#endif
