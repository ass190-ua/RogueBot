#include "Game.hpp"
#include "GettextCompat.hpp"
#include <cstdlib>    // strtoul (String to Unsigned Long)
#include <filesystem> // C++17: Manejo moderno de rutas y directorios
#include <iostream>
#include <locale.h>

// Helper: Verificación segura de directorios
// Comprueba si una ruta existe y es una carpeta.
// Usa 'std::error_code' para evitar que el programa se cierre (lance excepción)
// si hay problemas de permisos raros en el sistema operativo.
static bool dirExists(const std::filesystem::path &p) {
  std::error_code ec;
  return std::filesystem::exists(p, ec) && std::filesystem::is_directory(p, ec);
}

int main(int argc, char **argv) {
  // 1. Configuración del directorio de trabajo (CWD)
  // Objetivo: Asegurar que cuando el juego pida cargar "sprites/player.png",
  // el sistema operativo sepa dónde buscar, independientemente de si ejecutamos
  // el juego desde VSCode, desde la terminal o desde un instalador.
  try {
    namespace fs = std::filesystem;

    // RB_ASSET_ROOT viene del Makefile/CMake. Si no, usa el default.
    const fs::path ROOT = fs::path(RB_ASSET_ROOT);

    // Detectamos la estructura de carpetas:
    // Caso A: ROOT/sprites (Estructura plana, típica en desarrollo local)
    const bool looksLikeNoPrefix =
        dirExists(ROOT / "sprites") || dirExists(ROOT / "tiles");

    // Caso B: ROOT/assets/sprites (Estructura anidada, común en distribuciones)
    const bool looksLikeWithPrefix = dirExists(ROOT / "assets" / "sprites") ||
                                     dirExists(ROOT / "assets" / "tiles");

    if (looksLikeNoPrefix) {
      // Estamos justo encima de las carpetas de recursos
      fs::current_path(ROOT);
      std::cout << "[ASSETS] cwd = " << fs::current_path()
                << " (sin prefijo 'assets/')\n";
    } else if (looksLikeWithPrefix) {
      // Estamos un nivel arriba, o dentro de un 'bin' hermano de 'assets'.
      // Ajustamos el CWD al padre para que las rutas relativas cuadren.
      fs::current_path(ROOT.parent_path());
      std::cout << "[ASSETS] cwd = " << fs::current_path()
                << " (con prefijo 'assets/')\n";
    } else {
      // Fallback: Confiamos ciegamente en lo que diga el Makefile
      fs::current_path(ROOT);
      std::cout
          << "[ASSETS] cwd = " << fs::current_path()
          << " (fallback: revisa que existan 'sprites' o 'assets/sprites')\n";
    }
  } catch (const std::exception &e) {
    // Si fallan los permisos, avisamos pero intentamos arrancar igual
    std::cerr << "[ERR] No se pudo fijar cwd a RB_ASSET_ROOT: " << e.what()
              << "\n";
  }

  // i18n (gettext)
  setlocale(LC_ALL, "");

  namespace fs = std::filesystem;
  fs::path localesDir;

  if (dirExists(fs::current_path() / "locales")) {
    localesDir = fs::current_path() / "locales";
  } else if (dirExists(fs::current_path() / "assets" / "locales")) {
    localesDir = fs::current_path() / "assets" / "locales";
  } else {
    localesDir = fs::current_path() / "locales";
  }

  const std::string localesDirStr = localesDir.string();
  bindtextdomain("roguebot", localesDirStr.c_str());
  bind_textdomain_codeset("roguebot", "UTF-8");
  textdomain("roguebot");

  std::cout << "[I18N] locales = " << localesDirStr << "\n";

  // 2. Gestión de la semilla (Seed)
  // Esto es vital para depurar generación procedural.
  // Si se ejecuta "./roguebot 12345", siempre se generará la misma mazmorra.
  // Si se ejecuta "./roguebot", será aleatoria cada vez.

  unsigned seed = 0;
  if (argc > 1) {
    // Convertir argumento de texto a número (base 10)
    seed = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
    std::cout << "[CLI] Seed fija: " << seed << "\n";
  } else {
    std::cout << "[CLI] Sin seed -> aleatoria por ejecución\n";
  }

  // 3. Inicio del juego
  // Pasamos la seed al constructor para inicializar el RNG
  Game g(seed);
  g.run(); // Bucle principal (Game Loop)

  return 0;
}
