# Roles y Aportes del Equipo

## Agustín Dubovitsky Otero

Al inicio se le designó la **lógica de juego**.

Primero realizaría la conexión del protocolo con el servidor para poder realizar una partida con jugadores.
Luego escalaría la estructura del servidor para poder manejar múltiples partidas con múltiples jugadores.

Una vez hecha la base para poder empezar partidas, se pensaba que Agustín realizara todas las features de una partida (movimiento, checkpoints, etc.). Sin embargo, luego de implementar los movimientos básicos, puso enfoque en la integración de Box2D para poder aplicar físicas y colisiones.

Desde entonces comenzó con las colisiones, utilizando Tiled Maps, y dado que implicaba muchas horas de trabajo, terminó dedicándose mayormente a mapear las colisiones del mapa.

Mientras Agustín trabajaba con colisiones en el mapa, Nacho, quien tenía asignado el Protocolo y ya lo había terminado, continuó la lógica que había dejado Agustín, escalando y añadiendo más funcionalidades.

Para evitar conflictos al realizar cambios en la lógica del juego, Agustín siguió añadiendo features del mapa, como puentes y objetos por los que el jugador visualmente debe pasar por debajo. Implementó y dibujó esta lógica.

Cuando finalizó estas tareas, Nacho ya había implementado una lógica de NPCs que utilizan *waypoints* dibujados en el mapa, por lo que se le asignó a Agustín que completara esta lógica dibujando los waypoints en el editor que venía utilizando.

Una vez terminadas todas las features del mapa, Agustín y Nacho trabajaron en conjunto para definir e implementar el comportamiento del taller de mejoras, donde se permitió que los autos mejoren sus estadísticas y reciban penalizaciones de tiempo.

Si bien inicialmente se le asignó la lógica de juego con foco en las features de carrera, terminó enfocándose más en la jugabilidad del mapa. Aun así, también participó en la implementación de features de jugabilidad de la carrera.

---

## Ignacio Arisnabarreta

Se le designó la parte del **Protocolo**.

Primero realizó el apartado de *threads* donde conectó el cliente con el servidor, cada uno con sus respectivos hilos, similar al trabajo práctico anterior. Luego, tras definir un “diccionario” de opcodes, comenzó a realizar otras tareas ya que necesitaba que el sistema escalara para poder escalar el protocolo también.

Como Agustín ya había realizado el flujo principal del servidor y la creación de partidas, Ignacio se dedicó a implementar features escalando el sistema del servidor, como por ejemplo NPCs y checkpoints, además de aplicar una guía que encontró en internet para realizar físicas más realistas del auto.

Luego de esto se juntó con Agustín para implementar features restantes como el Taller de mejora, a la vez que escalaba el protocolo para mantener sincronizada la comunicación entre cliente y servidor.

---

## Nicolás Ariel Domínguez

Se le designó la parte de **SDL**.

Lo primero implementado fue una versión muy simple con el fondo del mapa y un auto moviéndose por inputs del teclado, sin conexión al servidor.

Posteriormente, esos inputs pasaron a enviarse al servidor, y SDL pasó a leer mensajes del servidor, que ahora enviaba una lista de posiciones. Fue necesario renderizar más de un auto.
Se implementó una clase Cámara encargada de calcular qué parte del mapa mostrar, siempre dentro del tamaño total, y siguiendo al auto del jugador.

Luego se implementó el minimapa, una versión reducida del mapa donde los autos se representan como puntos.

Después se añadieron los checkpoints, círculos en el mapa con una flecha apuntando al siguiente checkpoint. También se agregó un indicador en pantalla que apunta al próximo checkpoint.

Se añadieron animaciones para explosiones y colisiones, además de sonidos para estos eventos (con volumen según distancia) y música de fondo.
También movió las constantes de físicas a un archivo .yaml, e implementó la clase que lee y almacena dichas físicas en runtime.

Implementó también la pantalla de resultados, que muestra las mejoras actuales del jugador.

Como curiosidad: la cámara seguía el primer elemento del vector de posiciones si no encontraba el auto principal, generando accidentalmente un “modo espectador”. Se decidió completar esta feature aunque no estaba en la consigna.

---

## Agustín Puglisi

Al comienzo del proyecto se le asignó la **interfaz gráfica con Qt**.
Desarrolló toda la UI del cliente (`client_ui`), incluyendo menús, pantalla de lobby y selección de partida/mapa.

A mitad del proyecto implementó también el editor de mapas, permitiendo crear y modificar carreras y definir el *spawnpoint*.

Para la comunicación entre UI y backend trabajó junto con Ignacio, quien codificó los mensajes del protocolo necesarios para integrar correctamente Qt con el servidor. De esta forma se fue uniendo progresivamente la interfaz con la lógica del servidor.

---

# Herramientas Utilizadas y Dificultades

El IDE utilizado fue **VSCode**, junto con su debugger para resolver bugs.

Los puntos más problemáticos fueron:

* **Físicas del auto**:
  Se usó esta guía:
  [https://www.iforce2d.net/b2dtut/top-down-car](https://www.iforce2d.net/b2dtut/top-down-car)
  El documento menciona un enfoque más realista con 4 ruedas, pero se eligió el de una rueda por simplicidad.

* **Mapeo de colisiones del mapa**:
  Consumió mucho más tiempo del esperado y sólo se logró completar el primer mapa.

* **Mapeo de sprite sheets**:
  También fue más demandante de lo previsto.

Existen algunos bugs conocidos que no se llegaron a resolver por falta de tiempo, como:

* Colisiones que no se registran.
* Hitboxes imperfectas.
* Bugs visuales (por ejemplo, la barra de vida que aparece cuando los autos explotan).

Finalmente, sentimos que partes del trabajo como mapear colisiones o sprites demandan tiempo que no aporta al contenido de la materia. Sería útil que la cátedra proporcionara, por ejemplo, 2 mapas ya mapeados y los alumnos hicieran el tercero.
