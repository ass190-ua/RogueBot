#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <libintl.h>

static void set_env_var(const char* key, const char* value)
{
#ifdef _WIN32
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
}

static void force_non_C_locale()
{
    // IMPORTANTE: No usar "C" a secas, porque puede desactivar gettext.
    // "C.UTF-8" suele existir en Linux/CI. Si no, caemos a "" (entorno).
    set_env_var("LC_ALL", "C.UTF-8");
    set_env_var("LANG", "C.UTF-8");

    if (!setlocale(LC_ALL, "C.UTF-8"))
        setlocale(LC_ALL, ""); // fallback
}

static std::string tr_lang(const std::string& lang, const char* msgid)
{
    set_env_var("LANGUAGE", lang.c_str());
    force_non_C_locale();
    return std::string(dgettext("roguebot", msgid));
}

int main()
{
    namespace fs = std::filesystem;

    const fs::path localeDir = fs::current_path() / "assets" / "locales";
    if (!fs::exists(localeDir))
    {
        std::cerr << "[FAIL] No existe el directorio de locales: " << localeDir << "\n";
        return 1;
    }

    bindtextdomain("roguebot", localeDir.string().c_str());
    bind_textdomain_codeset("roguebot", "UTF-8");
    textdomain("roguebot");

    // Caso A: msgid en español -> en_GB debe traducir OPCIONES -> OPTIONS
    const std::string en_from_es = tr_lang("en_GB", "OPCIONES");
    if (en_from_es == "OPTIONS")
    {
        std::cout << "[OK] Base ES detectada: OPCIONES -> OPTIONS (en_GB)\n";
        return 0;
    }

    // Caso B: msgid en inglés -> es_ES debe traducir OPTIONS -> OPCIONES
    const std::string es_from_en = tr_lang("es_ES", "OPTIONS");
    if (es_from_en == "OPCIONES")
    {
        std::cout << "[OK] Base EN detectada: OPTIONS -> OPCIONES (es_ES)\n";
        return 0;
    }

    std::cerr << "[FAIL] No se ha detectado traducción en ninguno de los casos\n";
    std::cerr << "  en_GB dgettext('OPCIONES') => " << en_from_es << "\n";
    std::cerr << "  es_ES dgettext('OPTIONS')  => " << es_from_en << "\n";
    return 1;
}
