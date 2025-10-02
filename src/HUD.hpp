#ifndef HUD_HPP
#define HUD_HPP

// Forward declaration para evitar dependencias circulares
class Game;

class HUD {
public:
    void drawPlaying(const Game& game) const;
    void drawVictory(const Game& game) const;
    void drawGameOver(const Game& game) const; // para cuando se añada Game Over
};

#endif
