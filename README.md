# Need for Speed - Taller de Programación I

Juego de carreras multijugador inspirado en Need for Speed, desarrollado en C++ utilizando SDL2 para el cliente gráfico y Qt6 para la interfaz de lobby.

## Índice

- [Dependencias](#dependencias)
- [Instalación](#instalación)
- [Ejecución](#ejecución)
- [Instrucciones de Juego](#instrucciones-de-juego)

---

## Dependencias

El proyecto requiere las siguientes dependencias:

- **Qt6** (qt6-base-dev, qt6-base-dev-tools, qt6-tools-dev, qt6-tools-dev-tools)
- **SDL2** con sus extensiones:
  - SDL2_image
  - SDL2_mixer  
  - SDL2_ttf

### Instalación


---

## Ejecución


---

### Descripción del Juego

**Micro Machines** es un juego de carreras multijugador para hasta **8 jugadores simultáneos**. Los jugadores compiten en carreras de **3 rondas** en diferentes mapas de ciudades.

### Creación y Unión a Partidas

#### Crear una Partida

1. Ejecutar el cliente (`taller_client_ui`)
2. Conectarse al servidor ingresando la IP y puerto
3. Seleccionar **"Create Game"**
4. Ingresar el nombre de la partida
5. Seleccionar el mapa deseado (Liberty City, San Andreas o Vice City)
6. Presionar **"Create Game"** para crear la partida
7. Compartir el código de partida con los demás jugadores
8. Seleccionar el auto deseado
9. Presionar **"Start Game"** cuando todos los jugadores estén listos

#### Unirse a una Partida

1. Ejecutar el cliente (`taller_client_ui`)
2. Conectarse al servidor ingresando la IP y puerto
3. Seleccionar **"Join Game"** o **"Create Game"**
4. Ingresar o crear la partida
5. Seleccionar el auto deseado
6. Esperar a que el creador inicie la partida

### Sistema de Juego

- **Carreras de 3 rondas:** Cada partida consiste en 3 rondas completas al circuito
- **Entre rondas:** Hay un período de **10 segundos** entre cada ronda donde los jugadores pueden mejorar su auto
- **Sistema de mejoras:** Las mejoras del auto tienen un costo en forma de **penalización de tiempo**
- **Ganador:** Gana el jugador con el **menor tiempo total** acumulado (tiempo de carrera + penalizaciones)

---

## Instrucciones de Juego

### Controles de Movimiento

| Tecla | Acción |
|-------|--------|
| `↑` | Acelerar |
| `↓` | Frenar / Reversa |
| `←` | Girar a la izquierda |
| `→` | Girar a la derecha |

### Mejoras del Auto (durante los 10 segundos entre rondas)

| Tecla | Mejora |
|-------|--------|
| `1` | Mejora de velocidad máxima |
| `2` | Mejora de aceleración |
| `3` | Mejora de manejo |
| `4` | Mejora de turbo |

> **Nota:** Cada mejora aplicada añade una penalización de tiempo al total del jugador.

### Controles de Audio

| Tecla | Acción |
|-------|--------|
| `W` | Subir volumen |
| `S` | Bajar volumen |

---