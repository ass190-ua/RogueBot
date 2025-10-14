#include "Game.hpp"
#include "raylib.h"
#include <algorithm> // para std::min y std::clamp
#include <cmath>     // para std::floor
#include <ctime>
#include <iostream>

static inline unsigned now_seed() {
  return static_cast<unsigned>(time(nullptr));
}

static Texture2D loadTex(const char *path) {
  Image img = LoadImage(path);
  if (img.data == nullptr) {
    // fallback: 1x1 blanco si falla
    Image white = GenImageColor(1, 1, WHITE);
    Texture2D t = LoadTextureFromImage(white);
    UnloadImage(white);
    return t;
  }
  Texture2D t = LoadTextureFromImage(img);
  UnloadImage(img);
  return t;
}

void ItemSprites::load() {
  if (loaded)
    return;
  keycard = loadTex("assets/sprites/items/item_keycard.png");
  shield = loadTex("assets/sprites/items/item_shield.png");
  pila = loadTex("assets/sprites/items/item_healthbattery.png");
  glasses = loadTex("assets/sprites/items/item_glasses.png");
  swordBlue = loadTex("assets/sprites/items/item_sword_blue.png");
  swordGreen = loadTex("assets/sprites/items/item_sword_green.png");
  swordRed = loadTex("assets/sprites/items/item_sword_red.png");
  plasma1 = loadTex("assets/sprites/items/item_plasma1.png");
  plasma2 = loadTex("assets/sprites/items/item_plasma2.png");
  battery = loadTex("assets/sprites/items/item_battery.png");
  enemy = loadTex("assets/sprites/enemies/enemy.png");
  enemyUp = loadTex("assets/sprites/enemies/enemy_up.png");
  enemyDown = loadTex("assets/sprites/enemies/enemy_down.png");
  enemyLeft = loadTex("assets/sprites/enemies/enemy_left.png");
  enemyRight = loadTex("assets/sprites/enemies/enemy_right.png");
  loaded = true;
}

void ItemSprites::unload() {
  if (!loaded)
    return;
  UnloadTexture(keycard);
  UnloadTexture(shield);
  UnloadTexture(pila);
  UnloadTexture(glasses);
  UnloadTexture(swordBlue);
  UnloadTexture(swordGreen);
  UnloadTexture(swordRed);
  UnloadTexture(plasma1);
  UnloadTexture(plasma2);
  UnloadTexture(battery);
  UnloadTexture(enemy);
  UnloadTexture(enemyUp);
  UnloadTexture(enemyDown);
  UnloadTexture(enemyLeft);
  UnloadTexture(enemyRight);
  loaded = false;
}

// === Ataque melee por tiles
// ===================================================
struct AttackRuntime {
  // parámetros
  int rangeTiles = 1;      // 1 o 2 tiles de alcance
  float cooldown = 0.20f;  // s entre golpes
  float swingTime = 0.10f; // s que dura el gesto/flash

  // estado runtime
  float cdTimer = 0.f;
  float swingTimer = 0.f;
  bool swinging = false;

  // modo: false = cruz (4 direcciones adyacentes); true = solo al frente
  bool frontOnly = true;

  // última dirección del jugador (discreta) para "frente"
  IVec2 lastDir = {0, 1};

  // tiles golpeados este gesto (para debug/flash)
  std::vector<IVec2> lastTiles;
};
static AttackRuntime gAttack;

// Normaliza a eje dominante (N/S/E/O)
static inline IVec2 dominantAxis(IVec2 d) {
  if (std::abs(d.x) >= std::abs(d.y))
    return {(d.x >= 0) ? 1 : -1, 0};
  else
    return {0, (d.y >= 0) ? 1 : -1};
}

// Genera los tiles objetivo según modo y rango
static inline std::vector<IVec2> computeMeleeTiles(IVec2 center, IVec2 lastDir,
                                                   int range, bool frontOnly) {
  std::vector<IVec2> out;
  out.reserve(frontOnly ? range : range * 4);
  if (frontOnly) {
    IVec2 f =
        dominantAxis(lastDir.x == 0 && lastDir.y == 0 ? IVec2{0, 1} : lastDir);
    for (int t = 1; t <= range; ++t)
      out.push_back({center.x + f.x * t, center.y + f.y * t});
  } else {
    for (int t = 1; t <= range; ++t) {
      out.push_back({center.x + t, center.y});
      out.push_back({center.x - t, center.y});
      out.push_back({center.x, center.y + t});
      out.push_back({center.x, center.y - t});
    }
  }
  return out;
}

// Genera tiles del ataque pero CORTANDO por paredes/no walkables.
static inline std::vector<IVec2>
computeMeleeTilesOccluded(IVec2 center, IVec2 lastDir, int range,
                          bool frontOnly, const Map &map) {
  auto walkable = [&](int x, int y) -> bool {
    return x >= 0 && y >= 0 && x < map.width() && y < map.height() &&
           map.isWalkable(x, y);
  };

  std::vector<IVec2> out;
  out.reserve(range * (frontOnly ? 1 : 4));

  auto pushRay = [&](IVec2 dir) {
    for (int t = 1; t <= range; ++t) {
      int tx = center.x + dir.x * t;
      int ty = center.y + dir.y * t;
      if (!walkable(tx, ty))
        break; // pared/obstáculo: cortamos y NO añadimos
      out.push_back({tx, ty});
    }
  };

  if (frontOnly) {
    IVec2 f = dominantAxis((lastDir.x == 0 && lastDir.y == 0) ? IVec2{0, 1}
                                                              : lastDir);
    pushRay(f);
  } else {
    pushRay({1, 0});
    pushRay({-1, 0});
    pushRay({0, 1});
    pushRay({0, -1});
  }
  return out;
}

