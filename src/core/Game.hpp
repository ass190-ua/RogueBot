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

// Estructuras de datos auxiliares (Entidades ligeras)
// Proyectil de Plasma (Disparo a distancia)
struct Projectile
{
    Vector2 pos;
    Vector2 vel; // Velocidad (Vector dirección * magnitud)
    float distanceTraveled = 0.0f;
    float maxDistance = 0.0f;
    int damage = 0;
    bool active = true;   // Se pone a false al chocar para borrarlo
    bool isEnemy = false; // Identifica si la bala es de un enemigo
};

// Texto flotante de daño ("+20", "-1HP") - Estilo RPG
struct FloatingText
{
    Vector2 pos;
    int value;      // Valor numérico a mostrar
    float timer;    // Tiempo restante de vida
    float lifeTime; // Duración total inicial (para el Alpha/FadeOut)
    Color color;    // Rojo (Daño recibido), Blanco (Daño infligido)
};

// Partículas simples para explosiones y efectos
struct Particle
{
    Vector2 pos;
    Vector2 vel;
    float life; // Vida restante
    float maxLife;
    float size; // Tamaño en píxeles
    Color color;
};

// Enums de estado
// Modo de movimiento del jugador:
// StepByStep: Clásico Roguelike (1 pulsación = 1 paso). Preciso.
// RepeatCooldown: Tipo Action-RPG (mantener pulsado para correr). Fluido.
enum class MovementMode
{
    StepByStep,
    RepeatCooldown
};

enum class GameState
{
    MainMenu,
    Playing,
    Paused,
    Victory,
    GameOver,
    OptionsMenu
};

// Para mostrar prompts correctos ("Presiona E" vs "Presiona A")
enum class InputDevice
{
    Keyboard,
    Gamepad
};

// Dificultad del juego. Permite seleccionar entre modos Fácil, Medio y Difícil.
// Esto afecta al número de enemigos y a su salud en cada nivel.
enum class Difficulty
{
    Easy,
    Medium,
    Hard
};

// Contenedor de todas las texturas del juego para carga centralizada
struct ItemSprites
{
    // Texturas mapa
    Texture2D wall{};
    Texture2D floor{};
    // Items
    Texture2D keycard{};
    Texture2D shield{};
    Texture2D pila{};
    Texture2D glasses{};
    Texture2D battery{};
    // Armas
    Texture2D swordBlue{};
    Texture2D swordGreen{};
    Texture2D swordRed{};
    Texture2D plasma1{};
    Texture2D plasma2{};
    // Enemigos (Direccionales)
    Texture2D enemy{}; // Fallback
    Texture2D enemyUp{};
    Texture2D enemyDown{};
    Texture2D enemyLeft{};
    Texture2D enemyRight{};

    // Texturas del Boss
    Texture2D bossUpIdle{};
    Texture2D bossDownIdle{};
    Texture2D bossLeftIdle{};
    Texture2D bossRightIdle{};
    Texture2D bossUpWalk1{};
    Texture2D bossUpWalk2{};
    Texture2D bossDownWalk1{};
    Texture2D bossDownWalk2{};
    Texture2D bossLeftWalk1{};
    Texture2D bossLeftWalk2{};
    Texture2D bossRightWalk1{};
    Texture2D bossRightWalk2{};

    bool loaded = false;
    void load();   // Carga todo
    void unload(); // Libera todo
};

struct Boss {
    bool active = false;
    bool awakened = false; // ¿Se ha despertado ya?
    int x = 0, y = 0;      // Posición lógica del Boss
    
    // Para detectar si el jugador se ha movido
    int playerStartX = 0;
    int playerStartY = 0;

    int hp = 0;
    int maxHp = 0;
    int phase = 1;         // Fase 1, 2, 3, 4

    // Dirección
    enum Facing { UP, DOWN, LEFT, RIGHT } facing = DOWN;

    // Temporizadores de Combate
    float actionCooldown = 0.0f; // Tiempo para disparar
    float moveTimer = 0.0f;      // Tiempo para dar el siguiente paso
    float flashTimer = 0.0f;     // Feedback de daño rojo
    float animTime = 0.0f;       // Animación
};

