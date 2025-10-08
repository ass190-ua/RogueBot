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

struct IVec2 { int x{}, y{}; };

enum class ItemType : uint8_t {
    PilaBuena, PilaMala,
    Escudo,
    Gafas3DBuenas, Gafas3DMalas,
    BateriaVidaExtra,            // solo N3 y una vez por run
    EspadaPickup,                // tierSugerido por nivel
    PistolaPlasmaPickup,         // tierSugerido por nivel
    LlaveMaestra                 // 1 por nivel
};

struct ItemSpawn {
    ItemType type;
    IVec2    tile;          // en coordenadas de celda (no píxeles)
    int      nivel;         // 1..3
    int      tierSugerido;  // 0 si no aplica (pila, escudo, etc.)
};

struct RunContext {
    bool batterySpawned = false; // persiste entre niveles
    int espadaMejorasObtenidas = 0;     // 0..2
    int plasmaMejorasObtenidas = 0;     // 0..1
};

struct SpawnConfig {
    // Cuotas por nivel
    int pilasN1 = 3, pilasN2 = 3, pilasN3 = 4;
    int escudosN1 = 1, escudosN2 = 2, escudosN3 = 2;
    int gafasN1 = 1, gafasN2 = 1, gafasN3 = 2;

    // Probabilidades variantes
    double probPilaBuena = 0.7; // 70% buenas, 30% malas
    double probGafasBuenas = 0.5;

    // Separaciones y distancias (BFS)
    int minSepEntreItems = 4;
    int llaveMinDistSpawn = 30;
    int llaveMinDistSalida = 20;
    int llaveEnemyRadius   = 6;
    int llaveMinEnemies    = 2;

    // Batería
    int bateriaMinDistSpawn = 25;

    // Relax de restricciones
    int relaxPasos = 3;
};

class ItemSpawner {
public:
    using IsWalkableFn = std::function<bool(int,int)>;

    static std::vector<ItemSpawn> generate(
        int width, int height,
        const IsWalkableFn& isWalkable,
        IVec2 spawnTile,
        IVec2 exitTile,
        const std::vector<IVec2>& enemyTiles,
        int nivel,                 // 1..3
        std::mt19937& rng,
        RunContext& run,
        const SpawnConfig& cfg = {}
    )
    {
        std::vector<ItemSpawn> out;
        if (nivel < 1 || nivel > 3) return out;

        auto inBounds = [&](int x, int y){ return x>=0 && y>=0 && x<width && y<height; };

        // --- BFS desde spawn y desde salida ---
        const int INF = std::numeric_limits<int>::max()/4;
        auto bfsDist = [&](IVec2 source) {
            std::vector<int> dist(width*height, INF);
            std::queue<IVec2> q;
            auto idLocal = [&](int x,int y){ return y*width + x; };
            if (inBounds(source.x, source.y) && isWalkable(source.x, source.y)) {
                dist[idLocal(source.x, source.y)] = 0;
                q.push(source);
            }
            const int dx[4] = {1,-1,0,0};
            const int dy[4] = {0,0,1,-1};
            while(!q.empty()){
                auto p = q.front(); q.pop();
                int d = dist[idLocal(p.x,p.y)];
                for(int k=0;k<4;k++){
                    int nx=p.x+dx[k], ny=p.y+dy[k];
                    if (inBounds(nx,ny) && isWalkable(nx,ny)) {
                        int &ref = dist[idLocal(nx,ny)];
                        if (ref==INF){
                            ref = d+1;
                            q.push({nx,ny});
                        }
                    }
                }
            }
            return dist;
        };

        auto distSpawn = bfsDist(spawnTile);
        auto distExit  = bfsDist(exitTile);
        auto id = [&](int x,int y){ return y*width + x; };

        auto reachable = [&](IVec2 t){
            if (!inBounds(t.x,t.y) || !isWalkable(t.x,t.y)) return false;
            return distSpawn[id(t.x,t.y)] != INF;
        };

        // Ocupación para mantener separación
        std::vector<uint8_t> occupied(width*height, 0);
        auto markOccupied = [&](IVec2 t){
            if (inBounds(t.x,t.y)) occupied[id(t.x,t.y)] = 1;
        };
        auto isFarFromOtherItems = [&](IVec2 t, int minSep){
            if (!inBounds(t.x,t.y)) return false;
            for (int dy=-minSep; dy<=minSep; ++dy){
                for (int dx=-minSep; dx<=minSep; ++dx){
                    int nx=t.x+dx, ny=t.y+dy;
                    if (inBounds(nx,ny) && occupied[id(nx,ny)]) {
                        if (std::abs(dx)+std::abs(dy) <= minSep) return false; // Manhattan
                    }
                }
            }
            return true;
        };

        auto countEnemiesNear = [&](IVec2 t, int radius){
            int c=0;
            int r=radius;
            for (auto &e : enemyTiles){
                if (std::abs(e.x - t.x) + std::abs(e.y - t.y) <= r) ++c;
            }
            return c;
        };

        // Elegir tile aleatorio que cumpla un predicado
        auto randomTileMatching = [&](auto&& predicate, int attempts=500)->std::optional<IVec2>{
            std::uniform_int_distribution<int> X(0, width-1), Y(0, height-1);
            for(int i=0;i<attempts;i++){
                IVec2 t{X(rng), Y(rng)};
                if (predicate(t)) return t;
            }
            return std::nullopt;
        };

        // --- Colocadores ---
        auto placeLlave = [&](){
            int relax = 0;
            while (relax <= cfg.relaxPasos){
                int needEnemies = std::max(0, cfg.llaveMinEnemies - relax);
                int needStart = std::max(0, cfg.llaveMinDistSpawn - relax*8);
                int needExit  = std::max(0, cfg.llaveMinDistSalida - relax*6);

                auto cand = randomTileMatching([&](IVec2 t){
                    if (!reachable(t)) return false;
                    if (distSpawn[id(t.x,t.y)] < needStart) return false;
                    if (distExit[id(t.x,t.y)]  < needExit ) return false;
                    if (!isFarFromOtherItems(t, cfg.minSepEntreItems)) return false;
                    if (countEnemiesNear(t, cfg.llaveEnemyRadius) < needEnemies) return false;
                    return true;
                }, 2000);

                if (cand){
                    out.push_back({ItemType::LlaveMaestra, *cand, nivel, 0});
                    markOccupied(*cand);
                    return true;
                }
                ++relax;
            }
            return false;
        };

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
                run.batterySpawned = true;
            }
        };

        auto placeArma = [&](bool espada){
            // Espada: N1,N2,N3. Plasma: N2,N3.
            if (espada){
                if (nivel < 1 || nivel > 3) return;
            } else {
                if (nivel < 2 || nivel > 3) return;
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

        auto placePilas = [&](int n){
            std::bernoulli_distribution good(cfg.probPilaBuena);
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

        // --- Orden de colocación ---
        placeLlave();
        placeArma(true);   // espada
        placeArma(false);  // plasma (si aplica)
        placeBateriaIfNeeded();

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
