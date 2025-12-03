#pragma once
#include <vector>
#include <functional>
#include <queue>
#include <random>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <cstdlib>  

// Estructura simple para coordenadas 2D (Enteros)
struct IVec2 { int x{}, y{}; };

// Tipos de objetos que pueden aparecer en el suelo
enum class ItemType : uint8_t {
    PilaBuena, PilaMala,         // Recuperan o quitan energía
    Escudo,                      // Protección temporal
    Gafas3DBuenas, Gafas3DMalas, // Revelan mapa o ciegan
    BateriaVidaExtra,            // Item único (Nivel 3) que da una vida extra
    EspadaPickup,                // Mejora de daño cuerpo a cuerpo
    PistolaPlasmaPickup,         // Arma a distancia
    LlaveMaestra                 // Necesaria para salir del nivel
};

// Resultado de la generación: Qué es, dónde está y sus metadatos
struct ItemSpawn {
    ItemType type;
    IVec2    tile;         // Coordenadas en la rejilla (Tile X, Tile Y)
    int      nivel;        // Nivel actual (1..3)
    int      tierSugerido; // Para armas: indica qué nivel de potencia debería tener
};

// Contexto que persiste entre niveles (Estado del "Run")
struct RunContext {
    bool batterySpawned = false;    // Asegura que la batería de vida extra solo salga 1 vez por partida
    int espadaMejorasObtenidas = 0; // Para escalar el loot
    int plasmaMejorasObtenidas = 0;
};

// Configuración de equilibrio ("Knobs" de diseño)
struct SpawnConfig {
    // Cuotas: Cuántos items de cada tipo generar por nivel
    int pilasN1 = 3, pilasN2 = 4, pilasN3 = 5;
    int escudosN1 = 1, escudosN2 = 2, escudosN3 = 3;
    int gafasN1 = 2, gafasN2 = 3, gafasN3 = 4;

    // Probabilidades (RNG)
    double probPilaBuena = 0.65; // 65% probabilidad de éxito
    double probGafasBuenas = 0.65;

    // Reglas de Distancia y Posicionamiento
    int minSepEntreItems = 4;    // Radio mínimo entre cualquier objeto (evita amontonamiento)
    int llaveMinDistSpawn = 30;  // La llave debe estar lejos del inicio
    int llaveMinDistSalida = 20; // Y lejos de la salida (backtracking forzado)
    int llaveEnemyRadius   = 6;  // Área a chequear enemigos alrededor de la llave
    int llaveMinEnemies    = 2;  // La llave debe estar protegida por al menos X enemigos (Riesgo/Recompensa)

    // Configuración específica de items raros
    int bateriaMinDistSpawn = 25;

    // Relax: Cuántas veces intentamos bajar los requisitos si no cabe la llave
    int relaxPasos = 4;
};

class ItemSpawner {
public:
    // Función callback para consultar si una celda es muro o suelo
    using IsWalkableFn = std::function<bool(int,int)>;