// === HP y combate ================================================
static std::vector<int> gEnemyHP;      // HP por enemigo (paralelo a enemies)
static std::vector<float> gEnemyAtkCD; // cooldown de ataque enemigo (s)

static constexpr int ENEMY_BASE_HP = 100;   // % (0..100)
static constexpr int PLAYER_MELEE_DMG = 25; // daño por golpe del jugador
static constexpr int ENEMY_CONTACT_DMG = 1; // daño al jugador por golpe enemigo
static constexpr float ENEMY_ATTACK_COOLDOWN =
    0.40f; // s entre golpes del mismo enemigo
static constexpr int RANGE_MELEE_HAND = 1;
static constexpr int RANGE_SWORD = 3;  // para cuando implementes espada
static constexpr int RANGE_PISTOL = 7; // para cuando implementes pistola

static inline bool isAdjacent4(int ax, int ay, int bx, int by) {
  return (std::abs(ax - bx) + std::abs(ay - by)) == 1;
}

static inline IVec2 facingToDir(Game::EnemyFacing f) {
  switch (f) {
  case Game::EnemyFacing::Up:
    return {0, -1};
  case Game::EnemyFacing::Down:
    return {0, 1};
  case Game::EnemyFacing::Left:
    return {-1, 0};
  case Game::EnemyFacing::Right:
    return {1, 0};
  }
  return {0, 1};
}

Game::Game(unsigned seed) : fixedSeed(seed) {
  // Configurar ventana en fullscreen
  SetConfigFlags(FLAG_FULLSCREEN_MODE);
  InitWindow(0, 0, "RogueBot Alpha"); // tamaño se ajusta por el flag

  player.load("assets/sprites/player");
  itemSprites.load();

  screenW = GetScreenWidth();
  screenH = GetScreenHeight();

  camera.target = {0.0f, 0.0f}; // lo actualizamos tras posicionar al jugador
  camera.offset = {(float)screenW / 2, (float)screenH / 2};
  camera.rotation = 0.0f;
  camera.zoom = cameraZoom; // 1.0f por defecto

  SetTargetFPS(60);

  newRun(); // primera partida (empieza en Level 1)
}

// Devuelve la seed del RUN (si hay fija, esa; si no, aleatoria en cada R)
unsigned Game::nextRunSeed() const {
  return fixedSeed > 0 ? fixedSeed : now_seed();
}

// Mezcla para derivar una seed distinta por nivel del mismo run
unsigned Game::seedForLevel(unsigned base, int level) const {
  // Mezcla simple tipo "golden ratio" para variar por nivel
  const unsigned MIX = 0x9E3779B9u; // 2654435769
  return base ^ (MIX * static_cast<unsigned>(level));
}

void Game::newRun() {
  // runSeed: fijo si hay CLI, aleatorio si no
  runSeed = nextRunSeed();
  std::cout << "[Run] Seed base del run: " << runSeed << "\n";

  currentLevel = 1;
  state = GameState::Playing;
  moveCooldown = 0.0f;
  hp = 5;

  hasKey = false;
  hasShield = false;
  hasBattery = false;
  swordTier = 0;
  plasmaTier = 0;

  // reinicia progreso persistente del run
  runCtx.espadaMejorasObtenidas = 0;
  runCtx.plasmaMejorasObtenidas = 0;
  // reset ataque
  gAttack = AttackRuntime{};
  gAttack.frontOnly = true;

  newLevel(currentLevel);
}

void Game::newLevel(int level) {
  const float WORLD_SCALE = 1.2f;
  int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
  int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

  // Deriva una seed distinta por nivel, determinista dentro del mismo run
  levelSeed = seedForLevel(runSeed, level);
  std::cout << "[Level] " << level << "/" << maxLevels
            << " (seed nivel: " << levelSeed << ")\n";

  map.generate(tilesX, tilesY, levelSeed);

  auto r = map.firstRoom();
  if (r.w > 0 && r.h > 0) {
    px = r.x + r.w / 2;
    py = r.y + r.h / 2;
    map.computeVisibility(px, py, getFovRadius());
  } else {
    px = tilesX / 2;
    py = tilesY / 2;
  }

  player.setGridPos(px, py);

  // Recalcular FOV según viewport/tileSize actual
  fovTiles = defaultFovFromViewport();
  map.computeVisibility(px, py, getFovRadius());

  // centrar cámara en jugador (mundo -> píxeles)
  Vector2 playerCenterPx = {px * (float)tileSize + tileSize / 2.0f,
                            py * (float)tileSize + tileSize / 2.0f};
  camera.target = playerCenterPx;
  clampCameraToMap();

  hasKey = false; // hay una llave por nivel

  // ==== Spawner de ítems ====
  // RNG determinista por nivel
  rng = std::mt19937(levelSeed);

  // --- Enemigos del nivel ---
  spawnEnemiesForLevel();

  // (de momento no tienes enemigos: lista vacía.
  std::vector<IVec2> enemyTiles;
  enemyTiles.reserve(enemies.size());
  for (const auto &e : enemies) {
    enemyTiles.push_back({e.getX(), e.getY()});
  }

  // Walkable: todo lo que no sea WALL, con bounds check
  auto isWalkable = [&](int x, int y) { return map.isWalkable(x, y); };

  // Spawn = posición actual del jugador
  IVec2 spawnTile{px, py};

  // Exit = pedimos al mapa su posición
  auto [exitX, exitY] = map.findExitTile();
  IVec2 exitTile{exitX, exitY};

  // Generar ítems de este nivel (solo colocación; sin lógica aún)
  items = ItemSpawner::generate(map.width(), map.height(), isWalkable,
                                spawnTile, exitTile, enemyTiles,
                                level, // 1..3
                                rng, runCtx);
}

