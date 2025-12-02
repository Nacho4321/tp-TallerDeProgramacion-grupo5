# Documentación Técnica - GTA Racing

## Índice
1. [Arquitectura General](#arquitectura-general)
2. [Sistema de Threads](#sistema-de-threads)
3. [Protocolo de Comunicación](#protocolo-de-comunicación)
4. [Estructura de Mensajes](#estructura-de-mensajes)

---

## Arquitectura General

El proyecto sigue una arquitectura cliente-servidor con las siguientes características:

- **Servidor**: Maneja múltiples clientes concurrentemente, gestiona partidas (lobbies) y ejecuta la lógica del juego (física Box2D, colisiones, checkpoints).
- **Cliente**: Se conecta al servidor, envía inputs del jugador y renderiza el estado del juego recibido.
- **Protocolo**: Comunicación binaria con opcodes de 1 byte seguidos de payloads variables.

---

## Sistema de Threads

### Clase Base `Thread`

Todos los threads del sistema heredan de la clase `Thread` de la catedra, que tiene:

```cpp
class Thread : public Runnable {
protected:
    bool should_keep_running() const; 
public:
    void start();   
    void stop();    
    void join();    
    bool is_alive() const;
    virtual void run() = 0;  
};
```

### Threads del Servidor

```
┌────────────────────────────────────────────────────────────────┐
│                         SERVIDOR                               │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────┐                                              │
│  │   Acceptor   │ ─── Thread principal que acepta conexiones   │
│  │   (Thread)   │                                              │
│  └──────┬───────┘                                              │
│         │ crea                                                 │
│         ▼                                                      │
│  ┌─────────────────────────────────────────────────────┐       │
│  │              ClientHandler (por cliente)            │       │
│  │  ┌─────────────────┐    ┌─────────────────┐         │       │
│  │  │ ClientReceiver  │    │  ClientSender   │         │       │
│  │  │    (Thread)     │    │    (Thread)     │         │       │
│  │  │                 │    │                 │         │       │
│  │  │ Lee mensajes    │    │ Envía mensajes  │         │       │
│  │  │ del socket      │    │ desde outbox    │         │       │
│  │  └────────┬────────┘    └────────▲────────┘         │       │
│  │           │                      │                  │       │
│  │           ▼                      │                  │       │
│  │  ┌─────────────────┐    ┌────────┴────────┐         │       │
│  │  │  LobbyHandler   │    │     Outbox      │         │       │
│  │  │  (dispatcher)   │    │ Queue<Server    │         │       │
│  │  │                 │    │    Message>     │         │       │
│  │  └────────┬────────┘    └─────────────────┘         │       │
│  └───────────┼─────────────────────────────────────────┘       │
│              │                                                 │
│              ▼                                                 │
│  ┌──────────────────────────────────────────────────────┐      │
│  │                    GameMonitor                       │      │
│  │         (gestiona partidas con mutex)                │      │
│  └──────────────────────┬───────────────────────────────┘      │
│                         │                                      │
│                         ▼                                      │
│  ┌─────────────────────────────────────────────────────┐       │
│  │                  GameLoop (Thread)                  │       │
│  │                                                     │       │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │       │
│  │  │  EventLoop  │  │TickProcessor│  │ WorldManager│  │       │
│  │  │ (procesa    │  │             │  │  (Box2D)    │  │       │
│  │  │  eventos)   │  │             │  │             │  │       │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │       │
│  │                                                     │       │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │       │
│  │  │PlayerManager│  │ NPCManager  │  │BroadcastMgr │  │       │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │       │
│  └─────────────────────────────────────────────────────┘       │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

| Thread | Clase | Responsabilidad |
|--------|-------|-----------------|
| **Acceptor** | `Acceptor` | Escucha conexiones entrantes en el puerto, crea `ClientHandler` por cada cliente |
| **ClientReceiver** | `ClientReceiver` | Lee mensajes del socket del cliente, los envía al `LobbyHandler` para dispatch |
| **ClientSender** | `ClientSender` | Consume mensajes de la cola `outbox` y los envía al cliente |
| **GameLoop** | `GameLoop` | Ejecuta la simulación física a 60 FPS, procesa eventos, broadcast de posiciones |

### Threads del Cliente

```
┌────────────────────────────────────────────────────────────────┐
│                          CLIENTE                               │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌─────────────────────────────────────────────────────┐       │
│  │                GameClientHandler                    │       │
│  │                                                     │       │
│  │  ┌───────────────────┐    ┌───────────────────┐     │       │
│  │  │ GameClientReceiver│    │  GameClientSender │     │       │
│  │  │     (Thread)      │    │     (Thread)      │     │       │
│  │  │                   │    │                   │     │       │
│  │  │ Recibe mensajes   │    │ Envía comandos    │     │       │
│  │  │ del servidor      │    │ al servidor       │     │       │
│  │  └─────────┬─────────┘    └─────────▲─────────┘     │       │
│  │            │                        │               │       │
│  │            ▼                        │               │       │
│  │  ┌─────────────────┐    ┌──────────┴──────────┐     │       │
│  │  │    incoming     │    │      outgoing       │     │       │
│  │  │ Queue<Server    │    │  Queue<string>      │     │       │
│  │  │    Message>     │    │                     │     │       │
│  │  └─────────────────┘    └─────────────────────┘     │       │
│  └─────────────────────────────────────────────────────┘       │
│                                                                │
│  ┌─────────────────────────────────────────────────────┐       │
│  │                    Main Thread                      │       │
│  │                                                     │       │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │       │
│  │  │InputHandler │  │GameRenderer │  │   Client    │  │       │
│  │  │ (SDL events)│  │ (SDL render)│  │  (lógica)   │  │       │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  │       │
│  └─────────────────────────────────────────────────────┘       │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

| Thread | Clase | Responsabilidad |
|--------|-------|-----------------|
| **GameClientReceiver** | `GameClientReceiver` | Recibe paquetes del servidor, los encola en `incoming` |
| **GameClientSender** | `GameClientSender` | Consume comandos de `outgoing`, los envía al servidor |
| **Main Thread** | - | Loop SDL: procesa input, renderiza, consume `incoming` |

### Sincronización

- **`Queue<T>`**: Cola thread-safe de la catedra.
- **`players_map_mutex`**: Protege el mapa de jugadores en `GameLoop` para acceso concurrente.

---

## Protocolo de Comunicación

### Formato General

Cada mensaje tiene la estructura:

```
┌──────────┬─────────────────────────────────┐
│  Opcode  │            Payload              │
│ (1 byte) │         (variable)              │
└──────────┴─────────────────────────────────┘
```

### Tabla de Opcodes

#### Movimiento (Cliente → Servidor)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x01` | `MOVE_UP_PRESSED` | Acelerar (presionado) | Jugador presiona ↑ | `player_id (4B)`, `game_id (4B)` |
| `0x02` | `MOVE_UP_RELEASED` | Acelerar (soltado) | Jugador suelta ↑ | `player_id (4B)`, `game_id (4B)` |
| `0x03` | `MOVE_DOWN_PRESSED` | Frenar (presionado) | Jugador presiona ↓ | `player_id (4B)`, `game_id (4B)` |
| `0x04` | `MOVE_DOWN_RELEASED` | Frenar (soltado) | Jugador suelta ↓ | `player_id (4B)`, `game_id (4B)` |
| `0x05` | `MOVE_LEFT_PRESSED` | Girar izq (presionado) | Jugador presiona ← | `player_id (4B)`, `game_id (4B)` |
| `0x06` | `MOVE_LEFT_RELEASED` | Girar izq (soltado) | Jugador suelta ← | `player_id (4B)`, `game_id (4B)` |
| `0x07` | `MOVE_RIGHT_PRESSED` | Girar der (presionado) | Jugador presiona → | `player_id (4B)`, `game_id (4B)` |
| `0x08` | `MOVE_RIGHT_RELEASED` | Girar der (soltado) | Jugador suelta → | `player_id (4B)`, `game_id (4B)` |

#### Lobby (Cliente → Servidor)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x10` | `CREATE_GAME` | Crear partida | Crea nueva partida | `player_id (4B)`, `game_id (4B)`, `name_len (2B)`, `name (var)`, `map_id (1B)` |
| `0x11` | `JOIN_GAME` | Unirse a partida | Se une a partida existente | `player_id (4B)`, `game_id (4B)` |
| `0x13` | `GET_GAMES` | Listar partidas | Solicita lista de partidas | `player_id (4B)`, `game_id (4B)` |
| `0x15` | `START_GAME` | Iniciar partida | Host inicia la carrera | `player_id (4B)`, `game_id (4B)` |

#### Lobby (Servidor → Cliente)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x12` | `GAME_JOINED` | Unido a partida | Confirmación de unión | `game_id (4B)`, `player_id (4B)`, `success (1B)`, `map_id (1B)` |
| `0x14` | `GAMES_LIST` | Lista de partidas | Respuesta a GET_GAMES | `count (2B)`, `[game_id (4B)`, `name_len (2B)`, `name`, `player_count (4B)`, `map_id (1B)]...` |

#### Juego (Servidor → Cliente)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x16` | `GAME_STARTED` | Juego iniciado | Notifica inicio de carrera | (sin payload) |
| `0x17` | `STARTING_COUNTDOWN` | Cuenta regresiva | Notifica inicio de countdown | (sin payload) |
| `0x20` | `UPDATE_POSITIONS` | Actualizar posiciones | Estado del juego cada frame | Ver estructura abajo |
| `0x40` | `RACE_TIMES` | Tiempos de carrera | Tiempos al finalizar ronda | `count (2B)`, `[player_id (4B)`, `time_ms (4B)`, `disqualified (1B)`, `round_idx (1B)]...` |
| `0x41` | `TOTAL_TIMES` | Tiempos totales | Tiempos del campeonato | `count (2B)`, `[player_id (4B)`, `total_ms (4B)]...` |

#### Cambio de Auto y Mejoras (Cliente → Servidor)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x30` | `CHANGE_CAR` | Cambiar auto | Cambia tipo de vehículo | `player_id (4B)`, `game_id (4B)`, `car_type_len (2B)`, `car_type (var)` |
| `0x18` | `UPGRADE_CAR` | Mejorar auto | Aplica mejora al auto | `player_id (4B)`, `game_id (4B)`, `upgrade_type (1B)` |

#### Cheats (Cliente → Servidor)

| Opcode | Valor Hex | Nombre | Descripción | Payload |
|--------|-----------|--------|-------------|---------|
| `0x50` | `CHEAT_CMD` | Comando cheat | Ejecuta un cheat | `player_id (4B)`, `game_id (4B)`, `cheat_type (1B)` |

## Estructura de Mensajes

### UPDATE_POSITIONS Payload

```
┌────────────────────────────────────────────────────────────────┐
│                     UPDATE_POSITIONS (0x20)                    │
├────────────────────────────────────────────────────────────────┤
│ count (2 bytes) - Número de jugadores/NPCs                     │
├────────────────────────────────────────────────────────────────┤
│ Por cada entidad:                                              │
│ ├── player_id (4 bytes)                                        │
│ ├── Position:                                                  │
│ │   ├── new_X (float, 4B)                                      │
│ │   ├── new_Y (float, 4B)                                      │
│ │   ├── angle (float, 4B)                                      │
│ │   ├── on_bridge (1B)                                         │
│ │   ├── direction_x (1B)                                       │
│ │   └── direction_y (1B)                                       │
│ ├── car_type_len (2B) + car_type (string)                      │
│ ├── checkpoints_count (2B)                                     │
│ │   └── [checkpoint Position] × count                          │
│ ├── hp (float, 4B)                                             │
│ ├── collision_flag (1B)                                        │
│ ├── upgrade_speed (1B)                                         │
│ ├── upgrade_acceleration (1B)                                  │
│ ├── upgrade_handling (1B)                                      │
│ ├── upgrade_durability (1B)                                    │
│ └── is_stopping (1B)                                           │
└────────────────────────────────────────────────────────────────┘
```

**Tipos de Cheat (`cheat_type`):**

| Valor | Nombre | Descripción |
|-------|--------|-------------|
| `0` | `GOD_MODE` | Activa/desactiva modo dios (invencible) |
| `1` | `DIE` | Muere instantáneamente |
| `2` | `SKIP_LAP` | Completa la vuelta actual |
| `3` | `FULL_UPGRADE` | Maximiza todas las mejoras |

---

### Tipos de Auto Disponibles

| Identificador | Nombre |
|---------------|--------|
| `green_car` | Auto verde |
| `red_squared_car` | Auto rojo cuadrado |
| `red_sports_car` | Auto deportivo rojo |
| `light_blue_car` | Auto celeste |
| `red_jeep_car` | Jeep rojo |
| `purple_truck` | Camión púrpura |
| `limousine_car` | Limusina |

### Tipos de Mejora

| Valor | Enum | Descripción | Multiplicador |
|-------|------|-------------|---------------|
| `0` | `ACCELERATION_BOOST` | Mejora aceleración | 1.15× por nivel |
| `1` | `SPEED_BOOST` | Mejora velocidad máxima | 1.15× por nivel |
| `2` | `HANDLING_IMPROVEMENT` | Mejora manejo | 1.15× por nivel |
| `3` | `DURABILITY_ENHANCEMENT` | Mejora durabilidad | 1.15× por nivel |

*Máximo 3 niveles por estadística.*

### Mapas Disponibles

| ID | Nombre |
|----|--------|
| `0` | Liberty City |
| `1` | San Andreas |
| `2` | Vice City |

---

## Flujo de Comunicación

### Conexión y Lobby

```
Cliente                                    Servidor
   │                                          │
   │──────────── Connect ────────────────────▶│  
   │                                          │ Acceptor crea ClientHandler
   │                                          │
   │◀──────── (conexión establecida) ─────────│
   │                                          │
   │──────── CREATE_GAME (0x10) ─────────────▶│
   │         name="MiPartida", map_id=0       │ LobbyHandler procesa
   │                                          │ GameMonitor crea GameLoop
   │◀──────── GAME_JOINED (0x12) ─────────────│
   │         game_id=1, player_id=1, ok=true  │
   │                                          │
```

### Durante la Partida

```
Cliente                                    Servidor
   │                                          │
   │──────── START_GAME (0x15) ──────────────▶│
   │                                          │ GameLoop.start_game()
   │◀──────── STARTING_COUNTDOWN (0x17) ──────│
   │                                          │ (10 segundos de countdown)
   │◀──────── GAME_STARTED (0x16) ────────────│
   │                                          │
   │──────── MOVE_UP_PRESSED (0x01) ─────────▶│
   │                                          │ EventLoop procesa
   │◀──────── UPDATE_POSITIONS (0x20) ────────│ @ 60 FPS
   │◀──────── UPDATE_POSITIONS (0x20) ────────│
   │──────── MOVE_LEFT_PRESSED (0x05) ───────▶│
   │◀──────── UPDATE_POSITIONS (0x20) ────────│
   │              ...                         │
   │                                          │ (jugador/es cruza/n meta)
   │◀──────── RACE_TIMES (0x40) ──────────────│
   │         round_idx=0, times=[...]         │
   │                                          │
   │              (3 rondas)                  │
   │                                          │
   │◀──────── TOTAL_TIMES (0x41) ─────────────│
   │         totals=[...]                     │
```
