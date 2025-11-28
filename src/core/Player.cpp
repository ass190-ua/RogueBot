#include "Player.hpp"
#include <stdexcept>
#include <iostream>
#include "AssetPath.hpp"

// Helper: Carga segura de texturas
// Intenta cargar una textura. Si falla (archivo no encontrado o corrupto),
// genera una imagen blanca de 32x32 píxeles en tiempo de ejecución.
// Esto evita que el juego haga crash o dibuje basura invisible.
static Texture2D loadTexPoint(const std::string& rel) {
    const std::string full = assetPath(rel); // Resuelve la ruta absoluta
    Texture2D t = LoadTexture(full.c_str());
    
    // Verificación de Raylib: id 0 significa error de carga
    if (t.id == 0) {
        std::cerr << "[ASSETS] FALLBACK tex point para: " << full << "\n";
        // Generar textura de emergencia (Placeholder)
        Image white = GenImageColor(32, 32, WHITE);
        t = LoadTextureFromImage(white);
        UnloadImage(white); // Liberamos la imagen de RAM una vez subida a VRAM
    }

    return t;
}

// Gestión de recursos
void Player::load(const std::string& baseDir) {
    if (loaded) return; // Evitar doble carga

    // Arrays auxiliares para construir los nombres de archivo dinámicamente
    // Patrón esperado: "robot_down_idle.png", "robot_up_walk1.png", etc.
    const char* dirs[4] = {"down","up","left","right"};
    const char* frames[3] = {"idle","walk1","walk2"};

    // Bucle anidado para cargar la matriz de 4x3 texturas
    for (int d = 0; d < 4; ++d) {
        for (int f = 0; f < 3; ++f) {
            std::string path = baseDir + "/robot_" + dirs[d] + std::string("_") + frames[f] + ".png";
            
            tex[d][f] = loadTexPoint(path); // Carga segura
            
            if (tex[d][f].id == 0) {
                // Aquí se podría ser estricto y lanzar excepción, 
                // pero el fallback anterior ya maneja el error visualmente.
                // throw std::runtime_error("No se pudo cargar " + path);
            }
        }
    }
    
    loaded = true;
}

void Player::unload() {
    if (!loaded) return;
    // Es vital liberar cada textura para evitar fugas de memoria VRAM
    for (int d = 0; d < 4; ++d)
        for (int f = 0; f < 3; ++f)
            if (tex[d][f].id != 0) UnloadTexture(tex[d][f]);

    loaded = false;
}

// Posición y dirección
void Player::setGridPos(int x, int y) { 
    gx = x; gy = y; 
}   

int  Player::getX() const { return gx; }
int  Player::getY() const { return gy; }

// Lógica de "Giro": Decide hacia dónde mira el sprite basándose en el movimiento.
// Prioridad: El eje X tiene precedencia si hay movimiento diagonal (aunque aquí el input suele ser cardinal).
void Player::setDirectionFromDelta(int dx, int dy) {
    if (dx == 0 && dy == 0) return; // Si no hay movimiento, mantén la dirección anterior
    
    if (dx > 0)      dir = Direction::Right;
    else if (dx < 0) dir = Direction::Left;
    else if (dy > 0) dir = Direction::Down;
    else if (dy < 0) dir = Direction::Up;
}

// Máquina de estados de animación
void Player::update(float dt, bool isMoving) {
    if (isMoving) {
        // Acumular tiempo
        animTimer += dt;
        
        // Si superamos el intervalo (ej: 0.12s), cambiamos de pie
        if (animTimer >= animInterval) {
            animTimer = 0.0f;
            walkIndex = 1 - walkIndex; // Truco matemático para alternar: 0 -> 1 -> 0
        }
    } else {
        // Resetear al estado de reposo inmediatamente al pararse
        // Esto hace que el control se sienta "snappy" (preciso)
        animTimer = 0.0f;
        walkIndex = 0;
    }
}

// Renderizado
void Player::draw(int tileSize, int px, int py) const {
    int d = dirIndex(dir); // Convierte Enum a int (0-3)

    // Lógica de selección de frame:
    // 1. Si estamos quietos (animTimer == 0 && walkIndex == 0) -> Frame 0 (Idle)
    // 2. Si nos movemos, alternamos entre Frame 1 y 2 según walkIndex.
    int frameWalk = (walkIndex == 0) ? 1 : 2; 
    
    // Detectar estado IDLE exacto:
    // La condición 'animTimer == 0.0f && walkIndex == 0' ocurre en el 'else' del update().
    bool isIdle = (animTimer == 0.0f && walkIndex == 0); 
    
    int drawFrame = isIdle ? 0 : frameWalk;

    const Texture2D& t = tex[d][drawFrame];
    
    // Coordenadas de pantalla
    const float sx = (float)(px * tileSize);
    const float sy = (float)(py * tileSize);

    // Escalado automático
    // Si tus sprites son de 16x16 pero el tile del juego es 32x32, esto lo escala x2.
    // Si son de 64x64, los reduce a la mitad. Muy robusto para probar distintos assets.
    float scale = (float)tileSize / (float)t.width;
    
    DrawTextureEx(t, {sx, sy}, 0.0f, scale, WHITE);
}