void Game::tryMove(int dx, int dy) {
  if (dx == 0 && dy == 0)
    return;

  int nx = px + dx, ny = py + dy;

  if (nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
      map.at(nx, ny) != WALL) {
    // ⬇️ AÑADIDO: impedir entrar en una casilla ocupada por un enemigo
    bool occupied = false;
    for (const auto &e : enemies) {
      if (e.getX() == nx && e.getY() == ny) {
        occupied = true;
        break;
      }
    }

    if (!occupied) {
      px = nx;
      py = ny;
    }
    // (si está ocupada, simplemente no te mueves)
  }
}

const char *Game::movementModeText() const {
  return (moveMode == MovementMode::StepByStep) ? "Step-by-step"
                                                : "Repeat (cooldown)";
}

void Game::run() {
  while (!WindowShouldClose()) {
    processInput();
    update();
    render();
  }
  player.unload();
  itemSprites.unload();
  CloseWindow();
}

void Game::processInput() {
  // Reiniciar run completo
  if (IsKeyPressed(KEY_R)) {
    if (fixedSeed == 0)
      runSeed = nextRunSeed();
    newRun();
    return;
  }

  // Toggle modo de movimiento (step-by-step <-> cooldown)
  if (IsKeyPressed(KEY_T)) {
    moveMode = (moveMode == MovementMode::StepByStep)
                   ? MovementMode::RepeatCooldown
                   : MovementMode::StepByStep;
    moveCooldown = 0.0f;
  }

  // --- Toggle SECRETO de NIEBLA (no aparece en HUD) ---
  if (IsKeyPressed(KEY_F2)) {
    fogEnabled = !fogEnabled;
    map.setFogEnabled(fogEnabled);
    if (fogEnabled) {
      map.computeVisibility(px, py, getFovRadius());
    }
  }

  // Ajuste manual del FOV en tiles
  if (IsKeyPressed(KEY_LEFT_BRACKET)) { // '['
    fovTiles = std::max(2, fovTiles - 1);
    if (map.fogEnabled())
      map.computeVisibility(px, py, getFovRadius());
  }
  if (IsKeyPressed(KEY_RIGHT_BRACKET)) { // ']'
    fovTiles = std::min(30, fovTiles + 1);
    if (map.fogEnabled())
      map.computeVisibility(px, py, getFovRadius());
  }

  // Si no estamos jugando (p.ej., victoria), no mover ni animar
  if (state != GameState::Playing) {
    player.update(GetFrameTime(), /*isMoving=*/false);
    return;
  }

  // Zoom de cámara
  if (IsKeyDown(KEY_Q))
    cameraZoom += 1.0f * GetFrameTime(); // acercar
  if (IsKeyDown(KEY_E))
    cameraZoom -= 1.0f * GetFrameTime(); // alejar

  // Rueda del ratón (log-scaling, como el ejemplo oficial)
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    cameraZoom = expf(logf(cameraZoom) + wheel * 0.1f);
  }

  cameraZoom = std::clamp(cameraZoom, 0.5f, 3.0f);
  camera.zoom = cameraZoom;
  clampCameraToMap();

  // Reset SOLO de cámara (cambiamos la tecla para no chocar con reset de run)
  if (IsKeyPressed(KEY_C)) {
    cameraZoom = 1.0f;
    camera.zoom = cameraZoom;
    camera.rotation = 0.0f;
    clampCameraToMap();
  }

  // === Movimiento del jugador + animación de sprites ===
  int dx = 0, dy = 0;
  bool moved = false;
  const float dt = GetFrameTime();

  if (moveMode == MovementMode::StepByStep) {
    // Un paso por pulsación
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
      dy = -1;
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
      dy = +1;
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
      dx = -1;
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
      dx = +1;

    if (dx != 0 || dy != 0) {
      gAttack.lastDir = {dx, dy};
      player.setDirectionFromDelta(dx,
                                   dy); // para que el sprite gire sin moverse
    }

    int oldx = px, oldy = py;
    tryMove(dx, dy);
    moved = (px != oldx || py != oldy);

    if (moved) {

      player.setGridPos(px, py);
      if (map.fogEnabled())
        map.computeVisibility(px, py, getFovRadius());

      // centrar cámara en jugador (mundo -> píxeles)
      camera.target = {px * (float)tileSize + tileSize / 2.0f,
                       py * (float)tileSize + tileSize / 2.0f};
      clampCameraToMap();
      updateEnemiesAfterPlayerMove(true);
    }
    // Actualiza animación (idle si no te moviste)
    player.update(dt, moved);
  } else {
    // Repetición con cooldown mientras mantienes tecla
    moveCooldown -= dt;

    const bool pressedNow = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) ||
                            IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D) ||
                            IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                            IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT);

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
      dy = -1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
      dy = +1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
      dx = -1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
      dx = +1;

    // permite girarte sin moverte
    if (dx != 0 || dy != 0) {
      gAttack.lastDir = {dx, dy};
      player.setDirectionFromDelta(dx, dy);
    }

    if (pressedNow) {
      int oldx = px, oldy = py;
      tryMove(dx, dy);
      moved = (px != oldx || py != oldy);

      if (moved) {
        // NUEVO
        if (dx != 0 || dy != 0)
          gAttack.lastDir = {dx, dy};
        player.setGridPos(px, py);
        player.setDirectionFromDelta(dx, dy);
        if (map.fogEnabled())
          map.computeVisibility(px, py, getFovRadius());

        camera.target = {px * (float)tileSize + tileSize / 2.0f,
                         py * (float)tileSize + tileSize / 2.0f};
        clampCameraToMap();
        updateEnemiesAfterPlayerMove(true);
      }

      moveCooldown = MOVE_INTERVAL;
    } else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f) {
      int oldx = px, oldy = py;
      tryMove(dx, dy);
      moved = (px != oldx || py != oldy);

      if (moved) {

        // NUEVO
        if (dx != 0 || dy != 0)
          gAttack.lastDir = {dx, dy};
        player.setGridPos(px, py);
        player.setDirectionFromDelta(dx, dy);
        if (map.fogEnabled())
          map.computeVisibility(px, py, getFovRadius());

        camera.target = {px * (float)tileSize + tileSize / 2.0f,
                         py * (float)tileSize + tileSize / 2.0f};
        clampCameraToMap();
        updateEnemiesAfterPlayerMove(true);
      }

      moveCooldown = MOVE_INTERVAL;
    }

    // Actualiza animación (idle si no hubo movimiento este frame)
    player.update(dt, moved);
  }

  // === ATAQUE MELEE
  // =============================================================
  gAttack.cdTimer = std::max(0.f, gAttack.cdTimer - dt);
  if (gAttack.swinging) {
    gAttack.swingTimer = std::max(0.f, gAttack.swingTimer - dt);
    if (gAttack.swingTimer <= 0.f) {
      gAttack.swinging = false;
      gAttack.lastTiles.clear();
    }
  }

  // Rango: 1 tile base, 2 si swordTier >= 2
  gAttack.rangeTiles = RANGE_MELEE_HAND;

  // Input de ataque: SPACE o click izq
  const bool attackPressed =
      IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

  if (attackPressed && gAttack.cdTimer <= 0.f) {
    // 1) Calcula tiles válidos con oclusión ANTES de iniciar el gesto
    IVec2 center{px, py};
    auto tiles = computeMeleeTilesOccluded(
        center, gAttack.lastDir, gAttack.rangeTiles, gAttack.frontOnly, map);

    if (tiles.empty()) {
      // Delante hay pared/obstáculo: NO swing, NO cooldown, NO flash
      // (opcional) std::cout << "[Melee] Bloqueado por pared\n";
      return;
    }

    // 2) Ahora sí, inicia gesto/flash/cooldown
    gAttack.swinging = true;
    gAttack.swingTimer = gAttack.swingTime;
    gAttack.cdTimer = gAttack.cooldown;
    gAttack.lastTiles = std::move(tiles);

    std::vector<size_t> toRemove;
    toRemove.reserve(enemies.size());

    bool hitSomeone = false;

    for (size_t i = 0; i < enemies.size(); ++i) {
      const int ex = enemies[i].getX();
      const int ey = enemies[i].getY();

      bool impacted = false;
      for (const auto &t : gAttack.lastTiles) {
        if (t.x == ex && t.y == ey) {
          impacted = true;
          break;
        }
      }
      if (!impacted)
        continue;

      if (gEnemyHP.size() != enemies.size()) {
        gEnemyHP.resize(enemies.size(), ENEMY_BASE_HP);
        gEnemyAtkCD.resize(enemies.size(), 0.0f);
      }

      hitSomeone = true;

      int before = gEnemyHP[i];
      gEnemyHP[i] = std::max(0, gEnemyHP[i] - PLAYER_MELEE_DMG);

      std::cout << "[Melee] Player -> Enemy (" << ex << "," << ey << ") "
                << before << "% -> " << gEnemyHP[i] << "%\n";

      if (gEnemyHP[i] <= 0)
        toRemove.push_back(i);
    }

    if (!toRemove.empty()) {
      std::sort(toRemove.rbegin(), toRemove.rend());
      for (size_t idx : toRemove) {
        enemies.erase(enemies.begin() + (long)idx);
        if (idx < enemyFacing.size())
          enemyFacing.erase(enemyFacing.begin() + (long)idx);
        if (idx < gEnemyHP.size())
          gEnemyHP.erase(gEnemyHP.begin() + (long)idx);
        if (idx < gEnemyAtkCD.size())
          gEnemyAtkCD.erase(gEnemyAtkCD.begin() + (long)idx);
      }
    }

    if (!hitSomeone)
      std::cout << "[Melee] Swing (no target)\n";

    // Enemigo intenta contraatacar si está enfrente y mirando
    enemyTryAttackFacing();
  }

  // H: perder vida
  if (IsKeyPressed(KEY_H)) {
    hp = std::max(0, hp - 1);
  }

  // J: Ganar vida
  if (IsKeyPressed(KEY_J)) {
    hp = std::min(hpMax, hp + 1);
  }
}

