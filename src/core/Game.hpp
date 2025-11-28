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

struct Projectile {
    Vector2 pos;
    Vector2 vel;
    float   distanceTraveled = 0.0f;
    float   maxDistance = 0.0f;
    int     damage = 0;
    bool    active = true;
};

struct FloatingText {
    Vector2 pos;      // Posición en píxeles
    int value;        // El número a mostrar
    float timer;      // Tiempo de vida (ej: 1.0s)
    float lifeTime;   // Duración total (para calcular transparencia)
    Color color;      // Blanco para daño al enemigo, Rojo para daño al jugador
};

struct Particle {
    Vector2 pos;      // Posición en píxeles
    Vector2 vel;      // Velocidad (Dirección + Rapidez)
    float life;       // Vida actual
    float maxLife;    // Vida total (para calcular transparencia)
    float size;       // Tamaño del cuadradito
    Color color;      // Color de la partícula
};

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

    const std::vector<Enemy>& getEnemies() const { return enemies; }
    const std::vector<ItemSpawn>& getItems() const { return items; }

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
    float getDashCooldown() const { return dashCooldownTimer; }

    enum class EnemyFacing { Down, Up, Left, Right };

private:
    Player player;
    int screenW;
    int screenH;
    int tileSize = 32;

    Map map;
    int px = 0, py = 0;
    int hp = 10;
    int hpMax = 10;
    float damageCooldown = 0.0f;
    const float DAMAGE_COOLDOWN = 0.2f;

    unsigned fixedSeed = 0;
    unsigned runSeed = 0;
    unsigned levelSeed = 0;

    std::mt19937 rng;
    RunContext runCtx;
    std::vector<ItemSpawn> items;

    std::vector<int>   enemyHP;
    std::vector<float> enemyAtkCD;
    std::vector<float> enemyFlashTimer;
    std::vector<Enemy> enemies;
    int ENEMY_DETECT_RADIUS_PX = 32 * 6;

    std::vector<EnemyFacing> enemyFacing;
    void enemyTryAttackFacing();

    int enemiesPerLevel(int lvl) const { return (lvl == 1) ? 5 : (lvl == 2) ? 8 : 12;}

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
    float shakeTimer = 0.0f;

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

    // --- SISTEMA DE COMBATE Y PROYECTILES (NUEVO) ---
    std::vector<Projectile> projectiles; 
    
    float plasmaCooldown = 0.0f;
    int burstShotsLeft = 0;
    float burstTimer = 0.0f;

    // Funciones de ataque
    void performMeleeAttack();  // Manos
    void performSwordAttack();  // Espada
    void performPlasmaAttack(); // Plasma
    
    // Auxiliares proyectiles
    void spawnProjectile(int dmg);
    void updateProjectiles(float dt);
    void drawProjectiles() const;

    // --- TEXTOS FLOTANTES ---
    std::vector<FloatingText> floatingTexts;
    
    void spawnFloatingText(Vector2 pos, int value, Color color);
    void updateFloatingTexts(float dt);
    void drawFloatingTexts() const;

    // --- SISTEMA DE PARTÍCULAS ---
    std::vector<Particle> particles;
    
    // Función para crear una explosión en un punto (x,y)
    void spawnExplosion(Vector2 pos, int count, Color color);
    
    void updateParticles(float dt);
    void drawParticles() const;

    // SONIDOS
    Sound sfxHit{};       // Golpe seco
    Sound sfxExplosion{}; // Muerte enemigo
    Sound sfxPickup{};    // Objeto normal (Pila/Escudo)
    Sound sfxPowerUp{};   // Objeto nivel (Espada/Plasma)
    Sound sfxHurt{};      // Daño al jugador
    Sound sfxWin{};       // Victoria
    Sound sfxLoose{};     // Game Over
    Sound sfxDash{};      // Sonido de esquiva
    
    // AMBIENTE
    Sound sfxAmbient{};   // Zumbido de fondo (Drone)
    
    // Función auxiliar para generarlos sin archivos
    Sound generateSound(int type);

    // --- DASH (ESQUIVA) ---
    bool isDashing = false;       // ¿Está ocurriendo ahora mismo?
    float dashTimer = 0.0f;       // Duración del dash (muy corta, ej: 0.15s)
    float dashCooldownTimer = 0.0f; // Tiempo para volver a usarlo
    
    // Configuración (puedes ajustarlo luego)
    const float DASH_DURATION = 0.15f; 
    const float DASH_COOLDOWN = 2.0f;  
    const int DASH_DISTANCE = 3;       // Cuantas casillas avanza
    
    Vector2 dashStartPos{}; // Para interpolación visual suave
    Vector2 dashEndPos{};
};

#endif