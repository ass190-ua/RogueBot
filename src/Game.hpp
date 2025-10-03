#ifndef GAME_HPP
#define GAME_HPP

#include "Map.hpp"
#include "HUD.hpp"
#include "Player.hpp"

enum class MovementMode {
    StepByStep,
    RepeatCooldown
};

enum class GameState {
    Playing,
    Victory
};

class Game {
public:
    explicit Game(unsigned seed = 0);
    void run();

    // === Getters usados por HUD ===
    int  getScreenW() const { return screenW; }
    int  getScreenH() const { return screenH; }
    unsigned getRunSeed()   const { return runSeed; }
    unsigned getLevelSeed() const { return levelSeed; }
    int  getCurrentLevel()  const { return currentLevel; }
    int  getMaxLevels()     const { return maxLevels; }
    const char* movementModeText() const;
    const Map& getMap() const { return map; }
    int getPlayerX() const { return px; }
    int getPlayerY() const { return py; }

    // radio FOV 
    int getFovRadius() const { return 8; }

private:
    // Jugador
    Player player;
    
    // Pantalla / tiles
    int screenW;
    int screenH;
    int tileSize = 16;

    // Mundo y jugador
    Map map;
    int px = 0, py = 0;
    int hp = 3;

    // Semillas
    unsigned fixedSeed = 0;
    unsigned runSeed   = 0;
    unsigned levelSeed = 0;

    // Movimiento
    MovementMode moveMode = MovementMode::StepByStep;
    float moveCooldown = 0.0f;
    const float MOVE_INTERVAL = 0.12f;

    // Estados y niveles
    GameState state = GameState::Playing;
    int currentLevel = 1;
    const int maxLevels = 3;

    // HUD
    HUD hud;

    // Ciclo
    void newRun();
    void newLevel(int level);
    unsigned nextRunSeed() const;
    unsigned seedForLevel(unsigned base, int level) const;
    void processInput();
    void update();
    void render();

    // Utilidades
    void tryMove(int dx, int dy);
    void onExitReached();

    // niebla activada o no
    bool fogEnabled = true;
};

#endif