void Game::clampCameraToMap() {
  const float worldW = map.width() * (float)tileSize;
  const float worldH = map.height() * (float)tileSize;

  // tamaño del viewport en coordenadas de mundo (depende del zoom)
  const float viewW = screenW / camera.zoom;
  const float viewH = screenH / camera.zoom;
  const float halfW = viewW * 0.5f;
  const float halfH = viewH * 0.5f;

  // Si el mundo es más pequeño que el viewport, centramos;
  // si no, acotamos el target al rango [half, world-half]
  if (worldW <= viewW)
    camera.target.x = worldW * 0.5f;
  else
    camera.target.x = std::clamp(camera.target.x, halfW, worldW - halfW);

  if (worldH <= viewH)
    camera.target.y = worldH * 0.5f;
  else
    camera.target.y = std::clamp(camera.target.y, halfH, worldH - halfH);
}

int Game::defaultFovFromViewport() const {
  // nº de tiles visibles en cada eje
  int tilesX = screenW / tileSize;
  int tilesY = screenH / tileSize;
  int r = static_cast<int>(std::floor(std::min(tilesX, tilesY) * 0.15f));
  return std::clamp(r, 3, 20);
}

void Game::update() {
  if (state != GameState::Playing)
    return;

  // recoger si estás encima de algo
  tryPickupHere();

  // ¿Has llegado a la salida?
  if (map.at(px, py) == EXIT) {
    if (hasKey)
      onExitReached();
  }

  // helper local
  auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };

  // cada frame, en vez de asignar directo:
  Vector2 desired = {px * (float)tileSize + tileSize / 2.0f,
                     py * (float)tileSize + tileSize / 2.0f};
  float smooth = 10.0f * GetFrameTime(); // 0.0–1.0 (ajusta a gusto)
  camera.target.x = Lerp(camera.target.x, desired.x, smooth);
  camera.target.y = Lerp(camera.target.y, desired.y, smooth);
  clampCameraToMap();

  // Transición a Game Over si la vida llega a 0
  if (hp <= 0) {
    state = GameState::GameOver;
    gAttack.swinging = false; // por si había flash de melee activo
    gAttack.lastTiles.clear();
    return;
  }

  // Aquí podrías añadir daño / enemigos y pasar a Game Over si hp<=0
  // enfriar el cooldown de daño
  damageCooldown = std::max(0.0f, damageCooldown - GetFrameTime());
  for (auto &cd : gEnemyAtkCD) {
    cd = std::max(0.0f, cd - GetFrameTime());
  }
}

