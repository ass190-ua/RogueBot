#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <libintl.h>

#ifndef ROGUEBOT_SOURCE_DIR
#define ROGUEBOT_SOURCE_DIR "."
#endif

static void set_env_var(const char* key, const char* value)
{
#ifdef _WIN32
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
}

static uint32_t bswap32(uint32_t x)
{
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0xFF000000u) >> 24);
}

struct MoPair { std::string msgid; std::string msgstr; };

// Lee el primer par (msgid != msgstr), sin plurales (evita '\0' en msgid)
static bool read_first_translation_pair_from_mo(const std::filesystem::path& moPath, MoPair& out)
{
    std::ifstream f(moPath, std::ios::binary);
    if (!f) return false;

    auto read_u32_at = [&](std::streamoff off, bool swap) -> uint32_t {
        f.seekg(off);
        uint32_t v = 0;
        f.read(reinterpret_cast<char*>(&v), sizeof(v));
        return swap ? bswap32(v) : v;
    };

    // Header .mo
    uint32_t magic = read_u32_at(0, false);
    bool swap = false;
    if (magic == 0x950412deu) {
        swap = false; // little endian
    } else if (magic == 0xde120495u) {
        swap = true;  // big endian
    } else {
        return false;
    }

    uint32_t revision   = read_u32_at(4,  swap);
    uint32_t nstrings   = read_u32_at(8,  swap);
    uint32_t off_orig   = read_u32_at(12, swap);
    uint32_t off_trans  = read_u32_at(16, swap);
    (void)revision;

    auto read_str = [&](uint32_t off, uint32_t len) -> std::string {
        std::string s(len, '\0');
        f.seekg(off);
        f.read(&s[0], len);
        return s;
    };

    for (uint32_t i = 0; i < nstrings; ++i) {
        uint32_t olen = read_u32_at(off_orig  + i * 8 + 0, swap);
        uint32_t ooff = read_u32_at(off_orig  + i * 8 + 4, swap);
        uint32_t tlen = read_u32_at(off_trans + i * 8 + 0, swap);
        uint32_t toff = read_u32_at(off_trans + i * 8 + 4, swap);

        if (olen == 0 || tlen == 0) continue;

        std::string msgid  = read_str(ooff, olen);
        std::string msgstr = read_str(toff, tlen);

        if (msgid.empty()) continue;                 // header
        if (msgid.find('\0') != std::string::npos) continue; // plural => skip
        if (msgid == msgstr) continue;               // no traducción real

        out.msgid = msgid;
        out.msgstr = msgstr;
        return true;
    }

    return false;
}

static void force_language_en_gb()
{
    // Forzamos idioma para gettext. No dependemos de que el locale esté instalado.
    set_env_var("LANGUAGE",   "en_GB");
    set_env_var("LC_ALL",     "en_GB.UTF-8");
    set_env_var("LC_MESSAGES","en_GB.UTF-8");
    set_env_var("LANG",       "en_GB.UTF-8");
    setlocale(LC_ALL, "");
}

int main()
{
    namespace fs = std::filesystem;

    const fs::path root = fs::path(ROGUEBOT_SOURCE_DIR);
    const fs::path localeDir = root / "assets" / "locales";
    const fs::path mo_en = localeDir / "en_GB" / "LC_MESSAGES" / "roguebot.mo";

    if (!fs::exists(mo_en)) {
        std::cerr << "[FAIL] No existe el .mo esperado: " << mo_en << "\n";
        return 1;
    }

    // Cargamos dominio
    bindtextdomain("roguebot", localeDir.string().c_str());
    bind_textdomain_codeset("roguebot", "UTF-8");
    textdomain("roguebot");

    // Elegimos una clave REAL del .mo
    MoPair pair;
    if (!read_first_translation_pair_from_mo(mo_en, pair)) {
        std::cerr << "[FAIL] No se pudo extraer ningún par traducido del .mo: " << mo_en << "\n";
        return 1;
    }

    force_language_en_gb();

    const std::string got = std::string(dgettext("roguebot", pair.msgid.c_str()));
    if (got != pair.msgstr) {
        std::cerr << "[FAIL] gettext no traduce según el .mo\n";
        std::cerr << "  msgid:     " << pair.msgid << "\n";
        std::cerr << "  esperado:  " << pair.msgstr << "\n";
        std::cerr << "  obtenido:  " << got << "\n";
        std::cerr << "  locale:    " << (setlocale(LC_ALL, nullptr) ? setlocale(LC_ALL, nullptr) : "(null)") << "\n";
        return 1;
    }

    std::cout << "[OK] gettext traduce (en_GB): '" << pair.msgid << "' -> '" << got << "'\n";
    return 0;
}