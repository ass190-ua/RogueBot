#include "Game.hpp"
#include "AssetPath.hpp"
#include "GameUtils.hpp"
#include "I18n.hpp"
#include "ResourceManager.hpp"
#include "raylib.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>

static inline unsigned now_seed() {
  return static_cast<unsigned>(time(nullptr));
}

bool gQuitRequested = false;

static Texture2D loadTex(const char *path) {
  const std::string full = assetPath(path);
  Image img = LoadImage(full.c_str());
  if (img.data == nullptr) {
    Image white = GenImageColor(32, 32, WHITE);
    Texture2D t = LoadTextureFromImage(white);
    UnloadImage(white);
    std::cerr << _("[ASSETS] FALLBACK text para: ") << full << "\n";
    return t;
  }
  Texture2D t = LoadTextureFromImage(img);
  UnloadImage(img);
  return t;
}

void ItemSprites::load() {
  if (loaded)
    return;

  // 1. Generación procedural

  // A) Suelo: Losas de cemento limpias
  Image imgFloor = GenImageColor(32, 32, Color{30, 30, 35, 255});

  // Usamos Rectangle para el borde del suelo
  ImageDrawRectangleLines(&imgFloor, Rectangle{0, 0, 32, 32}, 1,
                          Color{15, 15, 20, 255});

  floor = LoadTextureFromImage(imgFloor);
  UnloadImage(imgFloor);

  // B) Pared: Bloque Biselado (Efecto 3D clásico)
  // 1. Base del bloque
  Image imgWall = GenImageColor(32, 32, Color{70, 70, 80, 255});

  // 2. Iluminación (Borde Superior e Izquierdo -> Color Claro)
  ImageDrawRectangle(&imgWall, 0, 0, 32, 2,
                     Color{110, 110, 120, 255}); // Borde Arriba
  ImageDrawRectangle(&imgWall, 0, 0, 2, 32,
                     Color{110, 110, 120, 255}); // Borde Izquierda

  // 3. Sombra (Borde Inferior y Derecho -> Color Oscuro)
  ImageDrawRectangle(&imgWall, 0, 30, 32, 2,
                     Color{40, 40, 50, 255}); // Borde Abajo
  ImageDrawRectangle(&imgWall, 30, 0, 2, 32,
                     Color{40, 40, 50, 255}); // Borde Derecha

  // 4. Detalle interior (Un cuadrado grabado en el centro)
  // Usamos Rectangle para el detalle interior
  ImageDrawRectangleLines(&imgWall, Rectangle{8, 8, 16, 16}, 1,
                          Color{50, 50, 60, 255});

  wall = LoadTextureFromImage(imgWall);
  UnloadImage(imgWall);

  // 2. Carga de sprites (Archivos reales)
  keycard = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_keycard.png");
  shield = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_shield.png");
  pila = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_healthbattery.png");
  glasses = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_glasses.png");
  swordBlue = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_sword_blue.png");
  swordGreen = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_sword_green.png");
  swordRed = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_sword_red.png");
  plasma1 = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_plasma1.png");
  plasma2 = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_plasma2.png");
  battery = ResourceManager::getInstance().getTexture(
      "assets/sprites/items/item_battery.png");

  auto &rm = ResourceManager::getInstance();

  // Pack enemy1
  enemy1 = rm.getTexture("assets/sprites/enemies/enemy1.png");
  enemy1Up1 = rm.getTexture("assets/sprites/enemies/enemy1_up1.png");
  enemy1Up2 = rm.getTexture("assets/sprites/enemies/enemy1_up2.png");
  enemy1Down1 = rm.getTexture("assets/sprites/enemies/enemy1_down1.png");
  enemy1Down2 = rm.getTexture("assets/sprites/enemies/enemy1_down2.png");
  enemy1Left1 = rm.getTexture("assets/sprites/enemies/enemy1_left1.png");
  enemy1Left2 = rm.getTexture("assets/sprites/enemies/enemy1_left2.png");
  enemy1Right1 = rm.getTexture("assets/sprites/enemies/enemy1_right1.png");
  enemy1Right2 = rm.getTexture("assets/sprites/enemies/enemy1_right2.png");

  // Pack enemy2
  enemy2 = rm.getTexture("assets/sprites/enemies/enemy2.png");
  enemy2Up1 = rm.getTexture("assets/sprites/enemies/enemy2_up1.png");
  enemy2Up2 = rm.getTexture("assets/sprites/enemies/enemy2_up2.png");
  enemy2Down1 = rm.getTexture("assets/sprites/enemies/enemy2_down1.png");
  enemy2Down2 = rm.getTexture("assets/sprites/enemies/enemy2_down2.png");
  enemy2Left1 = rm.getTexture("assets/sprites/enemies/enemy2_left1.png");
  enemy2Left2 = rm.getTexture("assets/sprites/enemies/enemy2_left2.png");
  enemy2Right1 = rm.getTexture("assets/sprites/enemies/enemy2_right1.png");
  enemy2Right2 = rm.getTexture("assets/sprites/enemies/enemy2_right2.png");

  // Compatibilidad con el render actual (por ahora = enemy1 frame1)
  enemy = enemy1;
  enemyUp = enemy1Up1;
  enemyDown = enemy1Down1;
  enemyLeft = enemy1Left1;
  enemyRight = enemy1Right1;

  // Carga del Boss
  bossDownIdle = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_down_idle.png");
  bossUpIdle = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_up_idle.png");
  bossLeftIdle = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_left_idle.png");
  bossRightIdle = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_right_idle.png");

  bossDownWalk1 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_down_walk1.png");
  bossDownWalk2 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_down_walk2.png");
  bossUpWalk1 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_up_walk1.png");
  bossUpWalk2 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_up_walk2.png");
  bossLeftWalk1 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_left_walk1.png");
  bossLeftWalk2 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_left_walk2.png");
  bossRightWalk1 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_right_walk1.png");
  bossRightWalk2 = ResourceManager::getInstance().getTexture(
      "assets/sprites/boss/boss_right_walk2.png");

  loaded = true;
}

void ItemSprites::unload() {
  if (!loaded)
    return;

  // Solo descargamos lo que creamos manualmente (procedural)
  UnloadTexture(wall);
  UnloadTexture(floor);

  loaded = false;
}