void Game::onExitReached() {
  if (currentLevel < maxLevels) {
    currentLevel++;
    newLevel(currentLevel); // ← cada nivel usa levelSeed diferente
  } else {
    state = GameState::Victory;
    std::cout << "[Victory] ¡Has completado los 3 niveles!\n";
  }
}

void Game::render() {
  BeginDrawing();
  ClearBackground(BLACK);

  // --- Cámara: el mapa y el jugador se dibujan dentro del mundo ---
  BeginMode2D(camera);

  // Mapa (mundo)
  map.draw(tileSize);

  // Enemigos
  drawEnemies();

  // Jugador (en coordenadas del mundo)
  player.draw(tileSize, px, py);

  // Ítems del nivel (placeholder en colores)
  drawItems();
  // === DEBUG: flash de tiles golpeados (melee) =========================
  // Requiere que exista gAttack (AttackRuntime) y que lo actualices en
  // processInput()
  if (gAttack.swinging && !gAttack.lastTiles.empty()) {
    Color fill = {255, 230, 50, 120}; // amarillo translúcido
    for (const auto &t : gAttack.lastTiles) {
      int xpx = t.x * tileSize;
      int ypx = t.y * tileSize;
      DrawRectangle(xpx, ypx, tileSize, tileSize, fill);
      DrawRectangleLines(xpx, ypx, tileSize, tileSize, YELLOW);
    }
  }

  EndMode2D();
  // --- Fin de cámara ---

  // --- HUD --- (no afectado por cámara ni zoom)
  if (state == GameState::Playing) {
    hud.drawPlaying(*this);
  } else if (state == GameState::Victory) {
    hud.drawVictory(*this);
  } else if (state == GameState::GameOver) {
    hud.drawGameOver(*this);
  }

  EndDrawing();
}

void Game::drawItems() const {
  auto isVisible = [&](int x, int y) {
    if (map.fogEnabled())
      return map.isVisible(x, y);
    return true;
  };

  for (const auto &it : items) {
    if (!isVisible(it.tile.x, it.tile.y))
      continue;
    drawItemSprite(it);
  }
}

void Game::tryPickupHere() {
  // busca un item exactamente en (px,py)
  for (size_t i = 0; i < items.size(); ++i) {
    if (items[i].tile.x == px && items[i].tile.y == py) {
      onPickup(items[i]);
      // quita el item del mundo
      items.erase(items.begin() + i);
      break;
    }
  }
}

