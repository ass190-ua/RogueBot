#include "Game.hpp"
#include <iostream>

Game::Game() : running(true), state(GameState::PLAYING) {}

void Game::run() {
    while (running) {
        processInput();
        update();
        render();
    }
}

void Game::reset() {
    player = Player();
    enemy = Enemy();
    state = GameState::PLAYING;
}

void Game::processInput() {
    // TODO: capturar entrada de usuario (movimiento, reinicio)
    // Ejemplo placeholder:
    char input;
    std::cin >> input;
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
    if (state == GameState::PLAYING) {
        map.draw();
        player.draw();
        enemy.draw();
        hud.draw(player);
    } else if (state == GameState::GAME_OVER) {
        hud.drawGameOver();
    }
}
