# 🤖 RogueBot — Hito 2: Distribución del Juego  
## ✳️ Entregable 1: Makefiles y ccache
  
**Proyecto:** RogueBot  
**Fecha:** _(03-12-2025)_  
**Entorno:** Linux (máquina virtual, 3 hilos de CPU)  
**Compilador:** g++ (17) con raylib 5.0  

---

## 🎯 Objetivo
Implementar un sistema de compilación **automático, genérico y eficiente** para el proyecto *RogueBot*, cumpliendo los requisitos del Entregable 1 del Hito 2:

- Sistema de build basado en **Makefile GNU**.  
- Soporte **multidirectorio** (`src/core`, `src/systems`, etc.).  
- Compilación **paralela (-jN)** con generación automática de dependencias.  
- Integración opcional con **ccache**.  
- Realización de pruebas de rendimiento y comparación de tiempos.  

---

## 🏗️ Estructura del proyecto

```
RogueBot/
├── assets/
├── docs/           
├── src/
│   ├── core/        → lógica principal (Game, Player, Map, Enemy…)
│   ├── systems/     → subsistemas (HUD, GameUI, EnemySystem…)
│   └── main.cpp
├── build_gnu/       → salida del Makefile
├── Makefile         → sistema de compilación GNU
```

El Makefile detecta automáticamente todos los `.cpp` en cualquier subcarpeta dentro de `src/`, por lo que no requiere modificaciones al añadir nuevos archivos o módulos.

---

## ⚙️ Funcionamiento del Makefile

### Reglas principales
| Regla | Descripción |
|--------|--------------|
| `make` / `make all` | Compila el proyecto completo. |
| `make -jN` | Compila en paralelo usando N hilos. |
| `make -j$(nproc)` | Compila usando todos los hilos disponibles del sistema. |
| `make run` | Ejecuta el binario generado. |
| `make clean` | Elimina los objetos intermedios. |
| `make distclean` | Limpia todo el directorio de build. |
| `make print-vars` | Muestra información de compilación. |
| `make bench` | Benchmark de compilación con ccache. |
| `make bench-nocache` | Benchmark sin ccache. |
| `make ccache-zero` / `clear` / `stats` | Gestión y análisis de la caché de compilación. |

---

## 🧵 Compilación paralela (-jN)

Para esta máquina virtual (3 hilos), se han medido los tiempos con distintos valores de `N`:

| N | Tiempo real (s) | %CPU | Memoria (KB) | Observaciones |
|---|------------------|------|---------------|---------------|
| 1 | 0:08.21 | 100% | 164900 | Secuencial (referencia). |
| 2 | 0:06.25 | 186% | 165016 | Mejora notable. |
| 3 | 0:05.19 | 246% | 164648 | Saturación de CPU (≈3 hilos). |
| 4 | 0:04.92 | 246% | 164976 | Ligera mejora por solape. |
| 8 | 0:04.24 | 254% | 165096 | Mejora adicional moderada. |
| 12 | 0:03.85 | 257% | 164976 | **Mejor tiempo medido** (margen pequeño). |
| 16 | 0:04.48 | 236% | 164956 | Peor por overhead de planificación. |

🧠 **Conclusión:**  
La **saturación** llega en **N=3** (coincide con `nproc=3`).  
Subir más hilos da **mejoras marginales** (por solapar TUs/latencias) hasta **N≈8–12**; **a partir de 12 empeora**. Para uso diario, **N=3–4** es óptimo/estable; para “quick builds”, **N=8** puede exprimir un poco más sin degradar.

---

## ⚡ Uso de *ccache*

### 1️⃣ Comparativa de compilación con y sin *ccache*

| Escenario | Tiempo total | Observaciones |
|------------|--------------|----------------|
| **Sin `ccache`** (`-j3`) | **0:05.19** | Compilación completa, sin reutilización. |
| **Con `ccache` (1.ª)** (`-j3`) | **0:04.43** | Primer uso: crea la caché (*misses* elevados). |
| **Con `ccache` (2.ª)** (`-j3`) | **0:00.23** | Gran reducción de tiempo gracias a *cache hits* directos. |

> Comandos usados:
> ```bash
> ccache -C && ccache -z
> make distclean && /usr/bin/time -f "CCACHE 1ª | Tiempo:%E | CPU:%P | Mem:%M KB" make -j$(nproc) all >/dev/null && ccache -s
> make clean    && /usr/bin/time -f "CCACHE 2ª | Tiempo:%E | CPU:%P | Mem:%M KB" make -j$(nproc) all >/dev/null && ccache -s
> ```

### 2️⃣ Estadísticas de *ccache*

| Métrica | Después de 1.ª compilación | Después de 2.ª compilación |
|----------|-----------------------------|-----------------------------|
| Cacheable calls | 10 / 11 (90.91%) | 20 / 22 (90.91%) |
| **Hits** | **0 / 10 (0.00%)** | **10 / 20 (50.00%)** |
|  • Direct hits | 0 | 10 (100%) |
|  • Preprocessed hits | 0 | 0 |
| **Misses** | **10 / 10 (100.0%)** | **10 / 20 (50.00%)** |
| Uncacheable calls | 1 / 11 (9.09%) | 2 / 22 (9.09%) |
| Cache size (GiB) | 0.0 / 5.0 | 0.0 / 5.0 |

🧠 **Conclusión de `ccache`:**  
La primera compilación **puebla** la caché (solo *misses*). La segunda alcanza **50% de *cache hits*** (todos **direct hits**), reduciendo el tiempo de `~4.43s` a `~0.23s` con `-j3`.

---

## 🔍 Análisis final
- El **Makefile** es **genérico y multidirectorio** (no requiere cambios al añadir archivos o carpetas).  
- La **compilación paralela** satura en `N=3` (hardware disponible); subir N ofrece mejoras marginales por solapamiento, pero con retornos decrecientes.  
- **`ccache`** aporta una **mejora muy significativa** tras la primera compilación.  
- Se cumplen los objetivos del **Entregable 1** del **Hito 2**.

---

## 🧩 Comandos utilizados
```bash
# Base
make distclean && make -j1

# Bench sin ccache
CCACHE_DISABLE=1 make distclean
CCACHE_DISABLE=1 /usr/bin/time -f "N=1 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j1  all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=2 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j2  all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=3 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j3  all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=4 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j4  all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=8 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j8  all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=12 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j12 all >/dev/null && make clean
CCACHE_DISABLE=1 /usr/bin/time -f "N=16 | Tiempo:%E | CPU:%P | Mem:%M KB" make -j16 all >/dev/null && make clean

# ccache
ccache -C && ccache -z
make distclean && /usr/bin/time -f "CCACHE 1ª | Tiempo:%E | CPU:%P | Mem:%M KB" make -j$(nproc) all >/dev/null && ccache -s
make clean    && /usr/bin/time -f "CCACHE 2ª | Tiempo:%E | CPU:%P | Mem:%M KB" make -j$(nproc) all >/dev/null && ccache -s
```

---

## 🏁 Conclusión
> El sistema de compilación de **RogueBot** cumple los criterios de automatización, rendimiento y mantenibilidad exigidos en el **Hito 2 – Entregable 1**, incorporando `make` multidirectorio, compilación paralela y `ccache`.