Game::Game(unsigned seed) : fixedSeed(seed) {
  SetConfigFlags(FLAG_FULLSCREEN_MODE);
  InitWindow(0, 0, _("RogueBot"));
  SetExitKey(KEY_NULL);

  // 1. Iniciar audio
  InitAudioDevice();
  audioVolume = 0.2f;
  SetMasterVolume(audioVolume); // Volumen general al 20%
  loadSettings();

  // 2. Generar sonidos
  sfxHit = generateSound(SND_HIT);
  sfxExplosion = generateSound(SND_EXPLOSION);
  sfxPickup = generateSound(SND_PICKUP);
  sfxPowerUp = generateSound(SND_POWERUP);
  sfxHurt = generateSound(SND_HURT);
  sfxWin = generateSound(SND_WIN);
  sfxLoose = generateSound(SND_LOOSE);
  sfxAmbient = generateSound(SND_AMBIENT);
  sfxDash = generateSound(SND_DASH);

  player.load("assets/sprites/player");
  itemSprites.load();

  screenW = GetScreenWidth();
  screenH = GetScreenHeight();

  camera.target = {0.0f, 0.0f};
  camera.offset = {(float)screenW / 2, (float)screenH / 2};
  camera.rotation = 0.0f;
  camera.zoom = cameraZoom;

  SetTargetFPS(60);

  state = GameState::MainMenu;
  mainMenuSelection = 0;
}

unsigned Game::nextRunSeed() const {
  return fixedSeed > 0 ? fixedSeed : now_seed();
}

unsigned Game::seedForLevel(unsigned base, int level) const {
  const unsigned MIX = 0x9E3779B9u;
  return base ^ (MIX * static_cast<unsigned>(level));
}

std::string Game::settingsPath() {
  const char *home = std::getenv("HOME");
  if (!home)
    return "roguebot_settings.cfg";

  return std::string(home) + "/.config/roguebot/settings.cfg";
}

void Game::applyCurrentLanguage() {
  const char *res = nullptr;

  // 1. Intentamos nombres estándar POSIX (Linux/Mac)
  if (language == Language::EN) {
    res = setlocale(LC_ALL, "en_GB.UTF-8");
    if (!res)
      res = setlocale(LC_ALL, "en_GB.utf8");
  } else // Caso ES
  {
    res = setlocale(LC_ALL, "es_ES.UTF-8");
    if (!res)
      res = setlocale(LC_ALL, "es_ES.utf8");
  }

  // 2. Fallback para Windows (si falló lo anterior)
  if (!res) {
    if (language == Language::EN) {
      res = setlocale(LC_ALL, "English_United Kingdom");
      if (!res)
        res = setlocale(LC_ALL, "English");
    } else {
      res = setlocale(LC_ALL, "Spanish_Spain");
      if (!res)
        res = setlocale(LC_ALL, "Spanish");
    }
  }

  // FIX WINDOWS: Forzar variables de entorno
#ifdef _WIN32
  if (language == Language::ES) {
    _putenv("LANGUAGE=es_ES");
    _putenv("LC_ALL=es_ES");
  } else {
    _putenv("LANGUAGE=en_GB");
    _putenv("LC_ALL=en_GB");
  }
#endif

  // Debug opcional
  if (res)
    std::cout << "[I18N] Idioma aplicado: " << res << "\n";

  // Recargar Gettext
  bind_textdomain_codeset("roguebot", "UTF-8");
  textdomain("roguebot");

  // Actualizar el título de la ventana al momento
  SetWindowTitle(_("RogueBot"));
}

void Game::loadSettings() {
  const std::string path = settingsPath();
  std::ifstream f(path);
  // Si no hay fichero, aplicamos idioma por defecto (ES)
  if (!f.is_open()) {
    applyCurrentLanguage();
    return;
  }

  std::string line;
  while (std::getline(f, line)) {
    if (line.empty())
      continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;

    const std::string key = line.substr(0, eq);
    const std::string val = line.substr(eq + 1);

    if (key == "language") {
      if (val == "en")
        language = Language::EN;
      else if (val == "es")
        language = Language::ES;

      pendingLanguage = language;

      applyCurrentLanguage();
    } else if (key == "difficulty") {
      int d = std::atoi(val.c_str());
      if (d >= 0 && d <= 2) {
        difficulty = (Difficulty)d;
        pendingDifficulty = difficulty;
      }
    } else if (key == "volume") {
      int volPct = std::atoi(val.c_str());
      volPct = std::clamp(volPct, 0, 100);
      audioVolume = (float)volPct / 100.0f;
      SetMasterVolume(audioVolume);
    }
  }
}

void Game::saveSettings() const {
  const std::string path = settingsPath();

  try {
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());
  } catch (...) {
    // Si falla crear carpetas, no abortamos
  }

  std::ofstream f(path);
  if (!f.is_open())
    return;

  const char *lang = (language == Language::EN) ? "en" : "es";
  f << "language=" << lang << "\n";
  f << "difficulty=" << (int)difficulty << "\n";
  int volPct = (int)std::lround(audioVolume * 100.0f);
  volPct = std::clamp(volPct, 0, 100);
  f << "volume=" << volPct << "\n";
}

void Game::newRun() {
  runSeed = nextRunSeed();
  std::cout << _("[Run] Seed base del run: ") << runSeed << "\n";

  // Reiniciar estado básico
  currentLevel = 1;
  state = GameState::Playing;
  moveCooldown = 0.0f;
  hp = 10;
  hpMax = 10;

  // Reiniciar Inventario y Power-Ups
  hasKey = false;

  // Escudo
  hasShield = false;
  shieldTimer = 0.0f; // Resetear el tiempo

  // Batería (Vida extra)
  hasBattery = false;

  // Gafas 3D (Visión)
  glassesTimer = 0.0f; // Si no es 0, el juego cree que siguen activas
  glassesFovMod = 0;   // Resetear el modificador de visión extra

  // Armas
  swordTier = 0;
  plasmaTier = 0;

  // Reiniciar modo Dios (Seguridad)
  godMode = false;
  showGodModeInput = false;
  map.setRevealAll(false); // Apagar la luz del modo dios

  // Reiniciar contexto de mejoras entre niveles
  runCtx.espadaMejorasObtenidas = 0;
  runCtx.plasmaMejorasObtenidas = 0;

  // Reiniciar lógica de ataque
  gAttack = AttackRuntime{};
  gAttack.frontOnly = true;

  // Limpiar proyectiles y entidades
  projectiles.clear();
  floatingTexts.clear(); // Limpiar números flotantes viejos
  particles.clear();     // Limpiar explosiones viejas

  // Reset boss
  boss = Boss{};

  newLevel(currentLevel);
}