// Clase principal del juego (Game Loop)
class Game
{
public:
    // Constructor explícito para evitar conversiones implícitas accidentales de int a Game
    explicit Game(unsigned seed = 0);

    // Bucle principal: Init -> Update -> Render -> Cleanup
    void run();

    // Getters públicos (Para el HUD y Renderizado)
    // Son const porque el HUD solo lee, no modifica.

    const std::vector<Enemy> &getEnemies() const { return enemies; }
    const std::vector<ItemSpawn> &getItems() const { return items; }
    const Boss& getBoss() const { return boss; }

    int getScreenW() const { return screenW; }
    int getScreenH() const { return screenH; }

    // Semillas para reproducibilidad (Debug/Speedrun)
    unsigned getRunSeed() const { return runSeed; }
    unsigned getLevelSeed() const { return levelSeed; }

    int getCurrentLevel() const { return currentLevel; }
    int getMaxLevels() const { return maxLevels; }

    const char *movementModeText() const; // Texto para la UI ("Modo Pasos" / "Modo Continuo")

    const Map &getMap() const { return map; }
    int getPlayerX() const { return px; }
    int getPlayerY() const { return py; }

    int getFovRadius() const; // Radio de visión actual (afectado por Gafas 3D)

    // Estadísticas del Jugador
    int getHP() const { return hp; }
    int getHPMax() const { return hpMax; }

    // Estados de Power-Ups
    bool isShieldActive() const { return hasShield; }
    float getShieldTime() const { return shieldTimer; }
    float getGlassesTime() const { return glassesTimer; }
    float getDashCooldown() const { return dashCooldownTimer; }

    // Modo Dios
    bool isGodMode() const { return godMode; }
    bool isInputtingGodPassword() const { return showGodModeInput; }
    const std::string& getGodPasswordInput() const { return godModeInput; }

    // Dirección del Enemigo (para saber qué sprite dibujar)
    enum class EnemyFacing
    {
        Down,
        Up,
        Left,
        Right
    };

private:
    // Sistemas core
    Player player;
    HUD hud;
    Map map;

    int screenW;
    int screenH;
    int tileSize = 32;

    // Estado del Jugador
    int px = 0, py = 0; // Posición en rejilla
    int hp = 10;
    int hpMax = 10;

    // Invulnerabilidad tras recibir daño (i-frames)
    float damageCooldown = 0.0f;
    const float DAMAGE_COOLDOWN = 0.2f;

    // Generación Aleatoria (RNG)
    unsigned fixedSeed = 0; // Si != 0, fuerza la semilla
    unsigned runSeed = 0;   // Semilla global de la partida
    unsigned levelSeed = 0; // Semilla específica del nivel actual
    std::mt19937 rng;

    // Contexto persistente entre niveles (armas desbloqueadas, batería usada, etc.)
    RunContext runCtx;

    // Objetos en el suelo
    std::vector<ItemSpawn> items;
    ItemSprites itemSprites;

    // Gestión de enemigos
    // Vectores paralelos (SoA - Structure of Arrays) para rendimiento
    std::vector<Enemy> enemies;
    std::vector<int> enemyHP;
    std::vector<int> enemyMaxHP;
    std::vector<float> enemyAtkCD;      // Cooldown de ataque individual
    std::vector<float> enemyShootCD;    // Cooldown disparo (Shooter)
    std::vector<float> enemyFlashTimer; // Feedback visual de golpe
    std::vector<EnemyFacing> enemyFacing;

    int ENEMY_DETECT_RADIUS_PX = 32 * 6; // Radio de agresión

    void spawnEnemiesForLevel();
    int enemiesPerLevel(int lvl) const
    {
        // Determina cuántos enemigos debe haber por nivel según la dificultad.
        // Easy: 3,5,7  / Medium: 4,6,8  / Hard: 5,8,12
        switch (difficulty)
        {
        case Difficulty::Easy:
            return (lvl == 1) ? 3 : (lvl == 2) ? 5
                                               : 7;
        case Difficulty::Medium:
            return (lvl == 1) ? 4 : (lvl == 2) ? 6
                                               : 8;
        case Difficulty::Hard:
        default:
            return (lvl == 1) ? 5 : (lvl == 2) ? 8
                                               : 12;
        }
    }

