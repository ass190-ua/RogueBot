#include <iostream>
#include <sys/stat.h>

static bool fileExists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

int main() {
  const char *enMo = "assets/locales/en_GB/LC_MESSAGES/roguebot.mo";
  const char *esMo = "assets/locales/es_ES/LC_MESSAGES/roguebot.mo";

  int fails = 0;

  if (!fileExists(enMo)) {
    std::cerr << "[FAIL] No existe: " << enMo << "\n";
    fails++;
  }

  if (!fileExists(esMo)) {
    std::cerr << "[FAIL] No existe: " << esMo << "\n";
    fails++;
  }

  if (fails == 0) {
    std::cout << "[OK] Existen los catálogos .mo en en_GB y es_ES\n";
    return 0;
  }
  return 1;
}
