# RogueBot — Game Design Document (GDD)

**Estado:** Alpha (Hito 1) · **Fecha:** 22/10/2025 · **Versión del doc:** 0.1.0

## 1. Visión / High Concept

**RogueBot** es un mini-roguelite de exploración por salas con estética retro. El jugador avanza por niveles generados proceduralmente, combate enemigos en turnos “cortos” (pasos discretos con animación) y busca la **salida**. Cada nivel incluye botines que alteran el alcance del ataque y utilidades (llave, escudo, etc.).

**Pilares**

* **Claridad**: tiles nítidos, HUD minimalista.
* **Lectura táctica**: FOV/niebla y ataque direccional con alcance.
* **Runs breves**: generación rápida y reintentos inmediatos.

**Bucle jugable (loop)**
Explorar → Avistar (FOV) → Posicionarte → **Atacar** → **Loot** → Salida → Siguiente nivel.

---

## 2. Mecánicas principales

### 2.1 Movimiento y cámara

* Movimiento en cuadrícula (WASD / cursores).
* **Dos modos**: `Paso a paso` (1 paso por pulsación) y `Repetición con cooldown` (avance continuado con retardo). Toggle con `T`.
* Cámara con zoom (`Q`/`E` o rueda). Reset de cámara con `C`.

### 2.2 Visión / Niebla de guerra (FOV)

* Niebla con radio configurable por tiles.
* Toggle niebla con `F2`. Ajuste fino del FOV con `[` y `]`.

### 2.3 Combate

* **Melee direccional** (frontal/orientado), desencadenado con `Espacio` o click izquierdo.
* Daño base de puño y **alcance** ampliable por arma (espada, pistola de plasma en roadmap).
* Enemigos hacen **daño por contacto** con cooldown por enemigo.
* **Invulnerabilidad** breve tras recibir daño.

### 2.4 Progresión del nivel

* Generación procedural de salas y pasillos con **tile EXIT** único por nivel.
* Colocación de jugador en sala inicial; objetivo: llegar a la **salida**.

---

## 3. Contenido del juego (Alpha)

### 3.1 Objetos/ítems

* **Llave maestra** (keycard): abre progreso/puertas del nivel (si aplica más adelante).
* **Escudo**: utilidad defensiva (hook para mitigar golpes en futuros hitos).
* **Pila de vida/energía**: curación/consumo.
* **Gafas 3D**: utilidad visual (espacio para efectos de visibilidad en futuros hitos).
* **Espada láser** (niveles: azul/verde/roja): aumenta **alcance** de melee (rango 3 en diseño).
* **Pistola de plasma** (niveles 1–2): diseño previsto para ataque a distancia (roadmap Hito 2).

> Nota: En Hito 1, el combate efectivo es **melee**; la pistola y mejoras adicionales se dejan preparadas como **hook** visual/UX.

### 3.2 Enemigos (Alpha)

* Enemigo básico que patrulla/avanza por celdas; **daño por contacto**.
* Atributos por defecto: vida 100 (escala %), cooldown de ataque 0.40 s.

### 3.3 UI / Pantallas

* **Menú principal** (Jugar, Ayuda, Salir) con **Guía de objetos**.
* **HUD** en partida (vidas con animación, avisos).
* **Game Over** (reinicio rápido) y **Victoria**.

---

## 4. Controles

* **Movimiento**: `WASD` / cursores.
* **Atacar**: `Espacio` o **click izquierdo**.
* **Alternar modo movimiento**: `T`.
* **Niebla ON/OFF**: `F2`.
* **Ajustar FOV**: `[` y `]`.
* **Zoom cámara**: `Q` (acercar) / `E` (alejar) o rueda.
* **Reset cámara**: `C`.
* **Reiniciar run**: `R`.
* **Volver al menú**: `Esc` (y cerrar overlay de Ayuda con `Esc` o click en «Volver»).
* *(Dev)* `H`/`J` para test de vida ±1.

---

## 5. Reglas y balance (parámetros base)

* Vida del jugador: `5`.
* Daño contacto enemigo: `1` por golpe.
* Invulnerabilidad tras golpe: `~0.6 s`.

---

## 6. Generación de niveles

* Algoritmia de **rooms+corridors** con un `EXIT` único.
* Posicionamiento del jugador: centro de la primera sala disponible.
* Re-cómputo de visibilidad tras cada paso.

---

