#ifndef GAME_HPP
#define GAME_HPP

#include "Player.hpp"
#include "Enemy.hpp"
#include "Map.hpp"
#include "HUD.hpp"
#include "State.hpp"

class Game {
public:
    Game();
    void run();       // bucle principal
    void reset();     // reiniciar partida

private:
    bool running;
    GameState state;

    Player player;
    Enemy enemy;
    Map map;
    HUD hud;

    void processInput();
    void update();
    void render();
};

#endif
