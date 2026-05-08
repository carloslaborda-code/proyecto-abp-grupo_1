# Binary Assault

`Binary Assault` es un tower defense 2D por carriles desarrollado en `C++17` con `SFML`, `TinyXML2` y `CMake`.
El proyecto combina defensa táctica, gestión de recursos, dificultad adaptativa y una campaña en modo historia con progresión narrativa por etapas y oleadas.

Este README está orientado a desarrollo: documenta el estado real del código, cómo se compila, cómo está organizado y qué responsabilidad tiene cada subsistema principal.

## Descripción general

El jugador debe impedir que varias oleadas de robots alcancen el núcleo de la ciudad. La partida se desarrolla sobre un mapa por carriles cargado desde un archivo `TMX`, donde se colocan defensas en casillas predefinidas. Cada modo de juego ajusta economía, oleadas, presión enemiga y reglas especiales.

La base del proyecto está en un bucle de estados sencillo:

- `MainMenuState`: gestiona portada, menú principal, ajustes, créditos y ayuda.
- `InGameState`: concentra la simulación de partida, render, input, HUD, narrativa y coordinación de sistemas.

## Objetivo del juego

- Los enemigos aparecen en el extremo derecho del mapa y avanzan hacia el núcleo.
- Si alcanzan el núcleo, su estabilidad disminuye.
- Si la estabilidad del núcleo llega a `0`, la partida termina en derrota.
- La victoria se alcanza al superar todas las oleadas del modo actual o, en infinito, resistiendo sectores sucesivos.

## Estado actual del desarrollo

El proyecto está en un estado jugable y relativamente consolidado:

- menú principal funcional con varias pantallas auxiliares,
- carga de nivel desde `TMX`,
- HUD y overlays de pausa / derrota / victoria,
- tres modos de juego diferenciados,
- economía, oleadas, proyectiles y colisiones implementados,
- narrativa integrada en el modo historia,
- audio de menú, partida y efectos puntuales,
- y varios subsistemas desacoplados respecto a una versión más monolítica.

## Características principales implementadas

- Campaña `Historia` en `3` etapas, con `3` oleadas por etapa y transmisiones narrativas durante la preparación.
- Modo `Desafío` con mayor presión y sin reparación del núcleo.
- Modo `Infinito` con escalado por sectores.
- Cuatro defensas jugables: `Firewall`, `EMP`, `Servidor auxiliar` y `Slow Node`.
- Cuatro tipos de enemigo: `Standard`, `Fast`, `Heavy` y `Adaptive`.
- Reabsorción enemiga con habilidades activas por tipo.
- Dificultad adaptativa según etapa, carriles desatendidos, impactos al núcleo y distancia media de eliminación.
- Sistema de mejora temporal de carril.
- Modo dios de depuración activable con `D`.

## Modos de juego

### Historia

- Progresión cerrada de `3` etapas.
- Cada etapa tiene `3` oleadas definidas en `WaveManager`.
- Permite reparación del núcleo.
- Incluye briefings, transiciones de capítulo y una narrativa progresiva entre oleadas.

### Desafío

- Diseñado como modo más punitivo.
- Sin reparación del núcleo.
- Menor margen económico y oleadas más tensas.
- Usa `4` oleadas por escenario.

### Infinito

- Estructura por sectores (`tier`).
- Escalada continua de cantidad enemiga, mezcla de tipos y economía pasiva.
- Mantiene reparación del núcleo.
- Reconfigura la partida al superar cada tramo.

## Mecánicas principales

### Colocación de defensas

Las defensas solo pueden colocarse en casillas prefijadas por carril. La construcción consume energía salvo en `modo dios`. En `Historia` el límite máximo de defensas es fijo, mientras que en otros modos ese límite depende de etapa y oleada.

### Preparación entre oleadas

Antes de cada oleada existe una fase de preparación controlada por `WaveManager`. Durante esa ventana el jugador puede reorganizar su despliegue y recibe créditos extra de preparación.

### Reparación del núcleo

La reparación se ejecuta con `RepairCoreCommand`:

- comprueba si el modo permite reparar,
- verifica que el núcleo no esté al máximo,
- gasta créditos,
- recupera `1` punto de estabilidad,
- y aplica un `cooldown`.

### Mejora de carril

La mejora de carril es un buff temporal aplicado al carril seleccionado:

- aumenta cadencia de disparo,
- acelera proyectiles,
- mejora la producción de los servidores,
- y se comparte con la habilidad de reabsorción del enemigo `Fast`.

### Reabsorción enemiga

`AbsorptionSystem` acumula progreso por tipo de enemigo derrotado y otorga hasta `2` cargas por categoría:

- `Standard`: pulso ofensivo en el carril actual.
- `Fast`: potenciación temporal del carril actual.
- `Heavy`: reparación masiva de defensas y posible recuperación de estabilidad del núcleo.
- `Adaptive`: ralentización global temporal de enemigos.

### Dificultad adaptativa

`DifficultySystem` ajusta:

- velocidad enemiga,
- vida enemiga,
- frecuencia efectiva de aparición,
- multiplicador de recursos,
- ingreso pasivo,
- y presión por carril.

La presión por carril aumenta si un carril pasa demasiado tiempo sin defensas, y la partida también responde a impactos al núcleo y al promedio de distancia al que el jugador elimina enemigos.

## Defensas

### Firewall

Defensa ofensiva principal.

- Busca y mantiene un objetivo en su carril.
- Dispara proyectiles horizontales mediante `DefenseSystem`.
- Usa `targetEnemyId` para conservar el bloqueo sobre un enemigo válido mientras siga en rango.

### EMP

Defensa táctica por fases.

- `Placement`: fase inicial de colocación.
- `Armed`: espera a que un enemigo entre en la distancia de activación.
- `Triggered`: muestra estado visual de explosión y luego desaparece.

Al activarse, elimina enemigos en el radio de explosión del carril y reproduce un sonido específico.

### Servidor auxiliar

Defensa económica.

- No dispara.
- Genera créditos de forma periódica.
- Su producción real se modula por dificultad, cantidad de servidores activos y mejora de carril.

### Slow Node

Defensa de control.

- No inflige daño directo.
- Marca el carril como ralentizado.
- Los enemigos de ese carril ven reducida su velocidad por un multiplicador configurable.

## Enemigos

### Standard

- Perfil base equilibrado.
- Vida y velocidad de referencia.

### Fast

- Menor vida.
- Alta velocidad.
- Castiga carriles mal cubiertos.

### Heavy

- Vida alta.
- Avance lento.
- Más exigente en daño sostenido.

### Adaptive

- Puede cambiar de carril una vez si detecta un carril vecino claramente menos amenazante.
- Evalúa amenaza defensiva local con `chooseAdaptiveTargetLane`.
- También participa en la progresión de mezcla avanzada de oleadas.

## Narrativa del modo Historia

La campaña narrativa está integrada directamente en `InGameState` usando `StoryBriefingView`.

### Estructura narrativa

- `3` capítulos.
- `3` oleadas por capítulo.
- mensajes únicos por oleada,
- y transiciones fuertes entre etapas.

### Uso en la simulación

- Al iniciar la etapa, la simulación queda bloqueada hasta que el jugador acepta la transmisión inicial.
- Durante la preparación se dibuja el panel narrativo con el mensaje correspondiente a la oleada actual.
- Al completar una etapa se activa una transición con banner, informe táctico y overlay de continuación.

El tono narrativo está construido como comunicaciones de crisis, análisis de red y defensa del núcleo.

## Sistemas principales

### `WaveManager`

Responsable de la estructura temporal de la etapa:

- define oleadas por modo,
- controla las fases `Preparation`, `Spawning`, `Cleanup` y `Completed`,
- decide el número de oleada actual,
- entrega créditos de preparación,
- y genera solicitudes de spawn (`SpawnRequest`).

### `DifficultySystem`

Encapsula la dificultad adaptativa:

- créditos iniciales por modo,
- multiplicadores base de etapa,
- presión por carril,
- castigo adicional si el núcleo recibe impactos,
- y recompensas pasivas.

### `DefenseSystem`

Actualiza comportamiento de defensas ya colocadas:

