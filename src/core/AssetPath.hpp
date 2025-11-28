#pragma once
#include <filesystem> // Requiere C++17: Para manejar rutas de forma agnóstica al SO (Windows/Linux)
#include <string>
#include <iostream>

// Configuración de ruta base
// RB_ASSET_ROOT se definine en el Makefile o CMake
// Si no está definido (estamos programando en local), usamos "assets" relativo.
#ifndef RB_ASSET_ROOT
#define RB_ASSET_ROOT "assets"
#endif

// 'inline' permite incluir este archivo en múltiples .cpp sin errores de "símbolo duplicado" al enlazar.
inline std::string assetPath(const std::string& rel) {
    namespace fs = std::filesystem; // Alias para escribir menos

    // Fase 0: Entorno de desarrollo (Búsqueda relativa)
    // El problema: ¿Desde dónde se ejecutó el juego?
    // 1. "."       -> Desde la raíz del proyecto (ideal)
    // 2. ".."      -> Desde una carpeta build/ (ej: make)
    // 3. "../.."   -> Desde build/bin/ o build/debug/ (común en IDEs)
    const fs::path roots[] = { fs::path("."), fs::path(".."), fs::path("../..") };

    for (const auto& r : roots) {
        fs::path baseAssets = r / "assets"; // Construye ruta: "./assets", "../assets", etc.

        // Heurística A: Estructura estándar
        // Buscamos: raiz/assets/sprites/enemy.png
        fs::path c1 = baseAssets / rel;
        if (fs::exists(c1)) return c1.string();

        // Heurística B: El usuario ya pasó "assets/" en la string 'rel'
        // Buscamos: raiz/assets/sprites/enemy.png (si rel era "assets/sprites/...")
        fs::path c2 = r / rel;
        if (fs::exists(c2)) return c2.string();

        // Heurística C: Atajo (Shortcut)
        // El usuario pide "enemy.png" y nosotros asumimos que está en "sprites/"
        // Buscamos: raiz/assets/sprites/enemy.png
        fs::path c3 = baseAssets / "sprites" / rel;
        if (fs::exists(c3)) return c3.string();

        // Heurística D: Defensiva (Error común de anidación)
        // Por si alguien creó la carpeta "assets" dentro de "assets".
        fs::path c4 = baseAssets / "assets" / rel;
        if (fs::exists(c4)) return c4.string();
    }

    // Fase 1: Entorno de instalación (Rutas absolutas)
    // Si la búsqueda relativa falló, asumimos que el juego está instalado en el sistema
    // y buscamos en la ruta definida durante la compilación (RB_ASSET_ROOT).
    
    const fs::path ROOT = fs::path(RB_ASSET_ROOT);

    // 1. Búsqueda directa en la ruta de instalación
    fs::path p1 = ROOT / rel;
    if (fs::exists(p1)) return p1.string();

    // 2. Búsqueda en el padre (por si RB_ASSET_ROOT apunta a /bin en lugar de /share)
    fs::path p2 = ROOT.parent_path() / rel;
    if (fs::exists(p2)) return p2.string();

    // 3. Búsqueda con atajo a "sprites"
    fs::path p3 = ROOT / "sprites" / rel;
    if (fs::exists(p3)) return p3.string();

    // 4. Búsqueda defensiva
    fs::path p4 = ROOT / "assets" / rel;
    if (fs::exists(p4)) return p4.string();

    // Fase 2: Fallo Total (Logging)
    // Si llegamos aquí, el archivo no existe en ningún lado.
    // Imprimimos un log detallado para ayudar al programador a saber qué falló.
    std::cerr << "[ASSETS] ERROR CRÍTICO: No encontrado: " << rel << "\n"
              << "  > Se probó modo Desarrollo (., .., ../..) con variantes:\n"
              << "    assets/<archivo>\n"
              << "    <archivo>\n"
              << "    assets/sprites/<archivo>\n"
              << "  > Se probó modo Instalado (ROOT = " << ROOT << "):\n"
              << "    " << p1 << "\n"
              << "    " << p2 << "\n"
              << "    " << p3 << "\n"
              << "    " << p4 << "\n";

    // Devolvemos p1 por defecto (aunque no exista) para que falle más adelante
    // al intentar cargar la textura, o para evitar crash inmediato aquí.
    return p1.string(); 
}
