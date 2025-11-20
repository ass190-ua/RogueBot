# 🤖 RogueBot

RogueBot es un proyecto desarrollado en C++ utilizando [raylib](https://www.raylib.com/) como motor gráfico.  
Actualmente se encuentra en **Hito 2 – Distribución del juego**, con sistema de compilación basado en **Makefile** y empaquetado en formato **.deb** para distribuciones Debian/Ubuntu.

---

## 🚀 Estado del Proyecto

- **Versión:** Alpha – Hito 2  
- **Ramas principales:**
  - `main`: versiones estables (releases)
  - `develop`: integración continua
  - `feature/*`: ramas de desarrollo por issue/tarea (por ejemplo `feature/gamepad`, `feature/refactoring`)

---

## 🛠️ Requisitos del entorno

### 🔹 Linux

Se recomienda Ubuntu 22.04 o superior (o distribución equivalente).  
Dependencias mínimas:

```bash
sudo apt update
sudo apt install -y --no-install-recommends   build-essential make pkg-config   libraylib-dev   libx11-dev libxrandr-dev libxi-dev libxxf86vm-dev   libxinerama-dev libxcursor-dev   libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev   libasound2-dev   ccache
```

> 🧩 Estas librerías permiten compilar y ejecutar correctamente raylib y RogueBot, tanto en local como en el CI de GitHub Actions.  
> `ccache` es opcional pero se utiliza en el Hito 2 para acelerar compilaciones repetidas.

---

## ⚙️ Compilación y ejecución (Makefile)

Clona el repositorio:

```bash
git clone https://github.com/ass190-ua/RogueBot
cd RogueBot
```

### 1️⃣ Compilar el proyecto

```bash
make
```

El binario se generará dentro del directorio de build configurado en el `Makefile` (por ejemplo `build_gnu/bin/roguebot`).

### 2️⃣ Ejecutar el juego

```bash
make run
```

### 3️⃣ Limpiar artefactos de compilación

```bash
make clean       # limpia objetos y binarios
make distclean   # limpieza profunda (incluye directorios de build)
```

### 4️⃣ Benchmarks de compilación (Hito 2 – ccache)

El proyecto incluye reglas para medir tiempos de compilación con y sin `ccache`:

```bash
make bench          # benchmarks utilizando ccache
make bench-nocache  # benchmarks sin ccache
```

Los resultados y el análisis se describen en `docs/Entregable1_Hito2.md`.

---

## 📦 Empaquetado de software (.deb)

Para el **Entregable 2** se ha configurado el empaquetado como paquete Debian.

### Construir el paquete `.deb`

```bash
make dist
```

Esto invoca internamente `dpkg-buildpackage` utilizando la carpeta `debian/` y deja el paquete resultante en el directorio `dist/` (por ejemplo `dist/roguebot_*.deb`).

### Instalar el paquete

```bash
sudo dpkg -i dist/roguebot_*.deb
```

Esto instalará:

- Binario: `/usr/bin/roguebot`
- Assets: `/usr/share/roguebot/assets/`
- Lanzador de escritorio: archivo `.desktop` e icono en las rutas estándar de `/usr/share`

Una vez instalado, podrás lanzar **RogueBot** desde el menú de aplicaciones del sistema.

---

## 🧱 Estructura del proyecto

```text
RogueBot/
│
├── assets/                     # Recursos del juego
│   ├── docs/                   # Documentación auxiliar de assets
│   └── sprites/                # Sprites organizados por tipo
│       ├── enemies/            # Sprites de enemigos
│       ├── items/              # Sprites de ítems
│       └── player/             # Sprites del jugador
│
├── docs/                       # Documentación general del proyecto
│   ├── GDD.md                  # Game Design Document
│   └── Entregable1_Hito2.md    # Informe del Hito 2 – Entregable 1 (Makefile + ccache)
│
├── src/                        # Código fuente del juego
│   ├── core/                   # Lógica núcleo del juego
│   │   ├── main.cpp            # Punto de entrada
│   │   ├── Game.cpp/.hpp       # Bucle principal y estado global del juego
│   │   ├── GameUtils.cpp/.hpp  # Utilidades varias
│   │   ├── Map.cpp/.hpp        # Gestión de mapas y salas
│   │   ├── Player.cpp/.hpp     # Lógica del jugador
│   │   ├── Enemy.cpp/.hpp      # Lógica de enemigos
│   │   ├── AssetPath.hpp       # Rutas centralizadas de assets
│   │   └── Attack.hpp          # Tipos y constantes de ataque
│   │
│   └── systems/                # Sistemas específicos desacoplados
│       ├── HUD.cpp/.hpp        # Heads-Up Display
│       ├── GameUI.cpp          # Menús, overlays y UI de juego
│       ├── ItemSystem.cpp      # Gestión y renderizado de ítems
│       ├── EnemySystem.cpp     # Gestión y actualización de enemigos
│       └── ItemSpawner.hpp     # Lógica de aparición de ítems
│
├── debian/                     # Ficheros de empaquetado Debian
│   ├── control                 # Metadatos del paquete
│   ├── rules                   # Reglas de build para debhelper
│   └── changelog               # Historial del paquete
│
├── packaging/                  # Archivos de integración con el escritorio
│   ├── roguebot.desktop        # Lanzador de escritorio
│   └── icons/roguebot.png      # Icono de la aplicación
│
├── dist/                       # (Ignorado) Salida de paquetes .deb generados
│
├── .github/workflows/          # GitHub Actions
│   ├── ci.yml                  # CI: compilación y verificación en cada push/PR
│   └── build-deb-on-release.yml# Build automático del .deb en cada release
│
├── Makefile                    # Sistema de build principal (Hito 2)
├── LICENSE                     # Licencia MIT
└── README.md                   # Este archivo
```

> 📝 A diferencia de la versión inicial del proyecto, ya **no se utiliza CMake** para el build principal. Toda la compilación, instalación y empaquetado se gestiona a través del `Makefile` y los ficheros de `debian/`.

---

## 🧩 Integración continua (CI)

El flujo de integración se gestiona mediante **GitHub Actions**:

- **CI de compilación** (`.github/workflows/ci.yml`)
  - Instala dependencias necesarias.
  - Compila el proyecto con `make` en Ubuntu.
  - Verifica que el código compila correctamente en la rama objetivo.

- **Build del paquete .deb en cada release** (`.github/workflows/build-deb-on-release.yml`)
  - Se dispara al crear una nueva *release* en GitHub.
  - Ejecuta `make dist` para construir el paquete `.deb`.
  - Adjunta el `.deb` como artefacto descargable en la release.

Esto automatiza el Entregable 2 del Hito 2.

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

> 🔖 Para tareas de mantenimiento, se recomienda usar el prefijo `chore/`, por ejemplo:  
> `chore/limpieza-build` o `chore/actualizar-ci`.

---

## 🎮 Notas técnicas

- Compilador mínimo: **g++ 9.0+** o **clang++ 10+**
- Estándar de C++: **C++17**
- Librería gráfica: **raylib 5.0**
- Sistema de build: **Makefile** (con soporte para `ccache` y `dpkg-buildpackage`)
- Sistema operativo compatible: **Linux** (Ubuntu recomendado)

> 🧰 El sistema de build detecta automáticamente los archivos fuente dentro de `src/` y genera las dependencias mediante reglas genéricas en el `Makefile`.

---

## 🧑‍💻 Equipo de desarrollo

| Usuario                                        | Nombre                   |
| ---------------------------------------------- | ------------------------ |
| [**@ass190-ua**](https://github.com/ass190-ua) | Arturo Soriano Sánchez   |
| [**@psm97-ua**](https://github.com/psm97-ua)   | Paula Soriano Muñoz      |
| [**@rla28-ua**](https://github.com/rla28-ua)   | Raúl López Arpa          |

---

## 📜 Licencia

Este proyecto se distribuye bajo la licencia **MIT**.  
Consulta el archivo `LICENSE` para más detalles.