void Game::newLevel(int level) {
  // Lógica especial para Nivel 4 (Boss)

  projectiles.clear(); // Limpiar balas
  enemies.clear();     // Limpiar enemigos anteriores
  items.clear();       // Limpiar items

  levelSeed = seedForLevel(runSeed, level);
  rng = std::mt19937(levelSeed);

  if (level == maxLevels) { // Nivel Final
    std::cout << _("[Level] FINAL BOSS LEVEL Initializing...\n");

    // 1. Generar Arena a pantalla completa
    // Calculamos cuántos tiles caben en la pantalla
    int arenaW = screenW / tileSize;
    int arenaH = screenH / tileSize;

    // Generamos la arena con esas dimensiones
    map.generateBossArena(arenaW, arenaH);

    // 2. Configurar Visibilidad
    map.setFogEnabled(false);

    // 3. Posicionar Jugador (Abajo centro)
    px = arenaW / 2;
    py = arenaH - 4; // Un poco separado del borde
    player.setGridPos(px, py);

    // 4. Centrar cámara (Fija en el centro de la pantalla)
    // Al ser tamaño exacto de pantalla, el centro es screenW/2, screenH/2
    camera.target = {(float)screenW / 2.0f, (float)screenH / 2.0f};

    fovTiles = 100;
    map.computeVisibility(px, py, fovTiles);

    hasKey = false;
    spawnBoss();
  } else {
    // Niveles normales (1, 2, 3)
    const float WORLD_SCALE = 1.2f;
    int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
    int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

    std::cout << _("[Level] ") << level << "/" << maxLevels
              << _(" (seed nivel: ") << levelSeed << ")\n";

    map.generate(tilesX, tilesY, levelSeed);
    map.setFogEnabled(true); // Asegurar niebla activada

    fovTiles = defaultFovFromViewport();

    auto r = map.firstRoom();
    if (r.w > 0 && r.h > 0) {
      px = r.x + r.w / 2;
      py = r.y + r.h / 2;
    } else {
      px = tilesX / 2;
      py = tilesY / 2;
    }

    player.setGridPos(px, py);
    map.computeVisibility(px, py, getFovRadius());

    Vector2 playerCenterPx = {px * (float)tileSize + tileSize / 2.0f,
                              py * (float)tileSize + tileSize / 2.0f};
    camera.target = playerCenterPx;
    clampCameraToMap();

    hasKey = false;
    spawnEnemiesForLevel();

    // Generar items
    std::vector<IVec2> enemyTiles;
    for (const auto &e : enemies)
      enemyTiles.push_back({e.getX(), e.getY()});
    auto isWalkable = [&](int x, int y) { return map.isWalkable(x, y); };
    IVec2 spawnTile{px, py};
    auto [exitX, exitY] = map.findExitTile();
    IVec2 exitTile{exitX, exitY};

    items =
        ItemSpawner::generate(map.width(), map.height(), isWalkable, spawnTile,
                              exitTile, enemyTiles, level, rng, runCtx);
  }
}

// Spawn del Boss
void Game::spawnBoss() {
  boss.active = true;
  boss.awakened = false; // Empieza dormido

  // Posición del Boss: Centro arriba
  boss.x = map.width() / 2;
  boss.y = 4;

  // Guardamos dónde está el jugador al entrar
  boss.playerStartX = px;
  boss.playerStartY = py;

  // Configurar Vida según Dificultad
  int baseHp = 800;
  float diffMult = (difficulty == Difficulty::Easy)   ? 0.5f
                   : (difficulty == Difficulty::Hard) ? 1.5f
                                                      : 1.0f;

  boss.maxHp = (int)(baseHp * diffMult);
  boss.hp = boss.maxHp;

  boss.phase = 1;
  boss.actionCooldown = 2.0f;
  boss.moveTimer = 1.0f;
  boss.facing = Boss::DOWN;

  std::cout << _("[BOSS] SPAWNED. Waiting for movement...\n");
}

