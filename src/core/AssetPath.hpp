#pragma once
#include <filesystem>
#include <string>
#include <iostream>

inline std::string assetPath(const std::string& rel) {
    namespace fs = std::filesystem;
    const fs::path ROOT = fs::path(RB_ASSET_ROOT);

    // 1) /usr/share/roguebot/assets/<rel>  (o "assets/<rel>" en dev)
    fs::path p1 = ROOT / rel;
    if (fs::exists(p1)) return p1.string();

    // 2) /usr/share/roguebot/<rel>  (por si en código ya pones "assets/...")
    fs::path p2 = ROOT.parent_path() / rel;
    if (fs::exists(p2)) return p2.string();

    // 3) /usr/share/roguebot/assets/sprites/<rel>  (por si usas "player/..."/"enemies/...")
    fs::path p3 = ROOT / "sprites" / rel;
    if (fs::exists(p3)) return p3.string();

    // 4) /usr/share/roguebot/assets/assets/<rel> (defensivo)
    fs::path p4 = ROOT / "assets" / rel;
    if (fs::exists(p4)) return p4.string();

    std::cerr << "[ASSETS] No encontrado: " << rel
              << " (probado: " << p1 << ", " << p2 << ", " << p3 << ", " << p4 << ")\n";
    return p1.string(); // fallback
}
