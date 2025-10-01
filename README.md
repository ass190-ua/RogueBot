# RogueBot

Alpha del proyecto (Hito 1).  
**Ramas**:  
- `main` → releases  
- `develop` → integración  
- `feature/*` → tareas  

---

## Build rápido (Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/roguebot
```

### Controles
- **ENTER**: reiniciar en placeholder (de momento)

### Estado
MVP en progreso para **P01 (Alpha)**.

---

# 5) Plantillas de Issues y PR (UI de GitHub)

## Issue templates

1. **Settings** → **General** → sección **Features**: activa **Issues** si no está.  
2. Ve a la pestaña **Issues** → botón **New issue** → **Get started** en *Bug report* y *Feature request* (si aparece).  
   - Si no aparecen: **Settings → Issue templates → New template**.

---

### `bug_report.md`

```markdown
**Descripción**  
¿Qué pasa? ¿Qué esperabas que pasara?

**Reproducir**  
1. ...
2. ...
3. ...

**Evidencia**  
Capturas / logs

**Entorno**  
- SO:
- Versión (commit/tag):
```