- adquisición de objetivos del `Firewall`,
- disparo de proyectiles,
- lógica completa del `EMP`,
- y producción periódica de `Servidor auxiliar`.

### `ProjectileSystem`

Gestiona proyectiles como entidades ligeras:

- snapshot para interpolación,
- avance por velocidad,
- reducción de alcance restante,
- y eliminación fuera de rango o fuera de pantalla.

### `AbsorptionSystem`

Lleva el conteo de progreso, umbral y cargas por tipo de enemigo para la mecánica de reabsorción.

### `MatchFlowController`

Centraliza el estado macro de la partida:

- pausa,
- intermisiones,
- derrota,
- victoria,
- mensajes de banner,
- y configuración del overlay de pausa / final.

### `GraphicsFacade`

Abstrae operaciones de ventana y vista:

- creación de ventana,
- configuración del viewport del mundo,
- y comienzo / cierre de frame.

### `TextureManager`

Cachea texturas y resuelve rutas candidatas con fallback para recursos visuales.

## Arquitectura y organización del código

La arquitectura es modular, aunque `InGameState` sigue siendo el coordinador más grande del proyecto.

### Patrones realmente observables

- `State`: `State`, `MainMenuState`, `InGameState`.
- `Factory`: `DefenseFactory`, `EnemyFactory`, `LevelFactory`.
- `Command`: `PlaceDefenseCommand`, `RepairCoreCommand`, `ActivateAbilityCommand`.
- `Singleton`: `Config`.
- `Facade`: `GraphicsFacade`.

### Observación importante

Aunque existen sistemas desacoplados, `InGameState.cpp` sigue concentrando gran parte de:

- configuración de gameplay,
- coordinación de subsistemas,
- lógica de progresión,
- narrativa,
- render del mundo,
- gestión de input,
- y resolución de partida.

Es el archivo más importante para entender el comportamiento global del juego.

## Flujo principal de ejecución

### Arranque

1. `main.cpp` crea una instancia de `Game`.
2. `Game` inicializa la ventana con `GraphicsFacade`.
3. El bucle principal selecciona y ejecuta un estado.

### Menú

`MainMenuState` delega toda la UX a `MainMenu`, que incluye:

- splash,
- menú principal,
- ajustes,
- créditos,
- y pantalla de ayuda.

El modo seleccionado se guarda en `Config`.

### Partida

`InGameState::run()`:

- carga mapa, audio, fuentes y animaciones,
- crea núcleo, defensas, enemigos, proyectiles y sistemas,
- configura la etapa inicial,
- ejecuta un bucle fijo de simulación a `20 FPS`,
- renderiza con interpolación,
- y cambia de estado cuando termina la sesión o se vuelve al menú.

## Mapa y escenario

El escenario se carga desde `resources/carriles.tmx`.

### `TileMap`

Se encarga de:

- cargar capas `Logic` y `Visual`,
- leer puntos nombrados desde `Points`,
- resolver tilesets embebidos,
- y dibujar el mapa con textura o fallback.

### `LevelFactory`

Construye un `Level` jugable a partir del `TMX`:

- detecta parejas `SpawnN` / `GoalN`,
- crea carriles ordenados verticalmente,
- calcula tiles de construcción,
- y prepara grid y puntos de referencia para enemigos y defensas.

## Controles

### Menú

- `Click izquierdo`: confirmar / entrar.
- `Mover el ratón`: resaltar opciones del menú principal.

### Partida

- `Click izquierdo` sobre carta: seleccionar tipo de defensa.
- `Click izquierdo` sobre casilla: construir defensa si la casilla está libre.
- `Click derecho` sobre casilla: seleccionar carril activo.
- `Click izquierdo` en el HUD lateral: reparar núcleo, potenciar carril o activar reabsorción.
- `Click izquierdo` sobre el botón superior: abrir pausa.
- `D`: activar o desactivar `modo dios`.

## Dependencias

El proyecto depende de:

- `C++17`
- `SFML` (`system`, `window`, `graphics`, `network`, `audio`)
- `TinyXML2`
- `CMake >= 3.16`

Además, usa recursos externos locales para:

