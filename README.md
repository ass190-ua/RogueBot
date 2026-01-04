# 🤖 RogueBot

RogueBot es un proyecto desarrollado en C++ utilizando [raylib](https://www.raylib.com/) como motor gráfico.

En el **Hito 3** hemos migrado a un sistema de build **multiplataforma con CMake**, capaz de compilar y empaquetar el juego tanto en **Linux** como en **Windows** (ZIP e instalador).  
El **Makefile clásico** del Hito 2 se mantiene y sigue siendo **totalmente funcional en Linux** como alternativa.

---

## 🚀 Estado del Proyecto

- **Versión:** Alpha – Hito 3  
- **Sistemas de build:**
  - ✅ **Principal:** CMake (+ CPack) – multiplataforma
  - ✅ **Alternativo (Linux):** Makefile + `dpkg-buildpackage`
- **Ramas principales:**
  - `main`: versiones estables (releases)
  - `develop`: integración continua
  - `feature/*`: ramas de desarrollo por issue/tarea (por ejemplo `feature/gamepad`, `feature/refactoring`)

---

## 🛠️ Requisitos del entorno

### 🔹 Linux

Se recomienda Ubuntu 22.04 o superior (o distribución equivalente).

Dependencias mínimas para compilar con **CMake** (Hito 3):

```bash
sudo apt update
sudo apt install -y --no-install-recommends \
    build-essential cmake pkg-config git \
    libx11-dev libxrandr-dev libxi-dev libxxf86vm-dev \
    libxinerama-dev libxcursor-dev \
    libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev \
    libasound2-dev \
    ccache
```

> 🧩 CMake descargará y compilará raylib automáticamente mediante **FetchContent**, por lo que no es obligatorio tener `libraylib-dev` instalado (aunque puede usarse con `USE_EXTERNAL_RAYLIB=ON` si se desea).  
> `ccache` es opcional pero útil para acelerar compilaciones durante el desarrollo.

Si quieres usar el **Makefile antiguo**, entonces sí necesitas el paquete de raylib del sistema:

```bash
sudo apt install -y libraylib-dev
```

---

### 🔹 Windows (MSYS2 MinGW64)

Se utiliza **MSYS2** con el entorno **MinGW 64-bit**.

1. Instalar MSYS2 desde su web oficial.
2. Abrir **MSYS2 MinGW 64-bit** y ejecutar:

```bash
pacman -Syu      # puede pedir reiniciar la consola
pacman -S --needed \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-make \
    mingw-w64-x86_64-pkg-config \
    git
```

> 🔧 En Windows usamos CMake como sistema de build principal.  
> raylib se obtiene automáticamente (FetchContent) o bien desde MSYS2 si se activa `USE_EXTERNAL_RAYLIB`.

---

## ⚙️ Compilación y ejecución con CMake (Hito 3)

> ✅ **Recomendado** en ambos sistemas (Linux y Windows).  
> El Makefile se explica más abajo y se mantiene únicamente como alternativa en Linux.

Clona el repositorio:

```bash
git clone https://github.com/ass190-ua/RogueBot
cd RogueBot
```

---

### 🐧 Linux – Build, instalación y paquetes

#### 1️⃣ Configurar (modo instalación del sistema)

Este modo compilará raylib con **FetchContent** y preparará la instalación estándar en `/usr/local`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
      -DPREFER_RAYLIB_STATIC=ON \
      -DUSE_EXTERNAL_RAYLIB=OFF \
      -DASSET_ROOT=/usr/local/share/roguebot/assets
```

- `PREFER_RAYLIB_STATIC=ON` → enlaza raylib estática.
- `USE_EXTERNAL_RAYLIB=OFF` → descarga raylib desde GitHub si no está ya en `build/_deps`.
- `ASSET_ROOT=/usr/local/share/roguebot/assets` → ruta donde se instalarán los assets.

> ⏱️ La **primera** configuración puede tardar bastante: es cuando se descarga y compila raylib.  
> Mientras no borres la carpeta `build/`, no se volverá a descargar.

#### 2️⃣ Compilar

```bash
cmake --build build -j$(nproc)
```

#### 3️⃣ Instalar en el sistema

```bash
sudo cmake --install build
```

Esto instala:

- Binario: `/usr/local/bin/roguebot`
- Assets: `/usr/local/share/roguebot/assets/`
- Lanzador de escritorio `.desktop` e icono en las rutas estándar de `/usr/local/share`.

Puedes ejecutar el juego con:

```bash
roguebot
```

o desde el menú de aplicaciones.

#### 4️⃣ Generar paquetes `.deb` y `.tar.gz`

```bash
cmake --build build --target package
```

Se generarán en `build/` archivos similares a:

- `roguebot-1.0-Linux.deb`
- `roguebot-1.0-Linux.tar.gz`

---

### 🪟 Windows (MSYS2 MinGW64) – Build, ZIP e instalador

En la consola **MSYS2 MinGW 64-bit**:

```bash
cd /c/Users/asscr/Desktop/RogueBot   # ajusta la ruta a tu caso
```

#### 1️⃣ Configurar

```bash
cmake -B build -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DPREFER_RAYLIB_STATIC=ON \
      -DUSE_EXTERNAL_RAYLIB=OFF \
      -DASSET_ROOT=assets
```

- `ASSET_ROOT=assets` porque el ejecutable va junto a la carpeta `assets/` en el ZIP/instalador.
- Con `USE_EXTERNAL_RAYLIB=OFF` se usa el mismo mecanismo de **FetchContent** que en Linux.

#### 2️⃣ Compilar

```bash
cmake --build build -j$(nproc)
```

#### 3️⃣ Ejecutar sin instalar

```bash
./build/roguebot.exe
```

#### 4️⃣ Generar ZIP + instalador NSIS

```bash
cmake --build build --target package
```

En `build/` tendrás:

- Un **ZIP “portable”** con:
  - `roguebot.exe`
  - `assets/…`
- Un **instalador NSIS `.exe`** que:
  - Instala en `C:\Program Files\RogueBot`
  - Crea acceso en el menú Inicio
  - Crea/elimina acceso directo en el Escritorio

---

## ⚙️ Sistema de build clásico (Makefile – solo Linux)

Aunque el sistema de build principal es ahora **CMake + CPack**, el proyecto conserva el **Makefile** del Hito 2, plenamente funcional en Linux.

> 💡 Úsalo si quieres reproducir los experimentos de compilación con `ccache` o el empaquetado `.deb` original.

### 1️⃣ Compilar el proyecto

```bash
make
```

El binario se genera en `build_gnu/bin/roguebot` (según la configuración del Makefile).

### 2️⃣ Ejecutar el juego

```bash
make run
```

### 3️⃣ Limpiar artefactos de compilación

```bash
make clean       # limpia objetos y binarios
make distclean   # limpieza profunda (incluye directorios de build)
```

### 4️⃣ Benchmarks de compilación (ccache)

```bash
make bench          # benchmarks utilizando ccache
make bench-nocache  # benchmarks sin ccache
```

Los resultados y el análisis se describen en `docs/Entregable1_Hito2.md`.

### 5️⃣ Empaquetado `.deb` con Makefile

Para el empaquetado clásico del **Entregable 2 (Hito 2)**:

```bash
make dist
```

Esto invoca internamente `dpkg-buildpackage` usando la carpeta `debian/` y deja el `.deb` en `dist/`.

---

## 🧩 Integración continua (CI)

La integración continua se gestiona mediante **GitHub Actions**:

- **CI de compilación en Linux** (`.github/workflows/ci-linux.yml`)  
  Workflow que:
  - Instala las dependencias del toolchain y de raylib.
  - Configura y compila el proyecto con **CMake** (usando el `CMakeLists.txt` del Hito 3).
  - Verifica en cada *push* y *pull request* que el build funciona correctamente en Linux.

- **Build de paquetes en cada release** (`.github/workflows/release.yml`)  
  Workflow que:
  - Se dispara al crear una nueva *release* en GitHub (o manualmente con *workflow_dispatch*).
  - Ejecuta los targets de **CPack** (`package`) para:
    - Generar `.deb` y `.tar.gz` en Linux.
    - Generar `.zip` y el instalador `.exe` (NSIS) en Windows.
  - Adjunta automáticamente todos los artefactos generados a la release correspondiente.

El `Makefile` utilizado en el Hito 2 se mantiene en el repositorio como alternativa para Linux,
pero el sistema de build principal del proyecto pasa a ser **CMake + CPack**, cumpliendo los
requisitos del **Entregable 3: proyecto autoconfigurable y aplicación multiplataforma**.

---

## 🧪 Tests (CTest)

Para reproducir localmente lo que ejecuta el CI (CTest con labels):

```bash
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests -j
ctest --test-dir build-tests -L unit --output-on-failure
ctest --test-dir build-tests -L integration --output-on-failure

## 💡 Desarrollo y contribución

1. Crear rama a partir de `develop`:

   ```bash
   git checkout develop
   git pull
   git checkout -b feature/<nombre-de-la-tarea>
   ```

2. Implementar cambios y hacer commit:

   ```bash
   git add .
   git commit -m "feat: descripción breve del cambio"
   ```

3. Subir rama y abrir Pull Request:

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
- Sistemas de build:
  - **Principal:** CMake + CPack (multiplataforma)
  - **Alternativo:** Makefile + `dpkg-buildpackage` (solo Linux, Hito 2)
- Sistemas operativos probados:
  - **Linux:** Ubuntu 22.04+
  - **Windows:** 10/11 con MSYS2 MinGW64

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