void Game::drawItemSprite(const ItemSpawn &it) const {
  // Selección de textura según tipo/tier sugerido
  const Texture2D *tex = nullptr;

  switch (it.type) {
  case ItemType::LlaveMaestra:
    tex = &itemSprites.keycard;
    break;
  case ItemType::Escudo:
    tex = &itemSprites.shield;
    break;
  case ItemType::PilaBuena:
  case ItemType::PilaMala:
    tex = &itemSprites.pila;
    break;
  case ItemType::Gafas3DBuenas:
  case ItemType::Gafas3DMalas:
    tex = &itemSprites.glasses;
    break;
  case ItemType::BateriaVidaExtra:
    tex = &itemSprites.battery;
    break;
  case ItemType::EspadaPickup: {
    // tier visual = min(tierSugerido, mejorasObtenidas + 1)
    int displayTier =
        std::min(it.tierSugerido, runCtx.espadaMejorasObtenidas + 1);
    displayTier = std::clamp(displayTier, 1, 3);
    tex = (displayTier == 1)   ? &itemSprites.swordBlue
          : (displayTier == 2) ? &itemSprites.swordGreen
                               : &itemSprites.swordRed;
    break;
  }
  case ItemType::PistolaPlasmaPickup: {
    int displayTier =
        std::min(it.tierSugerido, runCtx.plasmaMejorasObtenidas + 1);
    displayTier = std::clamp(displayTier, 1, 2);
    tex = (displayTier == 1) ? &itemSprites.plasma1 : &itemSprites.plasma2;
    break;
  }
  }

  const int pxl = it.tile.x * tileSize;
  const int pyl = it.tile.y * tileSize;

  if (tex && tex->id != 0) {
    // Escala al tileSize por si no es 32 exactamente
    Rectangle src{0, 0, (float)tex->width, (float)tex->height};
    Rectangle dst{(float)pxl, (float)pyl, (float)tileSize, (float)tileSize};
    Vector2 origin{0, 0};
    DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
  } else {
    // Fallback por si faltara algún PNG
    DrawRectangle(pxl, pyl, tileSize, tileSize, WHITE);
    DrawRectangleLines(pxl, pyl, tileSize, tileSize, BLACK);
  }
}

void Game::onPickup(const ItemSpawn &it) {
  switch (it.type) {
  case ItemType::LlaveMaestra:
    hasKey = true;
    std::cout << "[Pickup] Llave maestra obtenida.\n";
    break;

  case ItemType::Escudo:
    hasShield = true;
    std::cout << "[Pickup] Escudo preparado.\n";
    break;

  case ItemType::BateriaVidaExtra:
    hasBattery = true;
    std::cout << "[Pickup] Batería extra lista.\n";
    break;

  case ItemType::EspadaPickup: {
    // regla: no saltar niveles si no recogiste anteriores
    int real = std::min(it.tierSugerido, swordTier + 1);
    if (real > swordTier)
      swordTier = real;
    runCtx.espadaMejorasObtenidas = swordTier;
    std::cout << "[Pickup] Espada nivel " << swordTier << ".\n";
    break;
  }

  case ItemType::PistolaPlasmaPickup: {
    int real = std::min(it.tierSugerido, plasmaTier + 1);
    if (real > plasmaTier)
      plasmaTier = real;
    runCtx.plasmaMejorasObtenidas = plasmaTier;
    std::cout << "[Pickup] Plasma nivel " << plasmaTier << ".\n";
    break;
  }

  // De momento las pilas y gafas solo se recogen y desaparecen.
  case ItemType::PilaBuena:
    std::cout << "[Pickup] Pila buena (sin efecto por ahora).\n";
    break;
  case ItemType::PilaMala:
    std::cout << "[Pickup] Pila mala (sin efecto por ahora).\n";
    break;
  case ItemType::Gafas3DBuenas:
    std::cout << "[Pickup] Gafas 3D buenas (20s, sin aplicar aún).\n";
    break;
  case ItemType::Gafas3DMalas:
    std::cout << "[Pickup] Gafas 3D malas (20s, sin aplicar aún).\n";
    break;
  }
}

void Game::spawnEnemiesForLevel() {
  enemies.clear();
  const int n = enemiesPerLevel(currentLevel);
  const int minDistTiles = 8;

  IVec2 spawnTile{px, py};
  auto [exitX, exitY] = map.findExitTile();
  IVec2 exitTile{exitX, exitY};

  std::vector<IVec2> candidates;
  candidates.reserve(map.width() * map.height());
  for (int y = 0; y < map.height(); ++y) {
    for (int x = 0; x < map.width(); ++x) {
      if (!map.isWalkable(x, y))
        continue;
      if ((x == spawnTile.x && y == spawnTile.y) ||
          (x == exitTile.x && y == exitTile.y))
        continue;
      int manhattan = std::abs(x - spawnTile.x) + std::abs(y - spawnTile.y);
      if (manhattan < minDistTiles)
        continue;
      candidates.push_back({x, y});
    }
  }
  if (candidates.empty())
    return;

  std::uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
  auto used = std::vector<IVec2>{spawnTile, exitTile};
  auto isUsed = [&](int x, int y) {
    for (auto &u : used)
      if (u.x == x && u.y == y)
        return true;
    for (auto &it : items)
      if (it.tile.x == x && it.tile.y == y)
        return true;
    return false;
  };

  for (int i = 0; i < n; ++i) {
    for (int tries = 0; tries < 200; ++tries) {
      const auto &p = candidates[dist(rng)];
      if (!isUsed(p.x, p.y)) {
        enemies.emplace_back(p.x, p.y);
        used.push_back(p);
        break;
      }
    }
  }
  enemyFacing.assign(enemies.size(), EnemyFacing::Down);
  // HP y cooldowns paralelos a "enemies"
  gEnemyHP.assign(enemies.size(), ENEMY_BASE_HP);
  gEnemyAtkCD.assign(enemies.size(), 0.0f);
}

