---
name: Feature request
about: Proponer una nueva funcionalidad o mejora para RogueBot
title: "[FEATURE] "
labels: type/feature
assignees: ''
---

## 💡 Resumen de la funcionalidad

**Descripción corta:**  
<!-- Una frase clara de lo que quieres que se añada o cambie -->

**Tipo de cambio:**  
- [ ] Nueva mecánica de juego  
- [ ] Mejora de UI/HUD  
- [ ] Enemigos / ítems nuevos  
- [ ] Balanceo / ajustes de dificultad  
- [ ] Mejora técnica (rendimiento, refactor, etc.)  
- [ ] Otro (`...`)

---

## 🎯 Valor para el jugador / proyecto

Explica por qué esta idea merece la pena:

- ¿Qué problema resuelve?  
- ¿Qué aporta a la experiencia del jugador (diversión, claridad, rejugabilidad…)?  
- ¿Está alineada con el GDD / visión del juego?

Ejemplo de respuesta:
> “Añadir X ayudaría a que el jugador entienda mejor Y, y haría las partidas más variadas porque Z…”

---

## ✅ Criterios de aceptación

Marca lo que debería cumplirse para considerar el feature como **completado**:

- [ ] La nueva funcionalidad se implementa de forma clara y consistente con el estilo actual.
- [ ] Funciona correctamente en Linux (entorno de la práctica).
- [ ] No rompe el MVP actual ni introduce regresiones graves.
- [ ] Está cubierta por pruebas manuales básicas (y/o automáticas si aplica).
- [ ] La documentación relevante se ha actualizado (README, GDD, etc., si procede).

Puedes añadir criterios específicos, por ejemplo:

- [ ] El nuevo enemigo aparece a partir de la sala X.
- [ ] El nuevo ítem tiene probabilidad de aparición aproximada del `Y%`.
- [ ] El HUD muestra claramente el nuevo estado/indicador `Z`.

---

## 🧩 Detalles de diseño (opcional pero recomendado)

Si tienes una idea más concreta de cómo debería funcionar, descríbela aquí:

- Flujo de uso en el juego (qué hace el jugador, qué ve, qué pasa después).
- Estados o reglas importantes (enfriamientos, condiciones de activación, restricciones…).
- Cambios visuales esperados (sprites nuevos, iconos en HUD, efectos…).

Puedes usar viñetas, pseudo-diagramas o pseudo-código si ayuda.

---

## 🛠️ Notas técnicas / implementación (opcional)

Si tienes sugerencias técnicas, añádelas aquí:

- Archivos o sistemas que habría que tocar  
  (ej. `src/core/Game.hpp`, `src/systems/HUD.cpp`, `EnemySystem.cpp`, etc.).
- Nuevas estructuras de datos, enums, constantes, etc.
- Consideraciones de rendimiento o arquitectura.

Ejemplo:
> “Podría implementarse como un nuevo tipo de ítem gestionado desde `ItemSystem`, con una entrada adicional en `AssetPath.hpp` y un icono nuevo en `assets/sprites/items/`…”

---

## 🔗 Relacionado con… (opcional)

- Issues relacionados: `#123`, `#456`  
- Bugs que ayudaría a mitigar: `#789`  
- Otras ideas similares: `...`

---

## 📝 Notas adicionales

Cualquier otra cosa que ayude a entender mejor la propuesta:

- Inspiración (otros juegos, referencias visuales…).  
- Mockups, bocetos o diagramas (si tienes, puedes adjuntar imágenes).  
- Riesgos o dudas que tengas sobre este feature.
