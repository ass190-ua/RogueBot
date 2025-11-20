#pragma once
#include <filesystem>
#include <string>
#include <iostream>

// Si el macro no viene definido desde el Makefile,
// damos un valor por defecto para desarrollo.
#ifndef RB_ASSET_ROOT
#define RB_ASSET_ROOT "assets"
#endif

inline std::string assetPath(const std::string& rel) {
    namespace fs = std::filesystem;

    // ===========================
    // 0) Entorno de desarrollo
    // ===========================
    // Vamos a probar varios "root" posibles por si el binario se ejecuta desde:
    //   - la raíz del repo      (.)
    //   - build_gnu/            (..)
    //   - build_gnu/bin/        (../..)
    const fs::path roots[] = { fs::path("."), fs::path(".."), fs::path("../..") };

    for (const auto& r : roots) {
        fs::path baseAssets = r / "assets";

        // a) assets/<rel>   (ej: assets/sprites/enemies/enemy.png)
        fs::path c1 = baseAssets / rel;
        if (fs::exists(c1)) return c1.string();

        // b) <rel> tal cual desde ese root
        //    (ej: rel ya viene como "assets/sprites/enemy.png")
        fs::path c2 = r / rel;
        if (fs::exists(c2)) return c2.string();

        // c) assets/sprites/<rel>   (ej: rel = "player/robot_down_idle.png")
        fs::path c3 = baseAssets / "sprites" / rel;
        if (fs::exists(c3)) return c3.string();

        // d) assets/assets/<rel>  (defensivo, por si alguien pasó "assets/..." ya)
        fs::path c4 = baseAssets / "assets" / rel;
        if (fs::exists(c4)) return c4.string();
    }

    // ===========================================
    // 1) Rutas basadas en RB_ASSET_ROOT (instalado)
    // ===========================================
    const fs::path ROOT = fs::path(RB_ASSET_ROOT);

    // 1) RB_ASSET_ROOT/<rel>
    fs::path p1 = ROOT / rel;
    if (fs::exists(p1)) return p1.string();

    // 2) parent(RB_ASSET_ROOT)/<rel>
    fs::path p2 = ROOT.parent_path() / rel;
    if (fs::exists(p2)) return p2.string();

    // 3) RB_ASSET_ROOT/sprites/<rel>
    fs::path p3 = ROOT / "sprites" / rel;
    if (fs::exists(p3)) return p3.string();

    // 4) RB_ASSET_ROOT/assets/<rel> (defensivo)
    fs::path p4 = ROOT / "assets" / rel;
    if (fs::exists(p4)) return p4.string();

    // ===========================
    // Fallback + log de error
    // ===========================
    std::cerr << "[ASSETS] No encontrado: " << rel << "\n"
              << "  Probado (dev) con roots ., .., ../.. y combinaciones de:\n"
              << "    assets/<rel>\n"
              << "    <rel>\n"
              << "    assets/sprites/<rel>\n"
              << "    assets/assets/<rel>\n"
              << "  Probado (ROOT = " << ROOT << "):\n"
              << "    " << p1 << "\n"
              << "    " << p2 << "\n"
              << "    " << p3 << "\n"
              << "    " << p4 << "\n";

    return p1.string(); // último recurso
}
