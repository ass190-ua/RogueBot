#include "AssetPath.hpp"
#include <filesystem>
#include <iostream>

int main()
{
    namespace fs = std::filesystem;

    const std::string rel = "locales/es_ES/LC_MESSAGES/roguebot.mo";
    const std::string abs = assetPath(rel);

    if (!fs::exists(abs))
    {
        std::cerr << "[FAIL] assetPath no devuelve un fichero existente\n";
        std::cerr << "  rel: " << rel << "\n";
        std::cerr << "  abs: " << abs << "\n";
        return 1;
    }

    if (fs::file_size(abs) == 0)
    {
        std::cerr << "[FAIL] El fichero existe pero está vacío\n";
        std::cerr << "  abs: " << abs << "\n";
        return 1;
    }

    std::cout << "[OK] assetPath -> " << abs << "\n";
    return 0;
}
