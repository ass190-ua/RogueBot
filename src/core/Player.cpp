#include "Player.hpp"
#include <stdexcept>
#include <iostream>
#include <cmath> // Necesario para sinf
#include "AssetPath.hpp"
#include "ResourceManager.hpp" // Integración con el Gestor de Recursos

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
            
            // CAMBIO: Usamos el Singleton ResourceManager para cargar la textura
            // Esto asegura que si ya está en memoria, no se vuelva a leer del disco.
            tex[d][f] = ResourceManager::getInstance().getTexture(path);
            
            if (tex[d][f].id == 0) {
                // Aquí se podría ser estricto y lanzar excepción, 
                // pero el ResourceManager ya maneja el error visualmente (Magenta).
                // throw std::runtime_error("No se pudo cargar " + path);
            }
        }
    }
    
    loaded = true;
}

void Player::unload() {
    if (!loaded) return;
    
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

    // Al movernos, inclinamos el sprite hacia la dirección del movimiento
    if (dx > 0) targetTilt = -15.0f;      // Derecha
    else if (dx < 0) targetTilt = 15.0f;  // Izquierda
    else targetTilt = (dy > 0) ? 5.0f : -5.0f; // Arriba/Abajo (pequeño balanceo)
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

    // Respiración continua
    animTime += dt * 5.0f; 

    // Suavizado de la inclinación (Spring/Lerp)
    // El 15.0f hace que el jugador reaccione más rápido que los enemigos
    tiltAngle += (targetTilt - tiltAngle) * 15.0f * dt;
    
    // Decaimiento: Si dejas de moverte, targetTilt vuelve a 0
    // Si te mueves, setDirectionFromDelta lo volverá a poner en 15/-15
    if (!isMoving) {
        targetTilt += (0.0f - targetTilt) * 10.0f * dt;
    }
}

// Renderizado
void Player::draw(int tileSize, int px, int py) const {
    // Selección del frame (Igual que antes)
    int d = dirIndex(dir);
    int frame = (walkIndex == 0) ? 1 : 2; 
    int drawFrame = (animTimer == 0.0f && walkIndex == 0) ? 0 : frame;

    const Texture2D& t = tex[d][drawFrame];
    
    // Coordenadas base en pantalla
    const float sx = (float)(px * tileSize);
    const float sy = (float)(py * tileSize);
    
    // 1. Respiración (Squash & Stretch)
    // Cuando está quieto respira más visiblemente. Cuando corre, se tensa (menos amplitud).
    float breatheAmp = 0.05f; 
    float breathe = 1.0f + sinf(animTime) * breatheAmp;

    // 2. Origen de rotación (Los pies)
    // Para que al rotar no "flote", rotamos desde el centro inferior.
    Vector2 origin = { (float)tileSize / 2.0f, (float)tileSize };

    // 3. Rectángulos de Origen y Destino
    Rectangle src = { 0.0f, 0.0f, (float)t.width, (float)t.height };
    
    // El destino se ajusta para aplicar el "breathe" en el eje Y
    Rectangle dest = {
        sx + tileSize / 2.0f, // Centro X
        sy + tileSize,        // Base Y (Pies)
        (float)tileSize,      // Ancho
        (float)tileSize * breathe // Alto variable
    };

    // 4. Dibujar con rotación
    DrawTexturePro(t, src, dest, origin, tiltAngle, WHITE);
}
