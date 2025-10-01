# RogueBot

Alpha del proyecto (Hito 1).

## Ramas
- `main`: releases estables
- `develop`: integración diaria
- `feature/*`: trabajo por tarea/issue

## Requisitos (Linux)
- CMake ≥ 3.16
- Compilador C++17 (g++/clang++)

## Build y ejecución
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/roguebot
