#include <filesystem>
#include <iostream>

int main() {
    namespace fs = std::filesystem;

    const fs::path base = fs::path("assets") / "locales";
    const fs::path es = base / "es_ES" / "LC_MESSAGES" / "roguebot.mo";
    const fs::path en = base / "en_GB" / "LC_MESSAGES" / "roguebot.mo";

    bool ok = true;

    auto check = [&](const fs::path& p) {
        if (!fs::exists(p)) {
            std::cerr << "[FAIL] No existe: " << p << "\n";
            ok = false;
            return;
        }
        if (fs::file_size(p) == 0) {
            std::cerr << "[FAIL] Está vacío: " << p << "\n";
            ok = false;
        }
    };

    check(es);
    check(en);

    if (ok) std::cout << "[OK] Catálogos .mo presentes y no vacíos.\n";
    return ok ? 0 : 1;
}