## 7. Arte

* **Sprites** en `assets/sprites/` (player, enemies, items). Player con 4 direcciones × 3 frames (idle, walk1, walk2).
* Estética **pixel-art** limpia; fondo/tiles minimalistas.

---

## 8. Tecnología

* **Lenguaje**: C++17.
* **Motor**: raylib 5.0.
* **Build**: CMake ≥ 3.16 (Linux).
* **Ejecución**: `./build/roguebot [seed]` (seed opcional para runs deterministas).
* **Estructura**: `src/` modular (Game, Map, Player, Enemy, HUD, sistemas de Items/Enemigos, utilidades).

---

## 9. Criterios de aceptación — Hito 1 (Alpha)

1. **Jugable**: menú → partida → salida o game over → reinicio.
2. **Dos pantallas** mínimas: juego y game over (además de menú).
3. **Mecánica principal** funcional: moverse, ver FOV, atacar y recibir daño.
4. **Repositorio** con gestión de ramas, issues y **release** con SemVer + binario Linux.

---

## 10. Estrategia de gestión de ramas

**GitHub Flow extendido (con `develop`)**

> Trabajamos como GitHub Flow (ramas cortas por funcionalidad + PR), pero añadimos una rama **`develop`** para integración continua. **`main`** queda **solo** para releases estables.

**Ramas**

* `main`: estable/producción. Solo entra vía PR de `develop`. Se **tagea** cada release (`vX.Y.Z[-pre]`). Rama protegida.
* `develop`: integración de features. Base por defecto para PRs de funcionalidad. Rama protegida (requiere CI verde).
* `feature/<id>-<slug>`: una por issue/tarea. Nace desde `develop`, se mergea a `develop` vía PR.
* `hotfix/<slug>`: arreglos urgentes desde `main`. Se mergea a `main` y se **sincroniza** de vuelta a `develop` (merge o cherry-pick) para no perder el fix.

**Pull Requests**

* De `feature/*` → `develop` (revisión obligatoria + CI verde).
* De `develop` → `main` para **release** (CI verde + check de versión y changelog).
* Plantilla de PR con: *Resumen*, *Issue link*, *Checklist QA*, *Screenshots* cuando aplique.

**Convenciones**

* Commits semánticos: `feat:`, `fix:`, `refactor:`, `docs:`, `chore:`, `test:`…
* Nombrado de ramas: `feature/#123-fov-ajustable`, `hotfix/crash-menu-escape`.
* Issues siempre vinculados en el PR (`Closes #123`).

**Flujo de release**

1. Merge `develop` → `main` por PR cuando el hito esté completo.
2. Crear tag SemVer (p. ej., `v0.1.0-alpha.1`).
3. Publicar **GitHub Release** con binario Linux + docs.

---

## 11. Bugtracking

* **GitHub Issues** con **plantillas** para *bug*, *feature*, *task*.
* **Etiquetas**: `bug`, `enhancement`, `ui`, `balance`, `assets`, `tech-debt`, `good-first-issue`.
* **Milestones** por hito (Alpha, Beta, Release) y por *sprint* de clase.
* **Kanban**: Projects (To Do / In Progress / Review / Done).

---

## 12. Versionado y releases

* **SemVer**: `MAJOR.MINOR.PATCH[-pre]`.
* Hito 1: `v0.1.0-alpha.1` (tag + release).
* Artefactos: código fuente, **zip del ejecutable Linux**, y documentación (`README.md`, `docs/GDD.md`).

---

## 13. Roadmap (próximos hitos)

**Hito 2 (Beta)**

* Armas: pistola de plasma funcional (rango 7, oclusión por paredes), feedback de impacto.
* Más IA enemiga (seguimiento, patrones básicos).
* SFX y música base.
* Opciones en menú (volumen, dificultad, semilla fija).

**Hito 3 (Release)**

* Tipos de sala (eventos simples), variedad de enemigos y ítems sinérgicos.
* Pulido de UX, logros/estadísticas y balance final.

---

## 14. QA / Pruebas

* **Pruebas manuales** por checklist en cada PR (controles, FOV, daño, salida, reinicio).
* **Seeds fijas** en CI para reproducibilidad de generación procedural.
* Revisión cruzada entre compañeros y **issues** de regresión.

---

## 15. Créditos y licencia

* Equipo: ver `README.md` (Créditos).
* Licencia: **MIT**.