void Game::updateBoss(float dt) {
  // 0. Verificar muerte
  if (boss.hp <= 0) {
    boss.active = false;
    spawnExplosion({(float)boss.x * tileSize, (float)boss.y * tileSize}, 200,
                   GOLD);
    state = GameState::Victory;
    PlaySound(sfxWin);
    return;
  }

  boss.animTime += dt * 3.0f;
  boss.flashTimer = std::max(0.0f, boss.flashTimer - dt);

  // 1. Mecánica de despertar
  // Si tu posición (px, py) es distinta a la inicial.
  if (!boss.awakened) {
    if (px != boss.playerStartX || py != boss.playerStartY) {
      boss.awakened = true;
      PlaySound(sfxExplosion); // Rugido
      spawnFloatingText(
          {(float)boss.x * tileSize, (float)boss.y * tileSize - 20}, 0, RED);
      std::cout << _("[BOSS] AWAKENED! TIEMBLA MORTAL!\n");
    } else {
      // Si el jugador dispara al boss dormido, lo despierta también
      if (boss.hp < boss.maxHp)
        boss.awakened = true;
      else
        return; // Sigue durmiendo
    }
  }

  // 2. Configuración de fases
  float hpPercent = (float)boss.hp / (float)boss.maxHp;

  float phaseMoveDelay = 1.0f;
  float phaseFireDelay = 2.0f;
  int phaseDmg = 1;

  // Fase 1 (100-75%)
  if (hpPercent > 0.75f) {
    boss.phase = 1;
    phaseMoveDelay = 0.8f;
    phaseFireDelay = 2.0f;
    phaseDmg = 1;
  }
  // Fase 2 (75-50%)
  else if (hpPercent > 0.50f) {
    boss.phase = 2;
    phaseMoveDelay = 0.6f;
    phaseFireDelay = 1.5f;
    phaseDmg = 2;
  }
  // Fase 3 (50-25%)
  else if (hpPercent > 0.25f) {
    boss.phase = 3;
    phaseMoveDelay = 0.4f;
    phaseFireDelay = 1.0f;
    phaseDmg = 3;
  }
  // Fase 4 (<25%)
  else {
    boss.phase = 4;
    phaseMoveDelay = 0.25f;
    phaseFireDelay = 0.6f;
    phaseDmg = 4;
    if (GetRandomValue(0, 10) < 2)
      spawnExplosion({(float)boss.x * tileSize, (float)boss.y * tileSize}, 1,
                     RED);
  }

  // Ajustes por Dificultad
  float diffSpeedMult = 1.0f, diffFireMult = 1.0f;
  int diffDmgBonus = 0;
  if (difficulty == Difficulty::Easy) {
    diffSpeedMult = 1.3f;
    diffFireMult = 1.5f;
  }
  if (difficulty == Difficulty::Hard) {
    diffSpeedMult = 0.8f;
    diffFireMult = 0.7f;
    diffDmgBonus = 1;
  }

  float finalMoveDelay = phaseMoveDelay * diffSpeedMult;
  float finalFireDelay = phaseFireDelay * diffFireMult;
  int finalDmg = phaseDmg + diffDmgBonus;

  // 3. Movimiento
  boss.moveTimer -= dt;
  if (boss.moveTimer <= 0.0f) {
    boss.moveTimer = finalMoveDelay;
    int dx = px - boss.x, dy = py - boss.y;

    if (std::abs(dx) > 1 || std::abs(dy) > 1) {
      int stepX = 0, stepY = 0;
      if (std::abs(dx) >= std::abs(dy))
        stepX = (dx > 0) ? 1 : -1;
      else
        stepY = (dy > 0) ? 1 : -1;

      if (map.isWalkable(boss.x + stepX, boss.y + stepY)) {
        boss.x += stepX;
        boss.y += stepY;
      } else {
        if (stepX != 0)
          stepY = (dy > 0) ? 1 : -1;
        else
          stepX = (dx > 0) ? 1 : -1;
        if (map.isWalkable(boss.x + stepX, boss.y + stepY)) {
          boss.x += stepX;
          boss.y += stepY;
        }
      }
    }
    if (std::abs(dx) > std::abs(dy))
      boss.facing = (dx > 0) ? Boss::RIGHT : Boss::LEFT;
    else
      boss.facing = (dy > 0) ? Boss::DOWN : Boss::UP;
  }

  // 4. Ataque (Disparo)
  boss.actionCooldown -= dt;
  if (boss.actionCooldown <= 0.0f) {
    boss.actionCooldown = finalFireDelay;

    auto shoot = [&](float vx, float vy) {
      Projectile p;
      p.isEnemy = true;
      p.damage = finalDmg;
      p.pos = {(float)boss.x * tileSize + tileSize / 2.0f,
               (float)boss.y * tileSize + tileSize / 2.0f};
      p.maxDistance = 1000.0f;
      p.vel = {vx, vy};
      projectiles.push_back(p);
    };

    float speed = 250.0f + (boss.phase * 50.0f);
    float vx = 0, vy = 0;
    switch (boss.facing) {
    case Boss::UP:
      vy = -speed;
      break;
    case Boss::DOWN:
      vy = speed;
      break;
    case Boss::LEFT:
      vx = -speed;
      break;
    case Boss::RIGHT:
      vx = speed;
      break;
    }

    shoot(vx, vy);
    if (boss.phase >= 3)
      shoot(-vx, -vy);
    if (boss.phase == 4) {
      shoot(vy, vx);
      shoot(-vy, -vx);
      PlaySound(sfxExplosion);
    }
  }

  // 5. Colisión Melee
  if (std::abs(px - boss.x) <= 1 && std::abs(py - boss.y) <= 1) {
    if (damageCooldown <= 0.0f) {
      takeDamage(finalDmg + 1);
      damageCooldown = DAMAGE_COOLDOWN;
      int pushX = (px > boss.x) ? 2 : (px < boss.x ? -2 : 0);
      int pushY = (py > boss.y) ? 2 : (py < boss.y ? -2 : 0);
      if (map.isWalkable(px + pushX, py + pushY)) {
        px += pushX;
        py += pushY;
      } else if (map.isWalkable(px + pushX / 2, py + pushY / 2)) {
        px += pushX / 2;
        py += pushY / 2;
      }
      spawnFloatingText({(float)px * tileSize, (float)py * tileSize},
                        finalDmg + 1, RED);
    }
  }
}

// Implementación del toggle
void Game::toggleGodMode(bool enable) {
  godMode = enable;

  // Le decimos al mapa que lo revele todo (o vuelva a la normalidad)
  map.setRevealAll(godMode);

  if (godMode) {
    std::cout << _("[GOD MODE] ACTIVADO - IDDQD\n");

    PlaySound(sfxPowerUp);
    // Mensaje visual (puedes usar tu sistema de texto flotante)
    spawnFloatingText({(float)px * tileSize, (float)py * tileSize}, 9999,
                      GOLD); // Truco visual

    // Opcional: Curar al jugador
    hp = hpMax;
  } else {
    std::cout << _("[GOD MODE] DESACTIVADO\n");
    // Al desactivar, forzamos un recálculo de visión para que
    // la niebla vuelva a aparecer correctamente alrededor del jugador.
    // Sonido de error/apagado (ej. Hurt o Loose)
    PlaySound(sfxLoose);
    map.computeVisibility(px, py, getFovRadius());
  }
}

void Game::run() {
  while (!WindowShouldClose() && !gQuitRequested) {
    processInput();
    update();
    render();

    // Gestión de música ambiente
    // Solo suena si estamos jugando. Si salimos al menú, se calla.
    if (state == GameState::Playing || state == GameState::Paused) {
      if (!IsSoundPlaying(sfxAmbient)) {
        PlaySound(sfxAmbient);
      }

      // Si está en pausa, pausamos el stream de audio
      if (state == GameState::Paused)
        ResumeSound(sfxAmbient);
      // Mejor lógica:
      if (state == GameState::Paused) {
        // Silencio total:
        if (IsSoundPlaying(sfxAmbient))
          PauseSound(sfxAmbient);
      } else {
        if (!IsSoundPlaying(sfxAmbient))
          ResumeSound(sfxAmbient);
      }
    } else {
      // Menú principal / GameOver / Victory
      if (IsSoundPlaying(sfxAmbient)) {
        StopSound(sfxAmbient);
      }
    }
  }

  // Descargar sonidos
  UnloadSound(sfxHit);
  UnloadSound(sfxExplosion);
  UnloadSound(sfxPickup);
  UnloadSound(sfxPowerUp);
  UnloadSound(sfxHurt);
  UnloadSound(sfxWin);
  UnloadSound(sfxLoose);
  UnloadSound(sfxAmbient);
  UnloadSound(sfxDash);

  player.unload();
  itemSprites.unload();
  ResourceManager::getInstance().clear();
  CloseAudioDevice();
  CloseWindow();
}

