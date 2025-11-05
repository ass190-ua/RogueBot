#include "Game.hpp"
#include <cstdlib>   // strtoul
#include <iostream>

int main(int argc, char** argv) {
    // Seed por CLI (opcional). Si no se pasa, será aleatoria.
    unsigned seed = 0;
    if (argc > 1) {
        seed = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
        std::cout << "[CLI] Seed fija: " << seed << "\n";
    } else {
        std::cout << "[CLI] Sin seed -> aleatoria por ejecución\n";
    }

    Game g(seed);   // si seed==0 => el propio Game usará aleatoria
    g.run();
    return 0;
}