    // --- VARIABLES CHEAT CODE (MANDO) ---
    // Secuencia: Arriba, Arriba, Abajo, Abajo, Izq, Der, Izq, Der, B, A
    const std::vector<int> konamiCode = {
        GAMEPAD_BUTTON_LEFT_FACE_UP,
        GAMEPAD_BUTTON_LEFT_FACE_UP,
        GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        GAMEPAD_BUTTON_LEFT_FACE_DOWN,
        GAMEPAD_BUTTON_LEFT_FACE_LEFT,
        GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
        GAMEPAD_BUTTON_LEFT_FACE_LEFT,
        GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
        GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, // B (Círculo)
        GAMEPAD_BUTTON_RIGHT_FACE_DOWN   // A (Cruz)
    };
    
    // Índice actual de la secuencia (cuántos has acertado seguidos)
    size_t cheatCodeIndex = 0;

    // IA: Mueve a los enemigos cuando el jugador se mueve
    void updateEnemiesAfterPlayerMove(bool moved);
    void enemyTryAttackFacing(); // IA: Intenta atacar si tiene rango
    void drawEnemies() const;
    void takeDamage(int amount);

    // Gestión de input y estado de juego
    InputDevice lastInput = InputDevice::Keyboard;
    GameState state = GameState::MainMenu;

    MovementMode moveMode = MovementMode::StepByStep;
    float moveCooldown = 0.0f;         // Temporizador para repetición de tecla
    const float MOVE_INTERVAL = 0.12f; // Velocidad de repetición

    // Niveles
    int currentLevel = 1;
    const int maxLevels = 4;

    // Transiciones de Nivel/Juego
    void newRun();
    void newLevel(int level);
    unsigned nextRunSeed() const;
    unsigned seedForLevel(unsigned base, int level) const; // Hash determinista

    // Bucle principal desglosado
    void processInput();
    void update();
    void render();

    // UI Screens
    void renderMainMenu();
    void renderHelpOverlay();
    void renderOptionsMenu(); 
    void handleOptionsInput(); 

    Rectangle uiCenterRect(float w, float h) const;

    // CÁMARA Y VISIBILIDAD
    Camera2D camera{};
    float cameraZoom = 1.0f;
    float shakeTimer = 0.0f; // Temblor de pantalla (Screen Shake)

    void clampCameraToMap(); // Evita ver el vacío negro fuera del mapa
    void centerCameraOnPlayer();

    bool fogEnabled = true;
    int fovTiles = 8;                   // Radio de visión base
    int defaultFovFromViewport() const; // Calcula FOV según tamaño de ventana
    void recomputeFovIfNeeded();        // Raycasting de visión

    // Gestión del Boss
    Boss boss; // Instancia del jefe
    void spawnBoss();
    void updateBoss(float dt);
    void drawBoss() const;
    
    // Helpers colisión Boss (como es grande, necesitamos saber si un punto toca su "área")
    bool isBossCell(int x, int y) const;

    // Lógica de movimiento
    void tryMove(int dx, int dy);          // Intenta mover, gestiona colisiones
    void onSuccessfulStep(int dx, int dy); // Se llama si el movimiento fue válido
    void onExitReached();                  // Jugador pisa la salida

    // Inventario y habilidades pasivas
    bool hasKey = false;    // Necesaria para pasar de nivel
    bool hasShield = false; // Invulnerabilidad temporal
    float shieldTimer = 0.0f;
    bool hasBattery = false; // Vida extra (1-UP)

    // Gafas 3D (Modificadores de FOV)
    float glassesTimer = 0.0f;
    int glassesFovMod = 0;

    // Niveles de armas (0 = sin arma, 1..3 = tiers)
    int swordTier = 0;
    int plasmaTier = 0;

    // Recogida de objetos
    void tryAutoPickup();   // Para la llave
    void tryManualPickup(); // Para consumibles
    void onPickup(const ItemSpawn &it);
    void drawItems() const;
    void drawItemSprite(const ItemSpawn &it) const;