void Game::update() {
  if (state != GameState::Playing)
    return;
  if (showGodModeInput)
    return;

  float dt = GetFrameTime();

  // Dash
  if (isDashing) {
    dashTimer -= dt;

    // Efectos visuales
    if (GetRandomValue(0, 10) < 8) {
      float t = 1.0f - (dashTimer / DASH_DURATION);
      Vector2 lerpPos = {dashStartPos.x + (dashEndPos.x - dashStartPos.x) * t,
                         dashStartPos.y + (dashEndPos.y - dashStartPos.y) * t};
      // spawnExplosion(lerpPos, 1, SKYBLUE);
    }

    if (dashTimer <= 0.0f)
      isDashing = false;

    // Solo movemos la cámara si NO estamos en el nivel del boss.
    // Si es el boss, la cámara se queda quieta.
    if (currentLevel != maxLevels) {
      Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                         py * (float)tileSize + tileSize / 2.0f};
      camera.target.x = desired.x;
      camera.target.y = desired.y;
      clampCameraToMap();
    }

    return;
  }

  dashCooldownTimer = std::max(0.0f, dashCooldownTimer - dt);
  tryAutoPickup();

  if (hasShield) {
    shieldTimer -= dt;
    if (shieldTimer <= 0.0f)
      hasShield = false;
  }
  if (glassesTimer > 0.0f) {
    glassesTimer -= dt;
    if (glassesTimer <= 0.0f)
      recomputeFovIfNeeded();
  }
  if (slashActive) {
    slashTimer -= dt;
    if (slashTimer <= 0.0f)
      slashActive = false;
  }

  for (auto &t : enemyFlashTimer)
    t = std::max(0.0f, t - dt);
  for (auto &e : enemies)
    e.updateAnimation(dt);

  if (boss.active) {
    updateBoss(dt);
  } else {
    updateShooters(dt);
  }

  updateProjectiles(dt);
  updateFloatingTexts(dt);
  updateParticles(dt);

  if (currentLevel < maxLevels && map.at(px, py) == EXIT) {
    if (hasKey)
      onExitReached();
  }

  // Cámara Suave (Solo en niveles normales)
  if (currentLevel != maxLevels) {
    auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };
    Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                       py * (float)tileSize + tileSize / 2.0f};
    float smooth = 10.0f * dt;
    camera.target.x = Lerp(camera.target.x, desired.x, smooth);
    camera.target.y = Lerp(camera.target.y, desired.y, smooth);
    clampCameraToMap();
  }

  if (shakeTimer > 0.0f) {
    shakeTimer -= dt;
    float intensity = 5.0f;
    camera.target.x += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
    camera.target.y += (float)GetRandomValue(-100, 100) / 100.0f * intensity;
  }

  if (hp <= 0) {
    if (hasBattery) {
      hasBattery = false;
      hp = hpMax / 2;
      if (hp < 1)
        hp = 1;
      std::cout << _("[Bateria] Resucitado.\n");
      damageCooldown = 2.0f;
      shakeTimer = 0.5f;
      PlaySound(sfxPowerUp);
    } else {
      state = GameState::GameOver;
      gAttack.swinging = false;
      gAttack.lastTiles.clear();
      PlaySound(sfxLoose);
    }
    return;
  }

  damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
  for (auto &cd : enemyAtkCD)
    cd = std::max(0.0f, cd - dt);
  for (auto &cd : enemyShootCD)
    cd = std::max(0.0f, cd - dt);
}

void Game::onExitReached() {
  // Si completamos nivel 3, vamos al 4 (Boss)
  if (currentLevel < maxLevels) {
    currentLevel++;
    newLevel(currentLevel);
  }
}

// Cambia la dificultad actual de forma cíclica. Al llegar al final vuelve al
// primero.
void Game::cycleDifficulty() {
  if (difficulty == Difficulty::Easy) {
    difficulty = Difficulty::Medium;
  } else if (difficulty == Difficulty::Medium) {
    difficulty = Difficulty::Hard;
  } else {
    difficulty = Difficulty::Easy;
  }
}

void Game::cycleLanguage() {
  // Cambiamos el idioma
  if (language == Language::ES) {
    language = Language::EN;
    pendingLanguage = Language::EN;
  } else {
    language = Language::ES;
    pendingLanguage = Language::ES;
  }

  applyCurrentLanguage();
}

// Devuelve una cadena estática con el nombre de la dificultad, usada en el
// menú.
const char *Game::getDifficultyLabel(Difficulty d) const {
  switch (d) {
  case Difficulty::Easy:
    return _("Dificultad: Fácil");
  case Difficulty::Medium:
    return _("Dificultad: Normal");
  default:
    return _("Dificultad: Difícil");
  }
}

std::string Game::getLanguageLabel() const {
  std::string prefix = _("Idioma: "); // nueva cadena en el .po
  std::string name;
  switch (pendingLanguage) {
  case Language::ES:
    name = _("Español"); // cadena traducible
    break;
  case Language::EN:
    name = _("English");
    break;
    // más idiomas...
  }
  return prefix + name;
}

std::string Game::getVolumeLabel() const {
  int pct = (int)std::round(audioVolume * 100.0f);
  pct = std::clamp(pct, 0, 100);
  return _("Volumen: ") + std::to_string(pct) + "%";
}

