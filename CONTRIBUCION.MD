# Contribución a RogueBot

Gracias por contribuir a RogueBot. Esta guía resume el flujo de trabajo del repositorio y cómo añadir tests sin romper el CI.

## Flujo de trabajo (resumen)

1. Crear rama desde `develop`:
   ```bash
   git checkout develop
   git pull
   git checkout -b <tipo>/<nombre-corto>
