#include "Game.hpp"
#include <iostream>

Game::Game() : running(true), state(GameState::PLAYING) {
    map.generate();  // ← importante: generamos el mapa una vez
    std::cout << "RogueBot Alpha\n"
              << "Controles: w/a/s/d + ENTER para mover | r + ENTER reinicia | q + ENTER sale\n"
              << std::endl;
}

void Game::run() {
    while (running) {
        processInput();
        update();
        render();
    }
}

void Game::reset() {
    player = Player();
    enemy  = Enemy();
    state  = GameState::PLAYING;
    std::cout << "\n[Reset]\n";
}

void Game::processInput() {
    char input;
    std::cout << "> " << std::flush;   // prompt visible
    std::cin >> input;                 // bloqueante (válido para Alpha)

    // mover jugador si corresponde
    if (input == 'w' || input == 'a' || input == 's' || input == 'd') {
        player.move(input);
    }
    if (input == 'q') running = false;
    if (input == 'r') reset();
}

void Game::update() {
    if (state == GameState::PLAYING) {
        enemy.update(player);
        if (enemy.checkCollision(player)) {
            player.takeDamage(1);
        }
        if (player.isDead()) {
            state = GameState::GAME_OVER;
        }
    }
}

void Game::render() {
    // (opcional) limpiar consola para que se vea tipo “frames”:
    std::cout << "\033[2J\033[H"; // clear + home (ANSI)

    if (state == GameState::PLAYING) {
        map.draw();
        player.draw();
        enemy.draw();
        hud.draw(player);
    } else {
        hud.drawGameOver();
    }
}
