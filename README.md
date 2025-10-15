# 🤖 RogueBot

RogueBot es un proyecto desarrollado en C++ utilizando [raylib](https://www.raylib.com/) como motor gráfico.  
Actualmente en fase **Alpha (Hito 1)**, el objetivo es crear un _roguelike_ con estética retro, jugabilidad fluida y un sistema modular de entidades.

---

## 🚀 Estado del Proyecto
- **Versión:** Alpha – Hito 1  
- **Ramas principales:**
  - `main`: versiones estables (releases)
  - `develop`: integración continua
  - `feature/*`: ramas de desarrollo por issue/tarea

---

## 🛠️ Requisitos del entorno

### 🔹 Linux
Se recomienda Ubuntu 22.04 o superior.  
Asegúrate de tener las siguientes dependencias:

```bash
sudo apt update
sudo apt install -y --no-install-recommends   build-essential cmake pkg-config   libx11-dev libxrandr-dev libxi-dev libxxf86vm-dev   libxinerama-dev libxcursor-dev   libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev   libasound2-dev
```

> 🧩 Estas librerías permiten compilar y ejecutar correctamente raylib y RogueBot, tanto en local como en el CI de GitHub Actions.

---

## ⚙️ Compilación y ejecución

Clona el repositorio:

```bash
git clone https://github.com/ass190-ua/RogueBot
cd RogueBot
```

### 1️⃣ Configura el proyecto
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

### 2️⃣ Compila el ejecutable
```bash
cmake --build build --config Release
```

### 3️⃣ Ejecuta el juego
```bash
./build/bin/roguebot
```

> Si no existe el directorio `bin/`, se generará automáticamente tras la compilación.

---

## 🧱 Estructura del proyecto

```
RogueBot/
│
├── assets/                     # Recursos del juego
│   ├── docs/                   # Documentación auxiliar
│   └── sprites/                # Sprites organizados por tipo
│       ├── enemies/            # Sprites de enemigos
│       ├── items/              # Sprites de ítems (armas, llaves, etc.)
│       └── player/             # Sprites del jugador (idle y animaciones)
│
├── build/                      # Archivos generados tras la compilación
│   └── roguebot                # Ejecutable principal
│
├── docs/                       # Documentación general del proyecto
│   └── GDD.md                  # Game Design Document
│
├── src/                        # Código fuente del juego
│   ├── Attack.hpp
│   ├── Enemy.cpp/.hpp
│   ├── Game.cpp/.hpp
│   ├── HUD.cpp/.hpp
│   ├── ItemSpawner.hpp
│   ├── Map.cpp/.hpp
│   ├── Player.cpp/.hpp
│   └── State.cpp/.hpp
│
├── CMakeLists.txt              # Configuración del build con CMake
├── LICENSE                     # Licencia MIT
├── README.md                   # Este archivo
└── .github/workflows/ci.yml    # CI para compilación automática en GitHub

```

---

## 🧩 Integración continua (CI)

El flujo de integración se gestiona automáticamente mediante **GitHub Actions**:  
- Compila el proyecto con `CMake` y raylib.
- Ejecuta verificaciones básicas de build en Ubuntu.
- Sube el binario como artefacto descargable.

📄 Archivo del workflow: `.github/workflows/ci.yml`

---

## 💡 Desarrollo y contribución

1. Crea una rama a partir de `develop`:
   ```bash
   git checkout develop
   git pull
   git checkout -b feature/<nombre-de-la-tarea>
   ```

2. Implementa los cambios y haz commit:
   ```bash
   git add .
   git commit -m "feat: descripción breve del cambio"
   ```

3. Sube la rama y crea un Pull Request en GitHub:
   ```bash
   git push -u origin feature/<nombre-de-la-tarea>
   ```

> 🔖 Para tareas de mantenimiento, usa el prefijo `chore/`, por ejemplo:  
> `chore/update-cmake-ci`

---

## 🎮 Notas técnicas

- Compilador mínimo: **g++ 9.0+** o **clang++ 10+**
- Estándar de C++: **C++17**
- Librería gráfica: **raylib 5.0**
- Generador de build: **CMake ≥ 3.16**
- Sistema operativo compatible: Linux (Ubuntu recomendado)

> 🧰 El sistema de build detecta automáticamente todos los archivos `.cpp` dentro de `src/`  
> gracias a `file(GLOB_RECURSE ...)` en `CMakeLists.txt`.

---

## 🧑‍💻 Equipo de desarrollo

| Usuario                                        | Nombre                 |
| ---------------------------------------------- | ---------------------- |
| [**@ass190-ua**](https://github.com/ass190-ua) | Arturo Soriano Sánchez |
| [**@psm97-ua**](https://github.com/psm97-ua)   | Paula Soriano Muñoz                 |
| [**@rla28-ua**](https://github.com/rla28-ua)   | Raúl López Arpa                   |


---

## 📜 Licencia

Este proyecto se distribuye bajo la licencia **MIT**.  
Consulta el archivo `LICENSE` para más detalles.

---
