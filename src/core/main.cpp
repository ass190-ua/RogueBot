#include "Game.hpp"
#include <cstdlib>      // strtoul
#include <iostream>
#include <filesystem>

static bool dirExists(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec) && std::filesystem::is_directory(p, ec);
}

int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;
        const fs::path ROOT = fs::path(RB_ASSET_ROOT);

        const bool looksLikeNoPrefix   = dirExists(ROOT / "sprites") || dirExists(ROOT / "tiles");
        const bool looksLikeWithPrefix = dirExists(ROOT / "assets" / "sprites") || dirExists(ROOT / "assets" / "tiles");

        if (looksLikeNoPrefix) {
            fs::current_path(ROOT);
            std::cout << "[ASSETS] cwd = " << fs::current_path() << " (sin prefijo 'assets/')\n";
        } else if (looksLikeWithPrefix) {
            fs::current_path(ROOT.parent_path());
            std::cout << "[ASSETS] cwd = " << fs::current_path() << " (con prefijo 'assets/')\n";
        } else {
            // Fallback razonable: usar RB_ASSET_ROOT directamente
            fs::current_path(ROOT);
            std::cout << "[ASSETS] cwd = " << fs::current_path()
                      << " (fallback: revisa que existan 'sprites' o 'assets/sprites')\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERR] No se pudo fijar cwd a RB_ASSET_ROOT: " << e.what() << "\n";
        // seguimos igualmente; si tus rutas son absolutas funcionará
    }

    // 2) Seed por CLI (opcional)
    unsigned seed = 0;
    if (argc > 1) {
        seed = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
        std::cout << "[CLI] Seed fija: " << seed << "\n";
    } else {
        std::cout << "[CLI] Sin seed -> aleatoria por ejecución\n";
    }

    Game g(seed);
    g.run();
    return 0;
}