    // Método principal estático: Genera todos los items de un nivel
    static std::vector<ItemSpawn> generate(
        int width, int height,
        const IsWalkableFn& isWalkable,
        IVec2 spawnTile,                      // Dónde empieza el jugador
        IVec2 exitTile,                       // Dónde está la salida
        const std::vector<IVec2>& enemyTiles, // Dónde están los enemigos (para riesgo/recompensa)
        int nivel,                            // 1..3
        std::mt19937& rng,                    // Generador de números aleatorios
        RunContext& run,                      // Estado persistente
        const SpawnConfig& cfg = {}           // Configuración de balanceo
    )
    {
        std::vector<ItemSpawn> out;
        if (nivel < 1 || nivel > 3) return out;

        auto inBounds = [&](int x, int y){ return x>=0 && y>=0 && x<width && y<height; };

        // 1. Mapas de calor (BFS - Breadth First Search)
        // Calculamos la distancia en "pasos" desde un punto origen a todas las celdas del mapa.
        // Esto nos permite saber matemáticamente qué es "lejos" y qué es "cerca".
        
        const int INF = std::numeric_limits<int>::max()/4;
        
        auto bfsDist = [&](IVec2 source) {
            std::vector<int> dist(width*height, INF); // Inicializar todo a Infinito
            std::queue<IVec2> q;
            
            auto idLocal = [&](int x,int y){ return y*width + x; }; // 2D -> 1D
            
            // Setup inicial
            if (inBounds(source.x, source.y) && isWalkable(source.x, source.y)) {
                dist[idLocal(source.x, source.y)] = 0;
                q.push(source);
            }
            
            // Direcciones cardinales (Arriba, Abajo, Izq, Der)
            const int dx[4] = {1,-1,0,0};
            const int dy[4] = {0,0,1,-1};
            
            // Bucle principal del BFS (Flood Fill)
            while(!q.empty()){
                auto p = q.front(); q.pop();
                int d = dist[idLocal(p.x,p.y)];
                
                for(int k=0;k<4;k++){
                    int nx=p.x+dx[k], ny=p.y+dy[k];
                    // Si es válido, caminable y no visitado (INF)
                    if (inBounds(nx,ny) && isWalkable(nx,ny)) {
                        int &ref = dist[idLocal(nx,ny)];
                        if (ref==INF){
                            ref = d+1; // La distancia es la del padre + 1
                            q.push({nx,ny});
                        }
                    }
                }
            }
            return dist;
        };

        // Generamos dos mapas de distancia:
        auto distSpawn = bfsDist(spawnTile); // Distancia desde el Jugador
        auto distExit  = bfsDist(exitTile);  // Distancia desde la Salida
        auto id = [&](int x,int y){ return y*width + x; };

        // Predicado: ¿Es esta celda alcanzable? (No es muro ni isla aislada)
        auto reachable = [&](IVec2 t){
            if (!inBounds(t.x,t.y) || !isWalkable(t.x,t.y)) return false;
            return distSpawn[id(t.x,t.y)] != INF;
        };

        // 2. Sistema de ocupación
        // Marcamos dónde ponemos objetos para que no se generen uno encima de otro.
        std::vector<uint8_t> occupied(width*height, 0);
        
        auto markOccupied = [&](IVec2 t){
            if (inBounds(t.x,t.y)) occupied[id(t.x,t.y)] = 1;
        };
        
        // Verifica que no haya otros items en un radio 'minSep' (Distancia Manhattan)
        auto isFarFromOtherItems = [&](IVec2 t, int minSep){
            if (!inBounds(t.x,t.y)) return false;
            for (int dy=-minSep; dy<=minSep; ++dy){
                for (int dx=-minSep; dx<=minSep; ++dx){
                    int nx=t.x+dx, ny=t.y+dy;
                    if (inBounds(nx,ny) && occupied[id(nx,ny)]) {
                        if (std::abs(dx)+std::abs(dy) <= minSep) return false; 
                    }
                }
            }
            return true;
        };

        // Cuenta enemigos alrededor de un punto (para asegurar que la llave tenga guardias)
        auto countEnemiesNear = [&](IVec2 t, int radius){
            int c=0;
            int r=radius;
            for (auto &e : enemyTiles){
                if (std::abs(e.x - t.x) + std::abs(e.y - t.y) <= r) ++c;
            }
            return c;
        };

        // 3. Utilidad de muestreo (Rejection sampling)
        // Intenta 'attempts' veces encontrar un punto al azar que cumpla 'predicate'.
        auto randomTileMatching = [&](auto&& predicate, int attempts=500)->std::optional<IVec2>{
            std::uniform_int_distribution<int> X(0, width-1), Y(0, height-1);
            for(int i=0;i<attempts;i++){
                IVec2 t{X(rng), Y(rng)};
                if (predicate(t)) return t;
            }
            return std::nullopt; // Falló tras N intentos
        };

        // 4. Colocadores (Placers)
        // A) La Llave Maestra (Complejo: Constraint Relaxation)
        auto placeLlave = [&](){
            int relax = 0;
            // Bucle de relajación: Si no encontramos sitio perfecto, bajamos las exigencias
            while (relax <= cfg.relaxPasos){
                int needEnemies = std::max(0, cfg.llaveMinEnemies - relax);
                int needStart = std::max(0, cfg.llaveMinDistSpawn - relax*8); // Cada paso reduce 8 tiles de distancia requerida
                int needExit  = std::max(0, cfg.llaveMinDistSalida - relax*6);

                auto cand = randomTileMatching([&](IVec2 t){
                    if (!reachable(t)) return false;
                    // Usamos los mapas BFS precalculados aquí:
                    if (distSpawn[id(t.x,t.y)] < needStart) return false; // Muy cerca del inicio
                    if (distExit[id(t.x,t.y)]  < needExit ) return false; // Muy cerca de la salida
                    if (!isFarFromOtherItems(t, cfg.minSepEntreItems)) return false;
                    if (countEnemiesNear(t, cfg.llaveEnemyRadius) < needEnemies) return false; // Poca protección
                    return true;
                }, 2000); // 2000 intentos por nivel de relax

                if (cand){
                    out.push_back({ItemType::LlaveMaestra, *cand, nivel, 0});
                    markOccupied(*cand);
                    return true;
                }
                ++relax; // Si falló, relajar restricciones y reintentar
            }
            return false; // Mapa imposible (muy raro)
        };

        // B) Batería (Solo Nivel 3, una vez)
        auto placeBateriaIfNeeded = [&](){
            if (nivel != 3 || run.batterySpawned) return;
            auto cand = randomTileMatching([&](IVec2 t){
                if (!reachable(t)) return false;
                if (distSpawn[id(t.x,t.y)] < cfg.bateriaMinDistSpawn) return false;
                if (!isFarFromOtherItems(t, cfg.minSepEntreItems)) return false;
                return true;
            }, 1500);
            if (cand){
                out.push_back({ItemType::BateriaVidaExtra, *cand, nivel, 0});
                markOccupied(*cand);
                run.batterySpawned = true; // Marcar globalmente como spawneada
            }
        };

        // C) Armas (Espada / Plasma)
        auto placeArma = [&](bool espada){
            // Lógica de disponibilidad por nivel
            if (espada){
                if (nivel < 1 || nivel > 3) return;
            } else {
                if (nivel < 2 || nivel > 3) return; // Plasma solo desde N2
            }
            int tierSugerido = espada ? nivel : (nivel==2 ? 1 : 2);
            
            auto cand = randomTileMatching([&](IVec2 t){
                return reachable(t) && isFarFromOtherItems(t, cfg.minSepEntreItems);
            }, 1000);
            
            if (cand){
                out.push_back({espada ? ItemType::EspadaPickup : ItemType::PistolaPlasmaPickup,
                               *cand, nivel, tierSugerido});
                markOccupied(*cand);
            }
        };

        // D) Pilas (Consumibles comunes)
        auto placePilas = [&](int n){
            std::bernoulli_distribution good(cfg.probPilaBuena); // RNG booleano ponderado
            for(int i=0;i<n;i++){
                auto cand = randomTileMatching([&](IVec2 t){
                    return reachable(t) && isFarFromOtherItems(t, cfg.minSepEntreItems);
                }, 600);
                if (!cand) break;
                
                bool esBuena = good(rng);
                out.push_back({esBuena ? ItemType::PilaBuena : ItemType::PilaMala,
                               *cand, nivel, 0});
                markOccupied(*cand);
            }
        };

        // E) Escudos
        auto placeEscudos = [&](int n){
            for(int i=0;i<n;i++){
                auto cand = randomTileMatching([&](IVec2 t){
                    return reachable(t) && isFarFromOtherItems(t, cfg.minSepEntreItems);
                }, 600);
                if (!cand) break;
                out.push_back({ItemType::Escudo, *cand, nivel, 0});
                markOccupied(*cand);
            }
        };

        // F) Gafas (Visión / Ceguera)
        auto placeGafas = [&](int n){
            std::bernoulli_distribution buenas(cfg.probGafasBuenas);
            for(int i=0;i<n;i++){
                auto cand = randomTileMatching([&](IVec2 t){
                    return reachable(t) && isFarFromOtherItems(t, cfg.minSepEntreItems);
                }, 600);
                if (!cand) break;
                bool esBuena = buenas(rng);
                out.push_back({esBuena ? ItemType::Gafas3DBuenas : ItemType::Gafas3DMalas,
                               *cand, nivel, 0});
                markOccupied(*cand);
            }
        };

        // 5. Ejecución (Pipeline)
        // El orden importa: Primero colocamos lo más difícil/restrictivo (Llave)
        // y al final rellenamos huecos con lo común.
        
        placeLlave();
        placeArma(true);   // Espada
        placeArma(false);  // Plasma (si aplica)
        placeBateriaIfNeeded();

        // Spawneamos items comunes según la cuota del nivel
        if (nivel==1){
            placePilas(cfg.pilasN1);
            placeEscudos(cfg.escudosN1);
            placeGafas(cfg.gafasN1);
        } else if (nivel==2){
            placePilas(cfg.pilasN2);
            placeEscudos(cfg.escudosN2);
            placeGafas(cfg.gafasN2);
        } else { // nivel==3
            placePilas(cfg.pilasN3);
            placeEscudos(cfg.escudosN3);
            placeGafas(cfg.gafasN3);
        }

        return out;
    }
};
