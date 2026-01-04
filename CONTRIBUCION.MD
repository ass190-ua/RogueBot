# Contribución a RogueBot

Gracias por contribuir a RogueBot. Esta guía resume el flujo de trabajo del repositorio y cómo añadir tests sin romper el CI.

---

## 🧩 Flujo de trabajo (resumen)

1. Crear rama desde `develop`:

```bash
git checkout develop
git pull
git checkout -b <tipo>/<nombre-corto>
```

2. Commits pequeños y descriptivos:

```bash
git add .
git commit -m "<tipo>: descripción breve"
```

3. Push y Pull Request a `develop`:

```bash
git push -u origin <tipo>/<nombre-corto>
```

> Recomendación: usar prefijos como `feature/`, `fix/`, `chore/`, `docs/`.

---

## 🧪 Tests (CTest): cómo ejecutar, añadir y etiquetar

Los tests se compilan con CMake y se registran en CTest desde `tests/CMakeLists.txt`.

### Ejecutar tests (reproducir CI)

```bash
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests -j
ctest --test-dir build-tests -L unit --output-on-failure
ctest --test-dir build-tests -L integration --output-on-failure
```

**Criterio de clasificación:**
- `unit`: tests de lógica pura (sin assets, filesystem, locales, gettext ni raylib).
- `integration`: tests que dependen de assets, filesystem, locales, gettext o raylib.

---

## ➕ Añadir un test nuevo

1. Crear el fichero del test en `tests/` usando Boost.Test.
2. En `tests/CMakeLists.txt`, añadir:
   - `add_executable(...)`
   - `rb_link_boost_test(...)`
   - `add_test(NAME ... COMMAND ...)`
3. Asignar `WORKING_DIRECTORY` si aplica y etiquetar el test.

---

## 🏷️ Etiquetado con `rb_label_test`

Todos los tests deben etiquetarse usando el helper `rb_label_test`.

Se usan **dos labels**:
- **scope**: `unit` o `integration`
- **category**: `utils`, `map`, `assets`, `i18n`

### Ejemplo (test unitario de utils)

```cmake
add_test(NAME mi_test COMMAND rb_test_mi_test)

set_tests_properties(mi_test PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

rb_label_test(mi_test unit utils)
```

### Ejemplo (test de integración con assets)

```cmake
add_test(NAME mi_test_assets COMMAND rb_test_mi_test_assets)

set_tests_properties(mi_test_assets PROPERTIES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

rb_label_test(mi_test_assets integration assets)
```

---

## Nota sobre tests de i18n

Algunos tests de `i18n` requieren locales y gettext.  
Si algún test falla por configuración regional, prueba:

```bash
export LANG=es_ES.UTF-8
export LC_ALL=es_ES.UTF-8
export LANGUAGE=es
```

En Linux, asegúrate de tener los locales instalados:

```bash
sudo apt-get update
sudo apt-get install -y locales gettext
sudo locale-gen es_ES.UTF-8
sudo locale-gen en_GB.UTF-8
```
