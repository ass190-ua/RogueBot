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

enum class MovementMode
{
    StepByStep,
    RepeatCooldown
};

enum class GameState
{
    MainMenu,
    Playing,
    Victory,
    GameOver
};

struct ItemSprites
{
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

class Game
{
public:
    explicit Game(unsigned seed = 0);
    void run();

    // === Getters usados por HUD ===
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

    // radio FOV
    int getFovRadius() const { return fovTiles; }

    // HUD: Getters de vida
    int getHP() const { return hp; }
    int getHPMax() const { return hpMax; }

    enum class EnemyFacing
    {
        Down,
        Up,
        Left,
        Right
    };

private:
    // Jugador
    Player player;

    // Pantalla / tiles
    int screenW;
    int screenH;
    int tileSize = 32;

    // Mundo y jugador
    Map map;
    int px = 0, py = 0;
    int hp = 5;
    int hpMax = 5;
    float damageCooldown = 0.0f;        // invulnerabilidad breve tras recibir daño
    const float DAMAGE_COOLDOWN = 0.6f; // ~0.6s

    // Semillas
    unsigned fixedSeed = 0;
    unsigned runSeed = 0;
    unsigned levelSeed = 0;

    // RNG y contexto de run (para spawns)
    std::mt19937 rng;
    RunContext runCtx;
    std::vector<ItemSpawn> items;

    // --- Enemigos ---
    std::vector<Enemy> enemies;
    int ENEMY_DETECT_RADIUS_PX = 32 * 6; // ~6 tiles si tileSize=32

    std::vector<EnemyFacing> enemyFacing; // mismo tamaño que enemies
    void enemyTryAttackFacing();

    // Cantidad por nivel (ajústalo si quieres)
    int enemiesPerLevel(int lvl) const { return (lvl == 1) ? 3 : (lvl == 2) ? 4
                                                                            : 5; }

    // Helpers de enemigos
    void spawnEnemiesForLevel();                   // crear enemigos al iniciar nivel
    void updateEnemiesAfterPlayerMove(bool moved); // IA y colisión básica (placeholder)
    void drawEnemies() const;
    void takeDamage(int amount);

    // Movimiento
    MovementMode moveMode = MovementMode::StepByStep;
    float moveCooldown = 0.0f;
    const float MOVE_INTERVAL = 0.12f;

    // Estados y niveles
    GameState state = GameState::MainMenu;
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
    void renderMainMenu();
    void renderHelpOverlay();
    Rectangle uiCenterRect(float w, float h) const;
    void clampCameraToMap();

    // Utilidades
    void tryMove(int dx, int dy);
    void onExitReached();

    // niebla activada o no
    bool fogEnabled = true;

    int fovTiles = 8;
    int defaultFovFromViewport() const;

    // Cámara (útil si quieres zoom o scroll)
    Camera2D camera{};
    float cameraZoom = 1.0f; // control de zoom

    // Dibujo de ítems (placeholder de colores hasta tener sprites)
    void drawItems() const;
    void drawItemSprite(const ItemSpawn &it) const;

    // --- inventario mínimo ---
    bool hasKey = false;
    bool hasShield = false;
    bool hasBattery = false;
    int swordTier = 0;  // 0=sin espada, 1..3
    int plasmaTier = 0; // 0=sin pistola, 1..2

    // --- helpers de recogida ---
    void tryPickupHere();               // busca item en (px,py) y lo recoge
    void onPickup(const ItemSpawn &it); // aplica lógica de inventario

    // Sprites de ítems
    ItemSprites itemSprites;

    // --- Menú v2: visor “Leer antes de jugar” ---
    bool showHelp = false; // si está abierto el visor
    int helpScroll = 0;    // desplazamiento vertical del texto
    std::string helpText;
    Rectangle btnPlay{0, 0, 0, 0};
    Rectangle btnRead{0, 0, 0, 0};
    Rectangle btnBack{0, 0, 0, 0}; // botón "Volver" del panel de ayuda

};

#endif
