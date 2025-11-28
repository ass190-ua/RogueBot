#ifndef GAME_HPP
#define GAME_HPP

#include <random>
#include <vector>
#include <string>
#include "Map.hpp"
#include "HUD.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "raylib.h"
#include "ItemSpawner.hpp"

enum class MovementMode {
    StepByStep,
    RepeatCooldown
};

enum class GameState {
    MainMenu,
    Playing,
    Victory,
    GameOver
};

enum class InputDevice {
    Keyboard,
    Gamepad
};

struct ItemSprites {
    Texture2D keycard{};
    Texture2D shield{};
    Texture2D pila{};
    Texture2D glasses{};
    Texture2D swordBlue{};
    Texture2D swordGreen{};
    Texture2D swordRed{};
    Texture2D plasma1{};
    Texture2D plasma2{};
    Texture2D battery{};
    Texture2D enemy{};

    Texture2D enemyUp{};
    Texture2D enemyDown{};
    Texture2D enemyLeft{};
    Texture2D enemyRight{};
    bool loaded = false;
    void load();
    void unload();
};

class Game {
public:
    explicit Game(unsigned seed = 0);
    void run();

    // Getters HUD
    int getScreenW() const { return screenW; }
    int getScreenH() const { return screenH; }
    unsigned getRunSeed() const { return runSeed; }
    unsigned getLevelSeed() const { return levelSeed; }
    int getCurrentLevel() const { return currentLevel; }
    int getMaxLevels() const { return maxLevels; }
    const char *movementModeText() const;
    const Map &getMap() const { return map; }
    int getPlayerX() const { return px; }
    int getPlayerY() const { return py; }

    int getFovRadius() const;

    int getHP() const { return hp; }
    int getHPMax() const { return hpMax; }
    
    bool isShieldActive() const { return hasShield; }
    float getShieldTime() const { return shieldTimer; }
    float getGlassesTime() const { return glassesTimer; }

    enum class EnemyFacing { Down, Up, Left, Right };

private:
    Player player;
    int screenW;
    int screenH;
    int tileSize = 32;

    Map map;
    int px = 0, py = 0;
    int hp = 5;
    int hpMax = 5;
    float damageCooldown = 0.0f;
    const float DAMAGE_COOLDOWN = 0.6f;

    unsigned fixedSeed = 0;
    unsigned runSeed = 0;
    unsigned levelSeed = 0;

    std::mt19937 rng;
    RunContext runCtx;
    std::vector<ItemSpawn> items;

    std::vector<int>   enemyHP;
    std::vector<float> enemyAtkCD;
    std::vector<Enemy> enemies;
    int ENEMY_DETECT_RADIUS_PX = 32 * 6;

    std::vector<EnemyFacing> enemyFacing;
    void enemyTryAttackFacing();

    int enemiesPerLevel(int lvl) const { return (lvl == 1) ? 3 : (lvl == 2) ? 4 : 5; }

    InputDevice lastInput = InputDevice::Keyboard; // Por defecto teclado

    void spawnEnemiesForLevel();
    void updateEnemiesAfterPlayerMove(bool moved);
    void drawEnemies() const;
    void takeDamage(int amount);

    MovementMode moveMode = MovementMode::StepByStep;
    float moveCooldown = 0.0f;
    const float MOVE_INTERVAL = 0.12f;

    GameState state = GameState::MainMenu;
    int currentLevel = 1;
    const int maxLevels = 3;

    HUD hud;

    void newRun();
    void newLevel(int level);
    unsigned nextRunSeed() const;
    unsigned seedForLevel(unsigned base, int level) const;
    
    void processInput();
    void update();
    void render();
    void renderMainMenu();
    void renderHelpOverlay();
    Rectangle uiCenterRect(float w, float h) const;
    void clampCameraToMap();

    void tryMove(int dx, int dy);
    void onExitReached();

    bool fogEnabled = true;
    int fovTiles = 8; // Base FOV
    int defaultFovFromViewport() const;

    Camera2D camera{};
    float cameraZoom = 1.0f;

    void drawItems() const;
    void drawItemSprite(const ItemSpawn &it) const;

    // Inventario
    bool hasKey = false;
    
    // Escudo
    bool hasShield = false; 
    float shieldTimer = 0.0f;

    // Batería (Resurrección)
    bool hasBattery = false; 

    // Gafas 3D
    float glassesTimer = 0.0f;
    int glassesFovMod = 0;

    int swordTier = 0;
    int plasmaTier = 0;

    void tryAutoPickup();
    void tryManualPickup();
    void onPickup(const ItemSpawn &it);

    ItemSprites itemSprites;

    bool showHelp = false;
    int helpScroll = 0;
    std::string helpText;
    
    // Menú
    int mainMenuSelection = 0;

    void handleMenuInput();
    void handlePlayingInput(float dt);
    void centerCameraOnPlayer();
    void recomputeFovIfNeeded();
    void onSuccessfulStep(int dx, int dy);
};

#endif