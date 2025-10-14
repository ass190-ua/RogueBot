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
  loaded = false;
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
  hp = 3;

  hasKey = false;
  hasShield = false;
  hasBattery = false;
  swordTier = 0;
  plasmaTier = 0;

  // reinicia progreso persistente del run
  runCtx.espadaMejorasObtenidas = 0;
  runCtx.plasmaMejorasObtenidas = 0;

  newLevel(currentLevel);
}

void Game::newLevel(int level) {
  const float WORLD_SCALE = 1.2f;
  int tilesX = (int)std::ceil((screenW / (float)tileSize) * WORLD_SCALE);
  int tilesY = (int)std::ceil((screenH / (float)tileSize) * WORLD_SCALE);

  // Deriva una seed distinta por nivel, determinista dentro del mismo run
  levelSeed = seedForLevel(runSeed, level);
  std::cout << "[Level] " << level << "/" << maxLevels
            << "  (seed nivel: " << levelSeed << ")\n";

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

  // (de momento no tienes enemigos: lista vacía)
  std::vector<IVec2> enemyTiles;

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
    px = nx;
    py = ny;
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

  // Si no estamos jugando, bloquea el resto del input (solo R y ESC funcionan)
  if (state != GameState::Playing) {
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

    int oldx = px, oldy = py;
    tryMove(dx, dy);
    moved = (px != oldx || py != oldy);

    if (moved) {
      player.setGridPos(px, py);
      player.setDirectionFromDelta(dx, dy);
      if (map.fogEnabled())
        map.computeVisibility(px, py, getFovRadius());

      // centrar cámara en jugador (mundo -> píxeles)
      camera.target = {px * (float)tileSize + tileSize / 2.0f,
                       py * (float)tileSize + tileSize / 2.0f};
      clampCameraToMap();
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

    if (pressedNow) {
      int oldx = px, oldy = py;
      tryMove(dx, dy);
      moved = (px != oldx || py != oldy);

      if (moved) {
        player.setGridPos(px, py);
        player.setDirectionFromDelta(dx, dy);
        if (map.fogEnabled())
          map.computeVisibility(px, py, getFovRadius());

        camera.target = {px * (float)tileSize + tileSize / 2.0f,
                         py * (float)tileSize + tileSize / 2.0f};
        clampCameraToMap();
      }

      moveCooldown = MOVE_INTERVAL;
    } else if ((dx != 0 || dy != 0) && moveCooldown <= 0.0f) {
      int oldx = px, oldy = py;
      tryMove(dx, dy);
      moved = (px != oldx || py != oldy);

      if (moved) {
        player.setGridPos(px, py);
        player.setDirectionFromDelta(dx, dy);
        if (map.fogEnabled())
          map.computeVisibility(px, py, getFovRadius());

        camera.target = {px * (float)tileSize + tileSize / 2.0f,
                         py * (float)tileSize + tileSize / 2.0f};
        clampCameraToMap(); // <-- aquí faltaba
      }

      moveCooldown = MOVE_INTERVAL;
    }

    // Actualiza animación (idle si no hubo movimiento este frame)
    player.update(dt, moved);
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
    return;
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

  // Jugador (en coordenadas del mundo)
  player.draw(tileSize, px, py);

  // Ítems del nivel (placeholder en colores)
  drawItems();

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