void Game::updateEnemiesAfterPlayerMove(bool moved) {
  if (!moved)
    return;

  struct Intent {
    int fromx, fromy; // origen
    int tox, toy;     // destino propuesto
    bool wants;       // quiere moverse
    int score;        // menor = mejor (distancia al jugador tras moverse)
    size_t idx;       // índice del enemigo (para desempates estables)
  };

  auto inRangePx = [&](int ex, int ey) -> bool {
    int dx = px - ex, dy = py - ey;
    float distPx = std::sqrt(float(dx * dx + dy * dy)) * float(tileSize);
    return distPx <= float(ENEMY_DETECT_RADIUS_PX);
  };

  auto can = [&](int nx, int ny) -> bool {
    return nx >= 0 && ny >= 0 && nx < map.width() && ny < map.height() &&
           map.isWalkable(nx, ny);
  };

  auto sgn = [](int v) { return (v > 0) - (v < 0); };

  auto greedyNext = [&](int ex, int ey) -> std::pair<int, int> {
    int dx = px - ex, dy = py - ey;
    if (dx == 0 && dy == 0)
      return {ex, ey};
    if (std::abs(dx) >= std::abs(dy)) {
      int nx = ex + sgn(dx), ny = ey;
      if (can(nx, ny))
        return {nx, ny};
      nx = ex;
      ny = ey + sgn(dy);
      if (can(nx, ny))
        return {nx, ny};
    } else {
      int nx = ex, ny = ey + sgn(dy);
      if (can(nx, ny))
        return {nx, ny};
      nx = ex + sgn(dx);
      ny = ey;
      if (can(nx, ny))
        return {nx, ny};
    }
    return {ex, ey}; // bloqueado
  };

  // 1) Construir intenciones
  std::vector<Intent> intents;
  intents.reserve(enemies.size());
  for (size_t i = 0; i < enemies.size(); ++i) {
    const auto &e = enemies[i];
    Intent it{e.getX(), e.getY(), e.getX(), e.getY(), false, 1'000'000, i};

    if (inRangePx(e.getX(), e.getY())) {
      auto [nx, ny] = greedyNext(e.getX(), e.getY());
      it.tox = nx;
      it.toy = ny;
      it.wants = (nx != e.getX() || ny != e.getY());
      it.score = std::abs(px - nx) + std::abs(py - ny);
    } else {
      // fuera de rango: no se mueve
      it.score = std::abs(px - e.getX()) + std::abs(py - e.getY());
    }
    intents.push_back(it);
  }

  // 2) Resolver conflictos de MISMO destino: gana menor score; si empatan,
  // menor idx
  for (size_t i = 0; i < intents.size(); ++i) {
    if (!intents[i].wants)
      continue;
    for (size_t j = i + 1; j < intents.size(); ++j) {
      if (!intents[j].wants)
        continue;
      if (intents[i].tox == intents[j].tox &&
          intents[i].toy == intents[j].toy) {
        bool jWins = (intents[j].score < intents[i].score) ||
                     (intents[j].score == intents[i].score &&
                      intents[j].idx < intents[i].idx);
        if (jWins) {
          intents[i].tox = intents[i].fromx;
          intents[i].toy = intents[i].fromy;
          intents[i].wants = false;
        } else {
          intents[j].tox = intents[j].fromx;
          intents[j].toy = intents[j].fromy;
          intents[j].wants = false;
        }
      }
    }
  }

  // 3) No invadir la casilla de un enemigo que se queda quieto
  for (size_t i = 0; i < intents.size(); ++i) {
    if (!intents[i].wants)
      continue;
    for (size_t j = 0; j < intents.size(); ++j) {
      if (i == j)
        continue;
      bool otherStays =
          !intents[j].wants || (intents[j].tox == intents[j].fromx &&
                                intents[j].toy == intents[j].fromy);
      if (otherStays && intents[i].tox == intents[j].fromx &&
          intents[i].toy == intents[j].fromy) {
        intents[i].tox = intents[i].fromx;
        intents[i].toy = intents[i].fromy;
        intents[i].wants = false;
        break;
      }
    }
  }

  // 4) Evitar "swap" cabeza-con-cabeza (A<->B). Gana mejor score; si empatan,
  // menor idx
  for (size_t i = 0; i < intents.size(); ++i) {
    if (!intents[i].wants)
      continue;
    for (size_t j = i + 1; j < intents.size(); ++j) {
      if (!intents[j].wants)
        continue;
      bool headOnSwap = intents[i].tox == intents[j].fromx &&
                        intents[i].toy == intents[j].fromy &&
                        intents[j].tox == intents[i].fromx &&
                        intents[j].toy == intents[i].fromy;
      if (headOnSwap) {
        bool jWins = (intents[j].score < intents[i].score) ||
                     (intents[j].score == intents[i].score &&
                      intents[j].idx < intents[i].idx);
        if (jWins) {
          intents[i].tox = intents[i].fromx;
          intents[i].toy = intents[i].fromy;
          intents[i].wants = false;
        } else {
          intents[j].tox = intents[j].fromx;
          intents[j].toy = intents[j].fromy;
          intents[j].wants = false;
        }
      }
    }
  }

  // 5) Aplicar
  // 4) calcular facing por delta y aplicar movimientos
  for (size_t i = 0; i < enemies.size(); ++i) {
    int ox = intents[i].fromx, oy = intents[i].fromy;
    int tx = intents[i].tox, ty = intents[i].toy;

    // Facing por DELTA INTENTADO (aunque luego no se mueva)
    int dxTry = tx - ox, dyTry = ty - oy;
    if (dxTry != 0 || dyTry != 0) {
      if (std::abs(dxTry) >= std::abs(dyTry))
        enemyFacing[i] = (dxTry > 0) ? EnemyFacing::Right : EnemyFacing::Left;
      else
        enemyFacing[i] = (dyTry > 0) ? EnemyFacing::Down : EnemyFacing::Up;
    }

    // No permitir que un enemigo se meta en la casilla del jugador
    int nx = tx, ny = ty;
    if (nx == px && ny == py) {
      nx = ox;
      ny = oy;
    }

    enemies[i].setPos(nx, ny);
  }

  enemyTryAttackFacing();
}