void Game::drawBoss() const {
  if (!boss.active)
    return;

  Texture2D tex = itemSprites.bossDownIdle;

  // Selección de sprite (Igual que antes)
  switch (boss.facing) {
  case Boss::UP:
    tex = itemSprites.bossUpIdle;
    break;
  case Boss::DOWN:
    tex = itemSprites.bossDownIdle;
    break;
  case Boss::LEFT:
    tex = itemSprites.bossLeftIdle;
    break;
  case Boss::RIGHT:
    tex = itemSprites.bossRightIdle;
    break;
  }

  // 1. Configuración de escala
  float scale = 3.0f;
  float baseW = (float)tex.width * scale;
  float baseH = (float)tex.height * scale;

  // 2. Factor de respiración (Squash & Stretch)
  // Usamos boss.animTime. El 0.03f define qué tanto se estira (3%).
  // El Player usa 0.05f, el Boss al ser grande se deforma menos para no parecer
  // de gelatina.
  float breathe = 1.0f + sinf(boss.animTime) * 0.03f;

  // 3. Posición en pantalla
  // Calculamos el centro de la casilla lógica del Boss
  Vector2 centerPos = {(float)boss.x * tileSize + tileSize / 2.0f,
                       (float)boss.y * tileSize + tileSize / 2.0f};

  // 4. Rectángulos de Destino y Origen
  // IMPORTANTE: Para que respire "desde el suelo", el ancla (origin) debe estar
  // en los pies.

  // El destino se dibuja en la posición de los pies del Boss
  Rectangle dest = {
      centerPos.x,                  // Centro X
      centerPos.y + (baseH / 2.0f), // Pies Y (ajustamos porque centerPos está
                                    // en el centro del tile)
      baseW,                        // Ancho fijo
      baseH * breathe               // Alto variable (Aquí ocurre la magia)
  };

  // El origen es el punto dentro de la textura que coincidirá con (dest.x,
  // dest.y) Lo ponemos abajo al centro.
  Vector2 origin = {
      baseW / 2.0f,
      baseH // Los pies de la textura
  };

  // Dibujado
  Color tint = WHITE;
  if (boss.flashTimer > 0.0f)
    tint = RED;
  else if (boss.phase == 2)
    tint = {255, 200, 200, 255};
  else if (boss.phase == 3)
    tint = {255, 100, 100, 255};

  DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, dest, origin,
                 0.0f, tint);

  // Barra de vida (Ajustada para seguir la respiración o quedarse fija)
  // La dibujamos fija relativa al centro lógico para que no "baile" con la
  // respiración
  float barW = 100.0f;
  float barH = 10.0f;
  float barX = centerPos.x - barW / 2;
  // Ponemos la barra arriba de la cabeza (considerando la altura base)
  float barY = centerPos.y - (baseH / 2.0f) - 25.0f;

  DrawRectangle(barX, barY, barW, barH, BLACK);
  float fill = ((float)boss.hp / (float)boss.maxHp) * barW;
  DrawRectangle(barX, barY, fill, barH, PURPLE);
  DrawRectangleLines(barX, barY, barW, barH, WHITE);
}

const char *Game::getInputText(const char *kb, const char *gp) const {
  return (lastInput == InputDevice::Gamepad) ? gp : kb;
}

void Game::startTutorial() {
  std::cout << _("[TUTORIAL] Iniciando Instalacion de Entrenamiento...\n");
  state = GameState::Tutorial;
  tutorialStep = TutorialStep::Intro;
  tutorialTimer = 4.0f;
  tutorialFlag = false;
  tutorialMenuSelection = 0;

  hpMax = 8;
  hp = 4;
  hasKey = false;
  hasShield = false;
  hasBattery = false;
  swordTier = 0;
  plasmaTier = 0;

  enemies.clear();
  items.clear();
  projectiles.clear();
  floatingTexts.clear();
  particles.clear();

  enemyHP.clear();
  enemyMaxHP.clear();
  enemyAtkCD.clear();
  enemyShootCD.clear();
  enemyFlashTimer.clear();
  enemyFacing.clear();

  // Generar mapa
  map.generateTutorialMap(50, 25);
  map.setFogEnabled(false);

  // Posición Jugador (Centro del Lobby)
  px = 5;
  py = map.height() / 2;
  player.setGridPos(px, py);

  centerCameraOnPlayer();
}

