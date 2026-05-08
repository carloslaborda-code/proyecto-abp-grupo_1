# Game Design Document / Ten Sheet

## 1. Título del juego

**Binary Assault**

Binary Assault es un juego de estrategia y defensa por carriles desarrollado en C++ con SFML. El jugador debe proteger un núcleo central frente a oleadas de enemigos digitales mediante la colocación inteligente de defensas, la gestión de recursos y la reacción táctica ante diferentes tipos de amenaza.

## 2. Concepto general

El proyecto combina la estructura clásica de un *tower defense* con una ambientación tecnológica. La partida se desarrolla sobre un mapa dividido en carriles, donde los enemigos avanzan hacia el núcleo y el jugador debe construir defensas en posiciones válidas para detenerlos.

La propuesta no se limita a mover enemigos y disparar proyectiles: el juego incorpora economía, tipos de defensa con funciones diferenciadas, oleadas progresivas, interfaz fija, menús, pausa, detección de victoria o derrota y carga de mapa desde Tiled. El resultado es una beta jugable con un ciclo de partida completo.

## 3. Objetivo del jugador

El objetivo principal es mantener la estabilidad del núcleo hasta superar todas las oleadas enemigas. El jugador empieza con una cantidad limitada de créditos y debe decidir qué defensa colocar, en qué carril y en qué momento.

La partida termina con victoria si se eliminan todos los enemigos de las oleadas. La derrota se produce cuando demasiados enemigos consiguen atravesar las defensas y dañar el núcleo.

## 4. Pilares de diseño

- **Claridad:** el jugador entiende rápidamente qué debe defender, dónde aparecen los enemigos y dónde puede construir.
- **Decisión táctica:** cada defensa tiene un coste y una utilidad distinta, por lo que no existe una única respuesta válida.
- **Progresión:** las oleadas aumentan en cantidad y presión, introduciendo enemigos más rápidos o resistentes.
- **Feedback visual:** el HUD, los proyectiles, las animaciones y los cambios de estado comunican lo que ocurre durante la partida.
- **Escalabilidad:** la arquitectura separa estados, entidades, fábricas y sistemas, facilitando ampliar enemigos, defensas, niveles o reglas.

## 5. Bucle principal de juego

1. El jugador entra desde el menú principal.
2. Observa el mapa, los carriles y sus créditos disponibles.
3. Selecciona una defensa desde las cartas inferiores.
4. Coloca la defensa en una casilla válida del mapa.
5. Los enemigos aparecen por oleadas y avanzan hacia el núcleo.
6. Las defensas atacan, generan recursos o modifican el comportamiento enemigo según su tipo.
7. El jugador obtiene créditos al derrotar enemigos o mediante servidores auxiliares.
8. La partida se resuelve con victoria al superar todas las oleadas o derrota si el núcleo pierde su estabilidad.

Este bucle permite partidas cortas, fáciles de probar y con decisiones constantes.

## 6. Mecánicas principales

### Colocación de defensas

El jugador puede seleccionar una defensa con el ratón y colocarla sobre una casilla disponible. La construcción consume créditos, por lo que cada colocación implica una decisión de coste y oportunidad.

Defensas implementadas:

| Defensa | Función | Coste |
| --- | --- | --- |
| Firewall | Defensa ofensiva principal. Dispara proyectiles de forma constante. | 42 créditos |
| EMP | Defensa de alto impacto con enfriamiento mayor. Elimina amenazas cercanas mediante una descarga. | 70 créditos |
| Servidor auxiliar | Defensa económica. Genera créditos periódicamente. | 58 créditos |
| Slow Node | Defensa de control. Reduce la velocidad de los enemigos del carril. | 54 créditos |

### Oleadas de enemigos

El juego cuenta con tres oleadas configuradas con distinta cantidad de enemigos, intervalo de aparición y probabilidad de enemigos especiales. Esta estructura crea una progresión clara: al principio se introduce la mecánica y después aumenta la presión sobre el jugador.

| Oleada | Enemigos | Intervalo de aparición | Peso enemigos rápidos | Peso enemigos pesados |
| --- | ---: | ---: | ---: | ---: |
| 1 | 8 | 2.6 s | 20 | 8 |
| 2 | 12 | 2.15 s | 30 | 14 |
| 3 | 16 | 1.75 s | 38 | 22 |

### Tipos de enemigos

Los enemigos se diferencian por velocidad, resistencia y tamaño. Esto obliga al jugador a combinar daño, control y economía.

