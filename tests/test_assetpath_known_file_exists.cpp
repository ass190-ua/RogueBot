#include <iostream>
#include <string>
#include <sys/stat.h>

#if __has_include("AssetPath.hpp")
#include "AssetPath.hpp"
#elif __has_include("assetPath.hpp")
#include "assetPath.hpp"
#elif __has_include("Assets.hpp")
#include "Assets.hpp"
#elif __has_include("assets/AssetPath.hpp")
#include "assets/AssetPath.hpp"
#else
#error "No se encuentra el header de assetPath (por ejemplo AssetPath.hpp)."
#endif

static bool fileExists(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

int main() {
  const std::string rel = "locales/es_ES/LC_MESSAGES/roguebot.mo";

  const std::string abs = assetPath(rel);

  if (abs.empty()) {
    std::cerr << "[FAIL] assetPath devolvió string vacío para: " << rel << "\n";
    return 1;
  }

  if (!fileExists(abs)) {
    std::cerr << "[FAIL] assetPath no resuelve a un fichero existente\n";
    std::cerr << "  rel: " << rel << "\n";
    std::cerr << "  abs: " << abs << "\n";
    return 1;
  }

  std::cout << "[OK] assetPath resuelve recurso existente: " << abs << "\n";
  return 0;
}