void Game::updateTutorial(float dt) {
  // 1. Lógica core
  player.update(dt, false);
  updateFloatingTexts(dt);
  updateParticles(dt);
  updateProjectiles(dt);
  tryAutoPickup();

  // 2. Físicas
  if (slashActive) {
    slashTimer -= dt;
    if (slashTimer <= 0.0f)
      slashActive = false;
  }
  if (isDashing) {
    dashTimer -= dt;
    if (dashTimer <= 0.0f)
      isDashing = false;
    if ((int)(dashTimer * 20) % 2 == 0) {
      Vector2 centerPos = {(float)px * tileSize + tileSize / 2.0f,
                           (float)py * tileSize + tileSize / 2.0f};
      spawnExplosion(centerPos, 1, SKYBLUE);
    }
  }
  dashCooldownTimer = std::max(0.0f, dashCooldownTimer - dt);
  damageCooldown = std::max(0.0f, damageCooldown - dt);
  if (hasShield)
    shieldTimer -= dt;
  if (glassesTimer > 0.0f)
    glassesTimer -= dt;

  // Centro vertical del mapa
  int cy = map.height() / 2;

  switch (tutorialStep) {
  case TutorialStep::Intro:
    tutorialTimer -= dt;
    if (tutorialTimer <= 0)
      tutorialStep = TutorialStep::Movement;
    break;

  case TutorialStep::Movement:
    // Esperamos a que salga del Lobby (x > 8)
    if (px > 8) {
      tutorialStep = TutorialStep::Dash;
      tutorialTimer = 1.0f;
    }
    break;

  case TutorialStep::Dash:
    if (tutorialTimer > 0)
      tutorialTimer -= dt;
    else if (isDashing) {
      tutorialStep = TutorialStep::MoveMode;
      tutorialTimer = 1.0f;
    }
    break;

  case TutorialStep::MoveMode:
    if (tutorialTimer > 0)
      tutorialTimer -= dt;
    else {
      static MovementMode lastMode = moveMode;
      if (moveMode != lastMode) {
        lastMode = moveMode;
        tutorialStep = TutorialStep::CameraZoom;
      }
    }
    break;

  case TutorialStep::CameraZoom:
    if (std::abs(cameraZoom - 1.0f) > 0.2f)
      tutorialStep = TutorialStep::CameraReset;
    break;

  case TutorialStep::CameraReset:
    if (std::abs(cameraZoom - 1.0f) < 0.05f && camera.rotation == 0.0f) {
      tutorialStep = TutorialStep::ItemPilaBuena;
      ItemSpawn it;
      it.type = ItemType::PilaBuena;
      it.tile = {11, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemPilaBuena:
    if (items.empty()) {
      PlaySound(sfxPickup);
      tutorialStep = TutorialStep::ItemPilaMala;
      tutorialTimer = 4.0f;
      ItemSpawn it;
      it.type = ItemType::PilaMala;
      it.tile = {13, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemPilaMala:
    if (items.empty()) {
      PlaySound(sfxHurt);
      tutorialStep = TutorialStep::ItemEscudo;
      tutorialTimer = 2.0f;
      ItemSpawn it;
      it.type = ItemType::Escudo;
      it.tile = {15, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemEscudo:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      hasShield = true;
      shieldTimer = 3.0f;
      tutorialStep = TutorialStep::PreGafas;
      tutorialTimer = 3.0f;
      map.setFogEnabled(true);
      map.setRevealAll(false);
      map.computeVisibility(px, py, getFovRadius());
    }
    break;

  case TutorialStep::PreGafas:
    tutorialTimer -= dt;
    if (tutorialTimer <= 0) {
      tutorialStep = TutorialStep::ItemGafasBuenas;
      ItemSpawn it;
      it.type = ItemType::Gafas3DBuenas;
      it.tile = {17, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemGafasBuenas:
    if (items.empty()) {
      PlaySound(sfxPickup);
      glassesFovMod = 5;
      recomputeFovIfNeeded();
      tutorialStep = TutorialStep::ItemGafasMalas;
      // x=19
      ItemSpawn it;
      it.type = ItemType::Gafas3DMalas;
      it.tile = {19, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemGafasMalas:
    if (items.empty()) {
      PlaySound(sfxLoose);
      glassesFovMod = -5;
      recomputeFovIfNeeded();
      tutorialStep = TutorialStep::BadGlassesEffect;
      tutorialTimer = 3.0f;
    }
    break;

  case TutorialStep::BadGlassesEffect:
    tutorialTimer -= dt;
    if (tutorialTimer <= 0.0f) {
      tutorialStep = TutorialStep::PostGafas;
      glassesFovMod = 0;
      map.setFogEnabled(false);
      map.setRevealAll(true);
      recomputeFovIfNeeded();
    }
    break;

  case TutorialStep::PostGafas:
    tutorialStep = TutorialStep::ItemVidaExtra;
    {
      ItemSpawn it;
      it.type = ItemType::BateriaVidaExtra;
      it.tile = {21, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);
    }
    break;

  case TutorialStep::ItemVidaExtra:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::SwordT1;
      tutorialTimer = 1.0f;
      // Zona de Armería: x=23
      ItemSpawn it;
      it.type = ItemType::EspadaPickup;
      it.tile = {23, cy};
      it.nivel = 1;
      it.tierSugerido = 1;
      items.push_back(it);
    }
    break;

  // Secuencia Rápida de Armas (x=25, 27, 29, 31)
  case TutorialStep::SwordT1:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::SwordT2;
      ItemSpawn it;
      it.type = ItemType::EspadaPickup;
      it.tile = {25, cy};
      it.nivel = 1;
      it.tierSugerido = 2;
      items.push_back(it);
    }
    break;
  case TutorialStep::SwordT2:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::SwordT3;
      ItemSpawn it;
      it.type = ItemType::EspadaPickup;
      it.tile = {27, cy};
      it.nivel = 1;
      it.tierSugerido = 3;
      items.push_back(it);
    }
    break;
  case TutorialStep::SwordT3:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::PlasmaT1;
      ItemSpawn it;
      it.type = ItemType::PistolaPlasmaPickup;
      it.tile = {29, cy};
      it.nivel = 1;
      it.tierSugerido = 1;
      items.push_back(it);
    }
    break;
  case TutorialStep::PlasmaT1:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::PlasmaT2;
      ItemSpawn it;
      it.type = ItemType::PistolaPlasmaPickup;
      it.tile = {31, cy};
      it.nivel = 1;
      it.tierSugerido = 2;
      items.push_back(it);
    }
    break;

  case TutorialStep::PlasmaT2:
    if (items.empty()) {
      PlaySound(sfxPowerUp);
      tutorialStep = TutorialStep::Combat;

      // Enemigo en el centro de la Arena (x=42)
      Enemy dummy;
      dummy.setPos(42, cy);
      enemies.push_back(dummy);

      enemyHP.push_back(60);
      enemyMaxHP.push_back(60);
      enemyAtkCD.push_back(0);
      enemyShootCD.push_back(0);
      enemyFlashTimer.push_back(0);
      enemyFacing.push_back(EnemyFacing::Down);
    }
    break;

  case TutorialStep::Combat:
    if (enemies.empty()) {
      tutorialStep = TutorialStep::Exit;
      // Llave donde murió el enemigo
      ItemSpawn it;
      it.type = ItemType::LlaveMaestra;
      it.tile = {42, cy};
      it.nivel = 1;
      it.tierSugerido = 0;
      items.push_back(it);

      map.setTile(46, cy, EXIT);
      PlaySound(sfxWin);
    }
    break;

  case TutorialStep::Exit:
    if (hasKey && px == 46 && py == cy) {
      tutorialStep = TutorialStep::FinishedMenu;
      PlaySound(sfxWin);
    }
    break;

  case TutorialStep::FinishedMenu:
    break;
  }
}

void Game::renderTutorialUI() {
  // ---------------------------------------------------------
  // Menú final interactivo
  // ---------------------------------------------------------
  if (tutorialStep == TutorialStep::FinishedMenu) {
    // Fondo oscurecido
    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.85f));

    int boxW = 500, boxH = 350;
    int cx = (screenW - boxW) / 2;
    int cy = (screenH - boxH) / 2;

    // Panel de fondo
    DrawRectangle(cx, cy, boxW, boxH, Color{20, 20, 25, 255});
    DrawRectangleLinesEx({(float)cx, (float)cy, (float)boxW, (float)boxH}, 2,
                         WHITE);

    // Título
    const char *title = _("ENTRENAMIENTO COMPLETADO");
    int titleW = MeasureText(title, 30);
    DrawText(title, cx + (boxW - titleW) / 2, cy + 30, 30, LIME);

    // Botones
    const char *options[] = {_("REPETIR TUTORIAL"), _("JUGAR PARTIDA"),
                             _("VOLVER AL MENU")};
    int startY = cy + 100;
    int btnH = 50;
    int gap = 20;

    for (int i = 0; i < 3; i++) {
      Rectangle btnRect = {(float)cx + 50, (float)startY + i * (btnH + gap),
                           (float)boxW - 100, (float)btnH};

      // Check hover del ratón para actualizar selección
      if (CheckCollisionPointRec(GetMousePosition(), btnRect)) {
        if (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)
          tutorialMenuSelection = i;
      }

      bool selected = (tutorialMenuSelection == i);

      // Colores
      Color bg = selected ? Color{60, 60, 80, 255} : Color{40, 40, 40, 255};
      Color border = selected ? SKYBLUE : GRAY;

      DrawRectangleRec(btnRect, bg);
      DrawRectangleLinesEx(btnRect, 2, border);

      int txtSize = 20;
      int txtW = MeasureText(options[i], txtSize);
      DrawText(options[i], btnRect.x + (btnRect.width - txtW) / 2,
               btnRect.y + (btnRect.height - txtSize) / 2, txtSize, WHITE);

      // Icono de selección
      if (selected) {
        DrawText(">", btnRect.x - 20, btnRect.y + 15, 20, YELLOW);
      }
    }
    return;
  }

  // ---------------------------------------------------------
  // Barra inferior (Instrucciones)
  // ---------------------------------------------------------
  float boxH = 110.0f;
  float yPos = screenH - boxH;

  DrawRectangle(0, (int)yPos, screenW, (int)boxH, Color{0, 0, 0, 230});
  DrawRectangleLines(0, (int)yPos, screenW, (int)boxH, WHITE);

  const char *msg = "";
  const char *subMsg = "";
  Color msgColor = YELLOW;

  switch (tutorialStep) {
  case TutorialStep::Intro:
    msg = _("BIENVENIDO AL HANGAR DE ENTRENAMIENTO");
    subMsg =
        getInputText(_("NOTA: [R] reinicia la partida, desactivado aqui."),
                     _("NOTA: [Y]/[Triangulo] reinicia, desactivado aqui."));
    break;
  case TutorialStep::Movement:
    msg = getInputText(_("MOVIMIENTO BASICO"), _("MOVIMIENTO BASICO"));
    subMsg = getInputText(_("Usa [W,A,S,D] o Flechas."),
                          _("Usa [CRUCETA] o [JOYSTICK IZQ]."));
    break;
  case TutorialStep::Dash:
    msg = _("MANIOBRA EVASIVA (DASH)");
    subMsg = getInputText(_("Pulsa [SHIFT] mientras te mueves."),
                          _("Pulsa [LB] o [LT]."));
    break;
  case TutorialStep::MoveMode:
    msg = _("MODOS DE MOTOR");
    subMsg =
        getInputText(_("Pulsa [T] para alternar Pasos/Continuo."),
                     _("Usa [CRUCETA] (Tactico) o [JOYSTICK IZQ] (Continuo)."));
    break;
  case TutorialStep::CameraZoom:
    msg = _("CONTROL DE CAMARA");
    subMsg = getInputText(_("Usa la [RUEDA DEL RATON]."),
                          _("Mueve el [JOYSTICK DERECHO]."));
    break;
  case TutorialStep::CameraReset:
    msg = _("RESTABLECER CAMARA");
    subMsg = getInputText(_("Pulsa [C] para centrar."),
                          _("Pulsa [R3] (Click Joystick Derecho)."));
    break;

  case TutorialStep::ItemPilaBuena:
    msg = _("OBJETO: PILA DE ENERGIA");
    subMsg =
        getInputText(_("Recupera salud. Acercate y pulsa [E] para recoger."),
                     _("Recupera salud. Acercate y pulsa [A]/[X]."));
    msgColor = GREEN;
    break;
  case TutorialStep::ItemPilaMala:
    msg = _("PELIGRO: PILA OXIDADA");
    subMsg = _("Este objeto DAÑA la salud (-1 Corazon). ¡Ten cuidado!");
    msgColor = RED;
    break;

  case TutorialStep::ItemEscudo:
    msg = _("OBJETO: ESCUDO DE FUERZA");
    subMsg = _("Otorga INVULNERABILIDAD temporal. Se desactiva al recibir el "
               "primer golpe");
    msgColor = SKYBLUE;
    break;

  case TutorialStep::PreGafas:
    msg = _("FALLO DE SENSORES");
    subMsg = _("Niebla densa detectada. Visibilidad reducida.");
    msgColor = ORANGE;
    break;
  case TutorialStep::ItemGafasBuenas:
    msg = _("OBJETO: GAFAS 3D");
    subMsg = _("Restauran visibilidad en niebla.");
    msgColor = BLUE;
    break;
  case TutorialStep::ItemGafasMalas:
    msg = _("PELIGRO: GAFAS ROTAS");
    subMsg = _("Reducen tu campo de vision.");
    msgColor = RED;
    break;
  case TutorialStep::BadGlassesEffect:
    msg = _("¡SISTEMAS VISUALES DAÑADOS!");
    subMsg =
        _("Esto ocurre al recoger items en mal estado. Esperando reinicio...");
    msgColor = RED;
    break;
  case TutorialStep::PostGafas:
    msg = _("SENSORES RESTAURADOS");
    subMsg = _("Calibrando sistemas...");
    break;

  case TutorialStep::ItemVidaExtra:
    msg = _("OBJETO RARO: BATERIA");
    subMsg = _("Otorga una VIDA EXTRA (Resurreccion).");
    msgColor = GOLD;
    break;

  case TutorialStep::SwordT1:
    msg = _("ARMA: ESPADA DE ACERO (TIER 1)");
    subMsg = getInputText(_("Ataque basico melee [1]."),
                          _("Ataque basico melee [RB]/[R1]."));
    break;
  case TutorialStep::SwordT2:
    msg = _("ARMA: ESPADA DE IONES (TIER 2)");
    subMsg = _("Hoja Verde. Mayor daño y velocidad.");
    msgColor = GREEN;
    break;
  case TutorialStep::SwordT3:
    msg = _("ARMA: ESPADA DE PLASMA (TIER 3)");
    subMsg = _("Hoja Roja. Daño maximo letal.");
    msgColor = RED;
    break;

  case TutorialStep::PlasmaT1:
    msg = _("ARMA: CAÑON DE PLASMA (TIER 1)");
    subMsg = getInputText(_("Disparo a distancia [2]."),
                          _("Disparo a distancia [RT]/[R2]."));
    msgColor = SKYBLUE;
    break;
  case TutorialStep::PlasmaT2:
    msg = _("ARMA: CAÑON DE PLASMA V2 (TIER 2)");
    subMsg = _("Lanza 2 ráfagas con un solo disparo.");
    msgColor = PURPLE;
    break;

  case TutorialStep::Combat:
    msg = _("SIMULACION DE COMBATE");
    subMsg = _("El objetivo esta al final del Hangar. Destruyelo.");
    msgColor = ORANGE;
    break;
  case TutorialStep::Exit:
    msg = _("OBJETIVO CUMPLIDO");
    subMsg = _("Recoge la LLAVE MAESTRA y ve a la SALIDA.");
    msgColor = GREEN;
    break;

  default:
    break;
  }

  int fs = 30;
  int fsSub = 20;
  int w = MeasureText(msg, fs);
  int w2 = MeasureText(subMsg, fsSub);

  DrawText(msg, (screenW - w) / 2, (int)yPos + 25, fs, msgColor);
  DrawText(subMsg, (screenW - w2) / 2, (int)yPos + 65, fsSub, WHITE);
}