void Game::drawEnemies() const {
  auto visible = [&](int x, int y) {
    return !map.fogEnabled() || map.isVisible(x, y);
  };

  for (size_t i = 0; i < enemies.size(); ++i) {
    const auto &e = enemies[i];
    if (!visible(e.getX(), e.getY()))
      continue;

    // elige textura por facing
    const Texture2D *tex = nullptr;
    switch (i < enemyFacing.size() ? enemyFacing[i] : EnemyFacing::Down) {
    case EnemyFacing::Up:
      tex = &itemSprites.enemyUp;
      break;
    case EnemyFacing::Down:
      tex = &itemSprites.enemyDown;
      break;
    case EnemyFacing::Left:
      tex = &itemSprites.enemyLeft;
      break;
    case EnemyFacing::Right:
      tex = &itemSprites.enemyRight;
      break;
    }

    const float xpx = (float)(e.getX() * tileSize);
    const float ypx = (float)(e.getY() * tileSize);

    if (tex && tex->id != 0) {
      Rectangle src{0, 0, (float)tex->width, (float)tex->height};
      Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
      Vector2 origin{0, 0};
      DrawTexturePro(*tex, src, dst, origin, 0.0f, WHITE);
    } else if (itemSprites.enemy.id != 0) {
      // Fallback a enemy.png si no hay direccionales
      Rectangle src{0, 0, (float)itemSprites.enemy.width,
                    (float)itemSprites.enemy.height};
      Rectangle dst{xpx, ypx, (float)tileSize, (float)tileSize};
      Vector2 origin{0, 0};
      DrawTexturePro(itemSprites.enemy, src, dst, origin, 0.0f, WHITE);
    } else {
      // Fallback final: rectángulo rojo
      e.draw(tileSize);
    }

    // barra de vida sobre el enemigo i
    // barra de vida sobre el enemigo i
    if (i < gEnemyHP.size()) {
      const int w = tileSize;
      const int h = 4;
      const int x = (int)(e.getX() * tileSize);
      const int y = (int)(e.getY() * tileSize) - (h + 2);

      int hpw = (int)std::lround(w * (gEnemyHP[i] / 100.0)); // 0..w
      hpw = std::clamp(hpw, 0, w);

      DrawRectangle(x, y, w, h, (Color){60, 60, 60, 200}); // fondo

      // --- COLOR GRADIENTE VERDE -> ROJO segun % ---
      float pct = std::clamp(gEnemyHP[i] / 100.0f, 0.0f, 1.0f);
      unsigned char r = (unsigned char)std::lround(
          255 * (1.0f - pct)); // sube al bajar la vida
      unsigned char g =
          (unsigned char)std::lround(255 * pct); // baja al bajar la vida
      Color lifeCol = {r, g, 0, 230};

      DrawRectangle(x, y, hpw, h, lifeCol);  // barra
      DrawRectangleLines(x, y, w, h, BLACK); // borde
    }
  }
}

void Game::takeDamage(int amount) {
  int old = hp;
  hp = std::max(0, hp - amount);
  int oldPct = (int)std::lround(100.0 * old / std::max(1, hpMax));
  int newPct = (int)std::lround(100.0 * hp / std::max(1, hpMax));
  std::cout << "[HP] -" << amount << " (" << old << "→" << hp << ") " << oldPct
            << "%→" << newPct << "%\n";

  // Más adelante: if (hp == 0) state = GameState::GameOver;
  // Si mueres, pasa a GameOver y limpia el swing de melee para que no se quede
  // dibujado.
  if (hp == 0 && state == GameState::Playing) {
    state = GameState::GameOver;
    gAttack.swinging = false;
    gAttack.lastTiles.clear();
    std::cout << "[GameOver] Has sido destruido.\n";
  }
}

void Game::enemyTryAttackFacing() {
  for (size_t i = 0; i < enemies.size(); ++i) {
    const int ex = enemies[i].getX();
    const int ey = enemies[i].getY();

    if (!isAdjacent4(ex, ey, px, py))
      continue;

    // 1) ¿El enemigo mira al jugador?
    IVec2 edir = facingToDir(i < enemyFacing.size() ? enemyFacing[i]
                                                    : EnemyFacing::Down);
    int sdx = (px > ex) - (px < ex);
    int sdy = (py > ey) - (py < ey);
    if (edir.x != sdx || edir.y != sdy)
      continue;

    // 2) ➕ ¿El jugador mira al enemigo? (si quieres la regla de “ambos
    // mirándose”)
    IVec2 pdir = dominantAxis((gAttack.lastDir.x == 0 && gAttack.lastDir.y == 0)
                                  ? IVec2{0, 1}
                                  : gAttack.lastDir);
    int psx = (ex > px) - (ex < px);
    int psy = (ey > py) - (ey < py);
    bool playerFacingEnemy = (pdir.x == psx && pdir.y == psy);
    if (!playerFacingEnemy)
      continue;

    if (i < gEnemyAtkCD.size() && gEnemyAtkCD[i] <= 0.0f &&
        damageCooldown <= 0.0f) {
      takeDamage(ENEMY_CONTACT_DMG);
      gEnemyAtkCD[i] = ENEMY_ATTACK_COOLDOWN;
      damageCooldown = DAMAGE_COOLDOWN;
      std::cout << "[EnemyAtk] (" << ex << "," << ey
                << ") facing player and player facing enemy.\n";
    }
  }
}