| Enemigo | Rol jugable | Vida | Velocidad base |
| --- | --- | ---: | ---: |
| Standard | Amenaza equilibrada. | 145 | 72 |
| Fast | Enemigo rápido que presiona los carriles menos defendidos. | 105 | 108 |
| Heavy | Enemigo resistente que exige mayor daño sostenido. | 225 | 50 |

### Economía

La economía empieza con 110 créditos. El jugador gana créditos al derrotar enemigos y también puede invertir en servidores auxiliares para generar ingresos periódicos. Esta mecánica evita que la partida sea solo reactiva: construir economía pronto puede facilitar la defensa de oleadas posteriores, pero también deja menos recursos inmediatos para frenar enemigos.

### Combate y colisiones

El combate se basa en proyectiles, rangos, detección de impacto y reducción de vida. Los enemigos también pueden atacar defensas, dañar estructuras y avanzar hasta el núcleo. Cuando una entidad pierde toda su vida se retira de la partida y, si corresponde, se recompensa al jugador.

## 7. Controles

| Acción | Control |
| --- | --- |
| Cambiar casilla seleccionada | Flechas |
| Seleccionar defensa | Click izquierdo sobre carta |
| Colocar defensa | Click izquierdo sobre casilla |
| Abrir o cerrar pausa | Escape |
| Continuar o volver al menú desde pausa | Click izquierdo |
| Reiniciar tras victoria o derrota | R |

## 8. Interfaz y experiencia de usuario

La interfaz está pensada para que el jugador pueda leer la partida sin perder de vista la acción. El HUD permanece fijo mediante `sf::View`, mostrando información esencial como créditos, estabilidad del núcleo, oleada actual y defensa seleccionada.

El menú principal, la pausa y los estados de victoria o derrota completan el flujo de uso. Esto hace que la beta no sea solo una escena técnica, sino una experiencia jugable cerrada: se puede iniciar, jugar, pausar, perder, ganar y reiniciar.

## 9. Tecnología y arquitectura

El juego está implementado en **C++** utilizando **SFML** para ventana, renderizado, eventos y recursos gráficos. El mapa se carga desde un fichero `.tmx` creado con Tiled y procesado con **TinyXML2**, lo que permite separar el diseño del nivel de la lógica del juego.

La arquitectura del proyecto está dividida en módulos:

- `core`: configuración general y ciclo principal del juego.
- `states`: estados como menú principal e ingame.
- `entities`: enemigos, defensas, proyectiles y núcleo.
- `systems`: economía, oleadas, colisiones y fachada gráfica.
- `factories`: creación centralizada de enemigos, defensas y niveles.
- `map`: grid, carriles, celdas, nivel y carga de mapa.
- `ui`: HUD y menús.

Patrones utilizados:

| Patrón | Uso en el proyecto |
| --- | --- |
| Singleton | Gestión centralizada de configuración mediante `Config`. |
| State | Separación entre menú principal, partida y flujo de estados. |
| Factory | Creación de enemigos, defensas y niveles sin duplicar lógica. |
| Facade | Simplificación del acceso a operaciones gráficas mediante `GraphicsFacade`. |

### Diagrama de clases

El siguiente diagrama resume la arquitectura principal de Binary Assault y permite ver cómo se relacionan las clases de entidades, sistemas, estados, factorías, mapa e interfaz.

![Diagrama de clases](diagrama_clases.png)

También se ha implementado un bucle de juego...


## 10. Alcance de la beta y defensa del proyecto

La versión actual de Binary Assault cubre los elementos necesarios para defender una entrega jugable:

- Menú principal integrado.
- HUD fijo y funcional.
- Mapa cargado desde Tiled.
- Economía básica con gasto e ingresos.
- Oleadas progresivas.
- Enemigos con comportamientos diferenciados.
- Colocación de defensas con ratón.
- Proyectiles, colisiones y eliminación de entidades.
- Pausa, reinicio, victoria y derrota.
- Organización modular del código y uso de patrones de diseño.

El valor principal del proyecto está en que combina mecánicas de juego reales con una arquitectura preparada para crecer. No es un prototipo aislado de una sola mecánica, sino una beta con ciclo completo, sistemas conectados y decisiones de diseño justificables.

## Posibles mejoras futuras

- Incluir más niveles con diferentes distribuciones de carriles.
- Añadir mejoras o evolución de defensas durante la partida.
- Incorporar más tipos de enemigos con habilidades especiales.
- Ajustar la dificultad mediante curvas de balance más precisas.
- Mejorar el apartado sonoro y los efectos visuales de impacto.
- Crear un tutorial inicial para introducir las reglas al jugador.

## Enlace al gameplay

Pendiente de insertar enlace al vídeo de gameplay final.