    // UI Menu
    bool showHelp = false;
    int helpScroll = 0;
    std::string helpText;
    int mainMenuSelection = 0;
    int pauseSelection = 0;  
    GameState previousState = GameState::MainMenu;

    void handleMenuInput();
    void handlePlayingInput(float dt);
    void renderPauseMenu();
    void handlePauseInput();

    // Sistema de combate avanzado (Proyectiles & Skills)
    std::vector<Projectile> projectiles;

    float plasmaCooldown = 0.0f;
    int burstShotsLeft = 0; // Para disparo en ráfaga (opcional)
    float burstTimer = 0.0f;

    bool slashActive = false;
    float slashTimer = 0.0f;
    float slashBaseAngle = 0.0f; // Ángulo central del corte
    Color slashColor = WHITE;

    void drawSlash() const; // Función para dibujarlo

    void performMeleeAttack();     // Puñetazo
    void performSwordAttack();     // Espadazo
    void performPlasmaAttack();    // Disparo
    void updateShooters(float dt); // Lógica de disparo de enemigos independiente

    void spawnProjectile(int dmg);
    void updateProjectiles(float dt);
    void drawProjectiles() const;

    // Efectos visuales (Juiciness)
    // Textos flotantes
    std::vector<FloatingText> floatingTexts;
    void spawnFloatingText(Vector2 pos, int value, Color color);
    void updateFloatingTexts(float dt);
    void drawFloatingTexts() const;

    // Partículas
    std::vector<Particle> particles;
    void spawnExplosion(Vector2 pos, int count, Color color);
    void updateParticles(float dt);
    void drawParticles() const;

    // ---------------------------------------------------------------------
    // Ajustes del menú principal y dificultad
    // ---------------------------------------------------------------------
    // Si es true, se muestra el panel de ajustes en el menú principal
    bool showSettingsMenu = false;
    bool showDifficultyWarning = false;
    // Dificultad actual seleccionada. Por defecto empezamos en modo Difícil para
    // mantener el comportamiento original si el usuario no cambia nada.
    Difficulty difficulty = Difficulty::Medium;
    Difficulty pendingDifficulty = Difficulty::Medium;

    // Cambia la dificultad de forma cíclica (Easy → Medium → Hard → Easy)
    void cycleDifficulty();
    // Devuelve un texto descriptivo de la dificultad actual para la UI
    const char *getDifficultyLabel(Difficulty d) const;

    // Variables del Modo Dios
    bool godMode = false;           // ¿Está activo el modo dios?
    bool showGodModeInput = false;  // ¿Mostrando el cuadro de contraseña?
    std::string godModeInput = "";  // Texto que el usuario está escribiendo
    
    // Método auxiliar para activar/desactivar
    void toggleGodMode(bool enable);

    // Sistema de audio (Procedural)
    // Generamos sonidos con código si no hay archivos .wav

    // Tipos de sonido para el generador
    enum SoundType
    {
        SND_HIT,
        SND_EXPLOSION,
        SND_PICKUP,
        SND_POWERUP,
        SND_HURT,
        SND_WIN,
        SND_LOOSE,
        SND_AMBIENT,
        SND_DASH
    };

    Sound generateSound(int type);

    Sound sfxHit{};
    Sound sfxExplosion{};
    Sound sfxPickup{};
    Sound sfxPowerUp{};
    Sound sfxHurt{};
    Sound sfxWin{};
    Sound sfxLoose{};
    Sound sfxDash{};
    Sound sfxAmbient{};

    // Habilidad activa: Dash (Esquiva)
    bool isDashing = false;         // Estado de inmunidad/velocidad
    float dashTimer = 0.0f;         // Duración del dash
    float dashCooldownTimer = 0.0f; // Tiempo de recarga

    const float DASH_DURATION = 0.15f;
    const float DASH_COOLDOWN = 2.0f;
    const int DASH_DISTANCE = 3; // Salta 3 casillas

    Vector2 dashStartPos{}; // Para interpolación suave (Lerp)
    Vector2 dashEndPos{};
};

#endif