- fuentes,
- sprites,
- audio,
- y mapa `TMX`.

## Compilación

### Flujo recomendado

```bash
cd src/pro
cmake -S . -B build
cmake --build build -j4
```

### Script disponible

También existe:

```bash
./build.sh
```

### Resultado de compilación

El target generado es `MiJuego`. El `CMakeLists.txt` actual además copia el ejecutable compilado al directorio superior mediante un `POST_BUILD`, por lo que puede aparecer tanto en `build/` como en la raíz de `src/pro`.

## Ejecución

Tras compilar:

```bash
./build/MiJuego
```

En este proyecto también suele existir:

```bash
./MiJuego
```

porque `CMake` lo copia fuera de `build` al finalizar la compilación.

## Estructura de carpetas

### Código

- `main.cpp`: punto de entrada.
- `include/`: cabeceras del proyecto.
- `src/commands/`: comandos de acciones del jugador.
- `src/core/`: ventana, ciclo principal y configuración global.
- `src/entities/`: entidades base y datos de juego.
- `src/factories/`: creación de enemigos, defensas y nivel.
- `src/input/`: interpretación de clicks en partida.
- `src/map/`: carga y representación del mapa.
- `src/states/`: estados de alto nivel.
- `src/systems/`: sistemas de simulación y apoyo.
- `src/ui/`: HUD, menús y paneles visuales.

### Recursos

- `resources/audio/`: música y efectos.
- `resources/hoja-de-enemigos/`: sprites y animaciones enemigas.
- `resources/sprites_defensas/`: sprites de defensas.
- `resources/proto_ui/`: recursos del menú y HUD.
- `resources/carriles.tmx`: mapa principal del juego.

## Recursos usados por el juego

El código utiliza varios mecanismos de fallback para localizar recursos:

- `resources/...`
- `../resources/...`
- y en varios puntos aún aparece el fallback legado `src/pro/BinaryAssault/resources/...`

Esto sugiere que el proyecto fue movido o reestructurado en el pasado y todavía conserva rutas compatibles con la estructura anterior.

## Notas técnicas relevantes

- El bucle de simulación usa `fixed timestep` de `1/20` segundos.
- El render usa interpolación basada en snapshot para enemigos y proyectiles.
- `TileMap` implementa fallback visual si no puede dibujar con tileset.
- El `HUD` se dibuja en una vista separada respecto al mundo.
- El menú principal usa secuencias animadas de frames para fondo y paneles.
- El sistema de audio de partida usa `sf::Music` también para efectos one-shot.

## Limitaciones y consideraciones actuales

- `InGameState.cpp` es muy grande y centraliza demasiadas responsabilidades.
- Persisten rutas de recursos heredadas con referencia a `src/pro/BinaryAssault`, aunque el proyecto actual vive en `src/pro`.
- La copia automática del binario fuera de `build` puede ser cómoda para ejecución rápida, pero complica algo la limpieza del árbol.
- La documentación histórica no siempre coincide con la estructura actual, por lo que conviene tomar este README como referencia principal.

## Resumen de diseño narrativo

El modo historia ya no se limita a un briefing único por etapa.

- cada oleada tiene un texto propio,
- la información evoluciona dentro del capítulo,
- las transiciones entre etapas elevan el conflicto,
- y el cierre de la campaña identifica a `Atlas` como inteligencia hostil que dirige el asedio.

La narrativa está integrada funcionalmente en el flujo de preparación e intermisión, no solo como texto decorativo.

## Resumen del proyecto para desarrollo

`Binary Assault` es un tower defense por carriles con una base técnica clara:

- un núcleo de simulación concentrado en `InGameState`,
- subsistemas separados para oleadas, dificultad, defensas, proyectiles, absorción, overlays y recursos,
- una arquitectura con `State`, `Factory`, `Command`, `Singleton` y `Facade`,
- y una campaña narrativa ya integrada en el bucle jugable.

Para extender el proyecto, los puntos de entrada más importantes son:

- `src/states/InGameState.cpp`
- `src/systems/`
- `src/factories/`
- `src/ui/`
- y `resources/carriles.tmx`
