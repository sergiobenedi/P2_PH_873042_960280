# PRÁCTICA 1 — Proyecto Hardware (UNIZAR, EUPT Teruel, curso 2026/2027)
## Desarrollo de código para el procesador ARM — Reversi de 8 casillas

> Documento de referencia generado a partir de la documentación oficial de la asignatura
> (`PH_practica1_2026.pdf`, `PH_pract1_2026_slides.pdf`, `aclaracion_fuentes.pdf`) y del material
> complementario de la Universidad Complutense de Madrid (`GuiaEntorno.pdf`, `P1-EC.pdf`,
> `P2-EC.pdf`, `Entradasalida.pdf`, y la serie `P1-LEC.pdf`…`P5-LEC.pdf`).
> Todo lo indicado aquí está trazado a un documento concreto. Cuando algo no está confirmado
> por los documentos se indica explícitamente en la sección **"Dudas / huecos"** al final: no se
> ha supuesto ni inventado nada que no apareciera en las fuentes.

---

## 0. Resumen ejecutivo (léelo primero)

- La Práctica 1 **NO usa LEDs, pulsadores ni el display de 8 segmentos**. El jugador humano
  introduce sus movimientos **escribiendo directamente en memoria** con el depurador
  (variables `fila`, `columna`, `ready`). Este punto es explícito en el guión oficial (pág. 10) y se
  confirma leyendo `reversi8_2026.c`: no hay ninguna referencia a GPIO, pines, EINT ni al display.
- Si el repositorio de código contiene `led.c`, `8led.c` o `button.c`, **no forman parte de las
  "Fuentes" oficiales de esta práctica** (ver §7). Son casi con toda seguridad material de
  referencia/plantilla heredado del proyecto de ejemplo del timer0 o de la práctica de E/S de la
  UCM (P2-EC / P4-LEC), no ficheros que haya que completar para el Reversi. Trátalos como
  material de consulta, no como parte del entregable, salvo que el profesor indique lo contrario.
- Lo que sí hay que construir en esta práctica es:
  1. `patron_volteo_arm()` y `color_ficha_arm()` en ensamblador ARM (equivalentes a las
     funciones C `patron_volteo()` y `color_ficha()`).
  2. `color_ficha_thumb()` en ensamblador Thumb.
  3. Una pequeña librería de medición de tiempos con **timer 2**: `timer2_inicializar()`,
     `timer2_empezar()`, `timer2_leer()`, `timer2_parar()`.
  4. Comparativas de tamaño/instrucciones/tiempo entre las 2×3 combinaciones posibles y
     entre niveles de optimización del compilador (`-O0`…`-O3`, `-Os`) para la combinación C-C.
- Restricción más grave: **no romper la modularidad** (no fusionar `color_ficha` dentro de
  `patron_volteo_arm`, no acceder a variables locales de otra función). El guión dice literalmente
  que esto "implica un suspenso en la práctica".
- El código fuente game (`reversi8_2026.c`) es una **versión beta con, al menos, un posible bug**
  detectado al leer el fuente (ver §8). No lo corrijas por tu cuenta sin avisar: el propio guión pide
  avisar de fallos, no arreglarlos silenciosamente.

---

## 1. Objetivo y alcance exacto de la práctica

Fuente: `PH_practica1_2026.pdf` (guión oficial 2026/2027), secciones "Introducción" y "Objetivos".

### 1.1. Qué hay que implementar (trabajo del alumno)

Numerado tal y como aparece en el guión ("Trabajo a realizar"):

1. **Paso 1** — Estudiar la documentación (GuiaEntorno, P1-EC, P2-EC, EntradaSalida).
2. **Paso 2** — Estudiar y depurar el código inicial del juego en C:
   - Insertar el fichero del juego en el proyecto ejemplo, crear las cabeceras necesarias.
   - Modificar `main()` para que, tras inicializar el sistema, llame a `reversi8()`.
   - **Las funciones no llamadas desde `main()` no deben ser visibles** (usar `static` donde
     corresponda; ver también las diapositivas sobre `static`/`volatile`/alcance de variables).
   - Monitorizar la ejecución en C verificando el tablero en memoria.
   - Observar el ensamblador generado por el compilador y depurarlo paso a paso.
3. **Paso 3** — Implementar `patron_volteo_arm()` y `color_ficha_arm()` en **ensamblador ARM**,
   equivalentes a `patron_volteo()` y `color_ficha()`, con las optimizaciones que se consideren
   oportunas. `patron_volteo_arm()` puede llamar a `color_ficha()` original en C, en ARM o en
   Thumb.
   - El código ARM debe tener **la misma estructura** que el C: cada función, variable o
     condición debe poder identificarse fácilmente en la versión ensamblador (comentarios).
   - **Modularidad obligatoria**: no eliminar la llamada a `color_ficha()`, no integrarla en
     `patron_volteo_arm()`, no eliminar paso de parámetros, no acceder a variables locales de
     otra función desde fuera. El guión califica el incumplimiento de "fallo muy grave... implica
     un suspenso en la práctica".
   - La llamada debe ser una llamada convencional a función, pasando parámetros por
     registro según **AAPCS** (no un salto sin paso de parámetros).
   - El marco de pila de las distintas combinaciones debe ser **idéntico al generado por el
     compilador**, y explicarse en la memoria.
   - **No se puede modificar el algoritmo** del código C (p. ej. pasar de recursivo a iterativo)
     al pasar a ensamblador; cualquier cambio de algoritmo debe consultarse antes con el
     profesor.
4. **Paso 4** — Implementar `color_ficha_thumb()`, una nueva versión de `color_ficha()` en
   **ARM Thumb**, para reducir el tamaño en bytes de la zona crítica.
   - Para las llamadas a ARM se debe crear marco de pila como en las transparencias.
   - En Thumb se permite reducir el marco de pila (p. ej. omitir PC y FP), pero sigue sin poder
     alterarse el contenido de registros privados de otras funciones.
5. **Paso 5** — Comprobar que las 2×3 combinaciones posibles funcionan correctamente
   (`patron_volteo()`/`patron_volteo_arm()` × `color_ficha()` en C/ARM/Thumb).
6. **Paso 6** — Estudiar el proyecto de ejemplo del **timer0** que se entrega (cómo se gestionan
   dispositivos e interrupciones, ejecutar paso a paso el ensamblador de las rutinas de
   tratamiento de interrupción). El timer0 se gestiona con **interrupciones IRQ**.
7. **Paso 7** — Desarrollar la librería de medición de tiempos con **timer 2** (detalle en §3.3).
8. **Paso 8** — Medidas de rendimiento reales sobre la placa: comparar tamaño de código
   (bytes), instrucciones ejecutadas y tiempo de ejecución de cada combinación, para el
   movimiento concreto de **ficha negra en fila 2, columna 3**.
9. **Paso 9** — Estudiar el impacto de los niveles de optimización del compilador
   (`-O0, -O1, -O2, -O3, -Os`) — **obligatorio solo para la combinación C-C** de
   `patron_volteo()`/`color_ficha()` — y compararlo con las otras combinaciones.
10. **Apartado opcional** — Repetir la comparación de rendimiento con otro movimiento que
    voltee más fichas que el de fila 2 - columna 3.

### 1.2. Qué viene ya dado (no hay que programarlo desde cero)

- El código C completo del juego: `reversi8_2026.c` (funciones `init_table`, `esperar_mov`,
  `color_ficha`, `patron_volteo`, `voltear`, `actualizar_tablero`, `elegir_mov`, `contar`,
  `actualizar_candidatas`, `reversi8`). **No tiene `main()`.**
- Un "proyecto ejemplo" que usa el timer0 de la placa (sirve de plantilla/esqueleto a seguir
  para la estructura del proyecto real sobre placa).
- Ficheros de arranque para el **emulador**: `init_b_2026.asm` y `ld_script.ld` (raíz del
  repositorio; ver §7 para la diferencia con los de la placa real).
- Documentación de apoyo (GuiaEntorno, P1-EC, P2-EC, EntradaSalida, resumen de
  instrucciones ARM y Thumb, manual de arquitectura ARM, guía de memoria técnica).

### 1.3. Objetivos formativos declarados

Interactuar con la placa real (Embest S3CEV40) y el simulador (plugin Zylin/ARM7TDMI);
conocer el pipeline segmentado de 3 etapas del ARM7; depurar código ensamblador generado
por el compilador; gestionar E/S básica desde C; programar rutinas de tratamiento de
interrupción; usar temporizadores internos; desarrollar y optimizar ensamblador ARM;
entender y aplicar el AAPCS para combinar C y ensamblador; depurar siguiendo el estado
arquitectónico (registros y memoria).

---

## 2. Especificación funcional del Reversi de 8 casillas

Fuentes: `PH_practica1_2026.pdf` (sección "El Juego: Reversi" y "¿Cómo funciona el código que
nos han dado?"), `reversi8_2026.c`, diapositivas 7–10 de `PH_pract1_2026_slides.pdf`.

### 2.1. Reglas del juego

- Tablero de 8×8 (`DIM = 8`). Estados de casilla: `CASILLA_VACIA = 0`, `FICHA_BLANCA = 1`,
  `FICHA_NEGRA = 2`.
- Posición inicial: `tablero[3][3] = BLANCA`, `tablero[4][4] = BLANCA`, `tablero[3][4] = NEGRA`,
  `tablero[4][3] = NEGRA`.
- **El procesador (máquina) juega con blancas; el jugador humano juega con negras.**
- **Empiezan las negras** (el jugador humano mueve primero) — al revés que el ajedrez.
- Los jugadores se turnan colocando una ficha; solo se puede colocar si al hacerlo se rodea
  a una o varias fichas rivales en alguna de las 8 direcciones (N, S, E, O y las 4 diagonales).
- Las fichas rodeadas cambian de color ("se voltean").
- Si un jugador no tiene ninguna posición libre válida, debe pasar.
- El juego termina cuando **ningún jugador puede mover**. Gana quien tenga más fichas de
  su color (función `contar()`).
- **El código no comprueba que los movimientos del humano sean válidos** (el guión lo dice
  explícitamente: "de hecho, escribiendo en memoria podéis hacer tantas trampas como
  queráis"). Sí se exige que los movimientos de la máquina sean siempre correctos.

### 2.2. Interacción con el jugador — SIN periféricos de E/S

**Esto es lo más importante a tener en cuenta para no perder tiempo buscando código de
botones/leds que no existe en esta práctica.** El guión dice textualmente: *"Como en esta
versión todavía no hay ningún dispositivo de entrada/salida, el jugador debe realizar sus
movimientos escribiendo directamente en memoria."*

Mecanismo (variables globales en `reversi8_2026.c`, colocadas justo después del array
`tablero` para que ocupen memoria contigua y sean fáciles de localizar en el monitor de
memoria del depurador):

```c
char fila = 0, columna = 0, ready = 0;
```

Flujo de una partida en el simulador (depurador Eclipse/gdb):

1. Se pone un breakpoint en `reversi8()` justo en la línea que invoca a `esperar_mov(&ready)`.
   Cada vez que le toca mover al jugador, el simulador para ahí.
2. Se abre un monitor de memoria sobre la dirección de `tablero` (formato "hex integer", 8
   columnas) para visualizar el tablero de 8×8. `tablero` está declarado con
   `__attribute__((aligned(8)))` precisamente para que el monitor de memoria lo muestre
   correctamente alineado en filas de 8 bytes.
3. El jugador escribe directamente en las posiciones de memoria de `fila` y `columna` (justo
   después del tablero) el movimiento deseado, y luego escribe `1` en `ready`.
4. Se reanuda la ejecución (F8 / "resume"); tras esperar en `esperar_mov()`, el juego aplica el
   movimiento del humano, calcula el de la máquina y actualiza el tablero.
5. Para **pasar turno** (no poder mover), el jugador introduce `fila = 8, columna = 8` (valores
   fuera de rango 0–7, que el código interpreta como "no puedo mover").
6. `esperar_mov()` hace un `while (*ready == 0) {}` de espera activa y pone `ready = 0` al salir;
   por eso la propia práctica hace notar (comentario en el código) que probablemente haga
   falta el calificador `volatile` en `fila`, `columna` y `ready` para que el compilador no
   optimice esa espera (pregunta retórica que deja el propio enunciado del código,
   no una instrucción a ciegas: ver "Dudas/huecos").

### 2.3. Qué NO hace esta práctica con hardware de E/S

- No usa LEDs.
- No usa pulsadores/botones.
- No usa el display de 8 segmentos.
- No usa el teclado matricial.
- No usa la UART.
- El único periférico de E/S "real" que interviene en el Paso 6–7 es el **timer0** (proyecto de
  ejemplo, gestionado por interrupción IRQ) y el **timer2** (librería de medición de tiempos que
  hay que programar, ver §3.3). Ambos son E/S localizada en memoria (memory-mapped I/O),
  no dispositivos con pines externos visibles.

### 2.4. Funciones del juego (de `reversi8_2026.c`)

| Función | Rol |
|---|---|
| `init_table(tablero, candidatas)` | Vacía el tablero, coloca las 4 fichas centrales y marca las casillas candidatas iniciales. |
| `color_ficha(tablero, f, c, *es_posicion_correcta)` | Devuelve el contenido de `tablero[f][c]` si `f,c` están en rango y la casilla no está vacía; si no, devuelve `CASILLA_VACIA` y marca `*es_posicion_correcta = 0`. **Es la función que hay que traducir a ARM y a Thumb.** |
| `patron_volteo(tablero, *longitud, FA, CA, SF, SC, color)` | Recorre en la dirección `(SF,SC)` a partir de `(FA,CA)` buscando `n` fichas rivales seguidas de una ficha del color actual. Devuelve `MODELO_ENCONTRADO` o `NO_HAY_MODELO`. **Es la función que hay que traducir a ARM.** Llama a `color_ficha()`. |
| `voltear(tablero, FA, CA, SF, SC, n, color)` | Voltea `n` fichas en la dirección indicada. |
| `actualizar_tablero(tablero, f, c, color)` | Para las 8 direcciones, llama a `patron_volteo()` y, si hay modelo, llama a `voltear()`. |
| `elegir_mov(candidatas, tablero, *f, *c)` | Recorre las casillas candidatas, evalúa jugadas válidas con `patron_volteo()` y elige la de mayor valor según `tabla_valor` (tabla estática de pesos por casilla, favorece esquinas y penaliza casillas junto a esquina). Devuelve `-1` si no hay movimiento posible. |
| `contar(tablero, *b, *n)` | Cuenta fichas blancas y negras. |
| `actualizar_candidatas(candidatas, f, c)` | Marca la casilla jugada como ocupada y las 8 vecinas como candidatas a explorar. |
| `esperar_mov(*ready)` | Espera activa hasta que `ready != 0`; ver §2.2. |
| `reversi8()` | Bucle principal: inicializa tablero, espera movimiento humano, lo aplica si es válido (`fila,columna != 8`), calcula y aplica el movimiento de la máquina con `elegir_mov`, termina cuando ninguno de los dos puede mover. |

### 2.5. Constantes / tablas relevantes

- `vSF[8] = {-1,-1,0,1,1,1,0,-1}`, `vSC[8] = {0,1,1,1,0,-1,-1,-1}`: desplazamientos de fila/
  columna para las 8 direcciones (0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SO, 6=O, 7=NO,
  según el propio comentario del código en `actualizar_tablero`).
- `tabla_valor[8][8]`: pesos estáticos por casilla usados por `elegir_mov` para puntuar
  movimientos (mayor peso en esquinas, menor junto a ellas).
- `candidatas[f][c]` puede valer `NO(0)`, `SI(1)` o `CASILLA_OCUPADA(2)`.

### 2.6. Movimiento de referencia para las medidas de rendimiento

El Paso 8 pide medir el tiempo/instrucciones/tamaño para el cálculo del volteo al colocar
**ficha negra en fila 2, columna 3** (coincide con el ejemplo ilustrado en las diapositivas,
"Ejemplo de movimiento"). El apartado opcional pide repetir la comparación con un
movimiento que voltee más fichas.

---

## 3. Detalles de hardware relevantes

> **Importante**: la mayor parte del hardware descrito en `Entradasalida.pdf` (LEDs, pulsadores,
> display de 8 segmentos, teclado matricial, UART, DMA) **no se usa en esta práctica**. Se deja
> aquí documentado por si la sesión de código necesita descartar código relacionado con esos
> periféricos, pero no hace falta programarlo ni depurarlo para el Reversi. Lo único
> efectivamente usado en P1 es el sistema de memoria (mapa general) y los temporizadores
> (timer0 en el proyecto de ejemplo, timer2 en el trabajo a realizar).

### 3.1. Arquitectura y mapa de memoria (ARM7TDMI + S3C44B0X)

- CPU: **ARM7TDMI** (RISC de 32 bits, bus de direcciones de 32 bits → 4 GB direccionables).
  E/S **localizada en memoria** (memory-mapped I/O): mismo espacio de direcciones para
  memoria y periféricos, la distinción la hace hardware externo al procesador, no el propio
  ARM7TDMI.
- SoC: **Samsung S3C44B0X** (integra el ARM7TDMI + controlador de memoria + controlador
  de interrupciones + temporizadores + otros controladores de E/S).
- Placa: **Embest S3CEV40**, conexión al PC vía **JTAG**.
- El controlador de memoria del S3C44B0X reduce el espacio efectivo a **256 MB**, dividido en
  **8 bancos** de 32 MB cada uno (señales de selección `nGCS0`–`nGCS7`).
- Módulos de memoria de la placa de laboratorio:
  - **ROM Flash**: 1M×16 bits, banco 0, rango `0x00000000`–`0x001FFFFF`.
  - **SDRAM**: 4M×16 bits, banco 6, rango `0x0C000000`–`0x0C7FFFFF`.
- La parte alta del banco 0 (`0x01C00000`–`0x01FFFFFF`) está reservada a los **registros de
  E/S de los controladores internos** del S3C44B0X (memoria "no gestionada" por el
  controlador de memoria en el sentido de banco de RAM/ROM).
- El **script de enlazado del emulador** (`ld_script.ld`, raíz del repo) coloca `.text`, `.data`,
  `.rodata` y `.bss` a partir de `0x0C000000` (inicio de la SDRAM), sin tabla de vectores de
  excepción ni pilas de otros modos: es un layout mínimo, coherente con que
  `init_b_2026.asm` no configura excepciones/interrupciones (ver §3.4).
- El mapa de memoria "completo" usado en las prácticas de la placa real (`P2-EC.pdf`, fig.
  2.4) reserva, de direcciones altas a bajas dentro del banco de SDRAM: tabla de direcciones
  de ISRs (a partir de `_ISR_STARTADDRESS = 0x0C7FFF00`), pilas de los distintos modos de
  ejecución, y luego `.text/.data/.rodata/.bss` del programa, dejando el resto como heap. Esto
  aplica a los proyectos "Fuentes Placa" (con `common/44binit.asm`), no al emulador standalone.

### 3.2. Temporizadores (timer0 y timer2) — lo único de E/S que sí se usa

El S3C44B0X tiene **6 temporizadores de 16 bits** (timer0–timer5), contadores
descendentes con recarga automática (auto-reload) o de un solo disparo (one-shot).
Registros relevantes (`Entradasalida.pdf`, cap. 5):

| Registro | Dirección | R/W | Descripción |
|---|---|---|---|
| TCFG0 | 0x01D50000 | R/W | Factores de pre-escalado (uno para cada par de timers). |
| TCFG1 | 0x01D50004 | R/W | Selección de divisor de frecuencia (MUX) por timer y canal DMA. |
| TCON | 0x01D50008 | R/W | Control: start/stop, auto-reload, manual update, inversor de salida, por cada timer. |
| TCNTB*n*/TCMPB*n* | 0x01D5000C + 8·n (aprox., ver tabla completa en Entradasalida.pdf) | R/W | Registros buffer de cuenta y de comparación. |
| TCNTO*n* | — | R | Valor actual de cuenta (lectura del contador en marcha). |

- Frecuencia efectiva de un timer: `F = MCLK / ((preescalado+1) · divisor)`, con
  `MCLK = 64 MHz` en esta placa (dato explícito del guión oficial, Paso 7), preescalado en
  `[0,255]` y divisor en `{2,4,8,16,32}`.
- Secuencia de inicialización de un timer: (1) escribir valor inicial en TCNTBn/TCMPBn, (2)
  activar el *manual update bit*, (3) activar el *start bit* a la vez que se desactiva el manual
  update.
- **Timer0** (paso 6): usado en el "proyecto ejemplo" que se entrega, gestionado por
  **interrupción IRQ**. Sirve de plantilla de cómo se programan dispositivos + interrupciones en
  C, y de cómo depurar el ensamblador generado para la rutina de tratamiento de interrupción.
  No hay que modificarlo para el Reversi, solo entenderlo.
- **Timer2** (paso 7, obligatorio programarlo): librería de medición de tiempos que hay que
  escribir, con las siguientes funciones (firmas y comportamiento tal y como los da el guión):

  ```c
  void         timer2_inicializar(void); // configura timer2 a máxima precisión;
                                          // genera IRQ en cada cuenta completa
  void         timer2_empezar(void);     // reinicia contador + variable interna y arranca
  unsigned int timer2_leer(void);        // lee cuenta + nº de interrupciones; µs transcurridos
  unsigned int timer2_parar(void);       // para el timer; devuelve µs transcurridos
  ```

  - Variable interna `timer2_num_int`, compartida entre la ISR y el resto de funciones del
    módulo, que cuenta los periodos completos del temporizador.
  - Al completar la cuenta, el timer se reinicia automáticamente y sigue contando (modo
    interrupción "monótono decreciente" según la diapositiva 11).
  - **Calibración obligatoria**: comprobar la librería contra la función `Delay()` de `44blib.c`,
    midiendo 1 ms, 10 ms, 1 s y 10 s, y verificando que los resultados son parecidos (no
    idénticos, porque `Delay()` es una aproximación).

### 3.3. Interrupciones (contexto general — usado solo para timer0/timer2, no para botones)

Aunque el guión de P1 no usa pulsadores, sí exige entender y usar interrupciones **IRQ** para
los timers, así que este bloque es relevante:

- El ARM7TDMI tiene 2 líneas de interrupción externas: **IRQ** y **FIQ**, autovectorizadas
  (vectores fijos: IRQ = `0x18`, FIQ = `0x1C`), más 1 línea de RESET.
- El **controlador de interrupciones** del S3C44B0X amplía esto a **26 líneas** (30 fuentes;
  algunas líneas están compartidas), implementado con 5 controladores encadenados
  (1 maestro + 4 esclavos).
- Registros de configuración/gestión (`Entradasalida.pdf` / `P2-EC.pdf`):
  - `INTCON` (0x01E00000 según P2-EC / 0x01D20050 aparece para el registro EXTINT del
    GPIO, **no confundir ambos**): habilita interrupciones vectorizadas (bit V), IRQ (bit I),
    FIQ (bit F) — bits a `0` habilitan.
  - `INTMOD`: modo IRQ(0)/FIQ(1) por línea.
  - `INTPND`: bit a 1 = solicitud pendiente en esa línea (se actualiza aunque esté enmascarada).
  - `INTMSK`: bit a 1 = línea enmascarada (interrupción deshabilitada).
  - `I_ISPC` / `F_ISPC`: escribir un `1` en el bit de la línea borra el `INTPND` correspondiente e
    indica "fin de rutina de servicio" — **imprescindible escribir esto antes de salir de la ISR**
    o la interrupción se re-dispara.
  - `I_ISPR`: en modo vectorizado, indica qué línea se está sirviendo (solo puede haber un bit
    activo, a diferencia de `INTPND`).
- **Modo no vectorizado** vs **vectorizado**: en vectorizado (el que usa esta práctica, según la
  diapositiva 15: "En esta práctica usaremos esta opción"), el controlador inserta en el bus una
  instrucción de salto directa al vector de la línea concreta que interrumpió (p. ej. TIMER0 →
  `0x60`, EINT4/5/6/7 → `0x30`), evitando tener que consultar registros para identificar la fuente.
- Tabla de vectores por línea (subconjunto relevante para esta práctica — la lista completa de
  las 26 líneas está en `Entradasalida.pdf`/`P2-EC.pdf`):

  | Línea | Vector | Fuente |
  |---|---|---|
  | EINT4/5/6/7 | 0x30 | Interrupciones externas 4–7 (pulsadores; **no usadas en P1**) |
  | TIMER0 | 0x60 | Timer0 (proyecto de ejemplo, paso 6) |
  | TIMER2 | 0x68 | Timer2 (librería de medición de tiempos, paso 7) |

- Declaración de una ISR en C con GCC (usada en el proyecto de ejemplo y en la librería de
  timer2): `void mi_isr(void) __attribute__ ((interrupt ("IRQ")));`
- Marco de pila de una excepción/interrupción: muy parecido al de una función normal, con
  diferencias explicadas en las diapositivas 19–20: hay que restar 4 a LR antes de guardarlo
  (8 para Abort), hay que guardar también IP y r0-r3, y el retorno debe usar `^` para restaurar
  CPSR desde SPSR automáticamente.

### 3.4. EINT4-7 y pines de LEDs/pulsadores/8-segmentos — documentados, NO usados en P1

Se incluye aquí por completitud y para evitar dudas si el código heredado los menciona, pero
**no forma parte del trabajo de esta práctica** (ver §0 y §2.3):

- **Puerto B** (`PCONB` 0x01D20008, `PDATB` 0x01D2000C): los pines **PB9 y PB10**, configurados
  como salida, controlan los 2 LEDs de la placa (ánodo a Vdd; para encender hay que escribir
  un `0`).
- **Puerto G** (`PCONG` 0x01D20040, `PDATG` 0x01D20044, `PUPG` 0x01D20048): los pines
  **PG6 y PG7**, configurados como entrada, leen los 2 pulsadores (activos a nivel bajo; se
  necesitan resistencias de pull-up activadas por `PUPG` para que el valor sea `1` en reposo).
  Estos mismos pines pueden reconfigurarse (vía `PCONG`) para generar las interrupciones
  **EINT6/EINT7**, con el modo de disparo (nivel/flanco) configurado en `EXTINT` (0x01D20050).
  Los 4 sub-bits de la línea compartida EINT4/5/6/7 se distinguen leyendo `EXTINTPND`
  (0x01D20054), y hay que borrar el bit correspondiente ahí **antes** de borrar `INTPND`.
- **Display de 8 segmentos**: tipo ánodo común (poner a `0` para encender), conectado vía un
  latch triestado 74LS573 seleccionado por `CS6` en el rango `0x0214_0000–0x0217_FFFF`
  (banco 1). **Muy importante si algún día se usa**: hay que escribir siempre con `strb`
  (tamaño byte), nunca con escrituras de palabra completa, porque el banco 1 es de 8 bits de
  ancho y con little-endian se acabaría escribiendo el byte equivocado en el latch.
- Ninguno de estos registros/pines aparece usado en `reversi8_2026.c`, `init_b_2026.asm` ni
  en el guión oficial de P1. Si el código del repositorio incluye `led.c`/`8led.c`/`button.c`, revisar
  §7 y §9 antes de asumir que hace falta tocarlos.

---

## 4. Restricciones y normas explícitas

Fuente principal: `PH_practica1_2026.pdf` (cuerpo del guión + Anexos II, III, IV).

### 4.1. Modularidad y algoritmo (las dos reglas "de suspenso")

- **No fusionar funciones**: `color_ficha()`/`color_ficha_arm()`/`color_ficha_thumb()` deben
  seguir siendo *llamadas a función* independientes desde `patron_volteo_arm()`, con paso de
  parámetros real por registros (AAPCS), nunca inlineado ni sustituido por un salto sin pasar
  parámetros.
- **No acceder a variables locales de otra función** desde fuera de ella. El guión califica esto
  de "fallo muy grave" con **suspenso automático de la práctica**.
- **No cambiar el algoritmo** del C original al pasar a ensamblador (p. ej. recursivo→iterativo).
  Cualquier cambio de algoritmo debe consultarse antes con el profesor.
- El marco de pila debe describirse en la memoria y debe ser **el mismo entre combinaciones**
  para que la comparación de rendimiento sea justa; ninguna función puede alterar registros
  privados de otra.

### 4.2. Ficheros que no se deben tocar / se entregan completos

- `init_b_2026.asm` y `ld_script.ld` (fuentes del emulador): se entregan hechos; solo se
  mencionan como necesarios para depurar sin placa (Anexo I). El guión no dice
  explícitamente "no modificar", pero al ser el arranque mínimo del emulador, tocarlo
  cambiaría cómo arranca el programa — no hacerlo salvo necesidad justificada.
- El propio código de `reversi8_2026.c` es la "versión beta" que da la empresa ficticia del
  enunciado: el trabajo consiste en **traducir/optimizar** `patron_volteo`/`color_ficha`, no en
  reescribir la lógica del juego. Si se detecta un fallo (ver §8), el guión pide **avisar**, no
  corregirlo por libre.

### 4.3. Estilo de código exigido

- El ensamblador ARM debe seguir la **misma estructura** que el C correspondiente:
  cada función/variable/condición del C debe identificarse con facilidad en el ensamblador,
  con comentarios adecuados.
- Uso correcto de `static`/`extern`/`volatile`/`const` según lo tratado en las diapositivas 25–27:
  restringir a `static` toda función/variable que solo deba usarse en su propio fichero
  (reduce errores, evita colisiones de nombres entre módulos); usar `volatile` para variables
  que puedan modificarse "externamente" sin que el compilador lo vea en el flujo normal
  (candidato claro: `ready`, y posiblemente `fila`/`columna`, ver Dudas/huecos).
- **Las funciones no llamadas desde `main()` no deben ser visibles** (paso 2a del guión).

### 4.4. Qué se evalúa (Anexo II)

- Se valora especialmente que el código ARM esté **optimizado**: cuanto más rápido y
  pequeño (menos instrucciones ejecutadas, menos accesos a memoria), mejor valoración.
- Recomendaciones explícitas de optimización que el guión menciona (Anexo II):
  - Diseñar el código antes de escribirlo.
  - Mantener en registros las variables usadas con frecuencia, minimizando idas y venidas a
    memoria.
  - Usar instrucciones de transferencia múltiple (`LDMIA`, `STMIA`, `PUSH`, `POP`).
  - Usar ejecución condicional / instrucciones predicadas (p. ej. `ADDEQ`, `SUBNE`) para evitar
    saltos.
  - Usar instrucciones multi-operación como `MLA` (`r2 = r3*r4 + r5`).
  - Sustituir multiplicaciones/divisiones por potencias de 2 mediante desplazamientos (`lsl`),
    incluso integrados en otra instrucción (`ADD R1,R2,R3,lsl #1`).
  - Aprovechar el direccionamiento base+offset en `LDR`/`STR` (p. ej. `LDR R1,[R2,#4]`).
  - El propio guión avisa de que, al ser un código pequeño, no todas estas optimizaciones
    serán aplicables — no forzarlas todas.

### 4.5. Contenido obligatorio de la memoria (Anexo III)

1. Resumen ejecutivo (máx. 1 cara), independiente del resto.
2. Explicación de cómo se ha creado el marco de pila.
3. Código fuente comentado; cada función con cabecera que explique funcionamiento,
   parámetros (qué recibe y dónde), y uso de cada registro.
4. Descripción de las optimizaciones realizadas.
5. Resultados de la comparación entre las distintas versiones de las funciones.
6. Descripción de problemas encontrados y sus soluciones.
7. Conclusiones.
   (La versión de las diapositivas añade explícitamente "Firma y autoría" como punto propio.)

### 4.6. Entrega (Anexo IV)

- Vía Moodle2 (`https://moodle.unizar.es/add/course/view.php?id=177735`).
- Un único fichero ZIP por grupo, con:
  1. Memoria en PDF.
  2. Código fuente de los distintos apartados.
- Nombre exacto del fichero: `p1_NIP-Apellidos_Estudiante1_NIP-Apellidos_Estudiante2.zip`
  (ejemplo del propio guión: `p1_345456-Gracia_Esteban_45632-Arribas_Murillo.zip`).

---

## 5. Diferencias entre el guión de UNIZAR y los cuadernos de la UCM (y qué es solo referencia)

El material complementario (`GuiaEntorno.pdf`, `P1-EC.pdf`, `P2-EC.pdf`, `Entradasalida.pdf`)
procede explícitamente, según el propio guión de UNIZAR, de **"cuadernos de prácticas de
una asignatura de la Universidad Complutense que usa el mismo entorno de trabajo y las
mismas placas"**. Además, en el proyecto hay una segunda serie de cuadernos de la misma
procedencia (mismo pie de licencia Creative Commons, mismos autores L. Piñuel / C.
Tenllado), numerada de forma independiente: `P1-LEC.pdf` … `P5-LEC.pdf`. Ninguna de las
dos series es el guión que hay que seguir para el Reversi: son **material de fondo**, y cada una
tiene partes que aplican y partes que no.

### 5.1. `P1-EC.pdf` — "Práctica 1. Lenguajes de alto nivel y ensamblador" (UCM)

- **Aplica** (lectura obligatoria según el guión de UNIZAR): estructura de un programa
  ensamblador, directivas (`.global`, `.equ`, `.word`, `.space`, `.end`, secciones `.text/.data/.bss`),
  proceso de generación de un ejecutable (preprocesador → compilador → ensamblador →
  enlazador), **AAPCS** (convenio full-descending, uso de r0-r3/r4-r10/fp/sp/lr, prólogo y
  epílogo recomendados, tabla de qué registros hay que preservar), paso de más de 4
  argumentos por pila, tipos compuestos (arrays/estructuras/uniones) y su representación en
  memoria, tabla de símbolos y símbolos `.global`/`.extern`/`static`, arranque de un programa C
  (`crt0` simplificado: `ldr sp,=_stack ; mov fp,#0 ; bl main`).
- **NO aplica** (es el enunciado de la práctica propia de la UCM, no la nuestra): la sección
  1.10 "Desarrollo de la práctica" describe un ejercicio de **transformación de imagen RGB a
  escala de grises y a imagen binaria** (`rgb2gray`, `contarUnos`, ficheros `trafo.c`/`misc.S`/
  `types.h`). Es un ejercicio totalmente distinto al Reversi; no debe confundirse ni mezclarse.
  El `init.S` mínimo que describe (solo llama a `main()`) sí es conceptualmente análogo a
  nuestro `init_b_2026.asm`.

### 5.2. `P2-EC.pdf` — "Práctica 2. Modos de ejecución, gestión de excepciones y E/S mediante
interrupciones" (UCM)

- **Aplica** (lectura obligatoria según el guión de UNIZAR, "estudiarlo en detalle"): los 7 modos
  de ejecución del ARM7TDMI, autovectorización de excepciones, tabla de excepciones/
  modos/vectores (§3 de este documento la resume), mecanismo de modo vectorizado/no
  vectorizado del controlador de interrupciones, registros `INTCON/INTPND/INTMOD/INTMSK/
  I_ISPR/I_ISPC/F_ISPC`, mapa de memoria general de un programa de prácticas (fig. 2.4:
  tabla de ISRs + pilas + secciones + heap), atributo GCC `__attribute__((interrupt("IRQ")))`.
- **NO aplica directamente** — y **esta es la fuente más probable de la confusión con
  `led.c`/`button.c`/`timer.c`/`8seg.c`**: la sección 2.7 "Desarrollo de la Práctica" de `P2-EC.pdf`
  describe el ejercicio propio de la UCM, que sí usa LEDs, pulsadores, un contador
  descendente en el display de 8 segmentos y un timer, repartido exactamente en los ficheros
  `init.asm`, `boton.c`, `led.c`, `main.c`, `timer.c`, `8seg.c`, `44b.h`, `44blib.c`, `44init.asm`. **Ese
  es un ejercicio distinto al Reversi de UNIZAR.** Si el repositorio de código de esta práctica
  contiene ficheros con esos nombres, son casi con toda seguridad remanentes de esta
  plantilla de la UCM (o del "proyecto ejemplo de timer0" que la reutiliza como esqueleto),
  no una tarea pendiente del Reversi. Ver §9 antes de tocarlos.

### 5.3. `Entradasalida.pdf` (UCM) — "Sistema de memoria y de entrada/salida en la placa
S3CEV40"

- **Aplica**: cap. 1 (sistema de memoria, igual que en P2-EC) y cap. 5 (temporizadores: los
  mismos registros TCFG0/TCFG1/TCON/TCNTBn/TCMPBn/TCNTOn que hacen falta para
  programar timer2).
- **NO aplica en esta práctica** (documentado para contexto, no para implementar): cap. 3
  (GPIO, LEDs, pulsadores, EINT), cap. 4 (display de 8 segmentos), cap. 6 (teclado matricial),
  cap. 7 (UART), cap. 8 (DMA). El propio guión de UNIZAR cita este documento como
  "incluyendo los que vamos a utilizar en esta práctica" — en la práctica, lo único
  efectivamente utilizado son memoria y temporizadores; el resto es documentación de la
  placa en general, reutilizada de un curso donde sí se usan esos periféricos.

### 5.4. Serie `P1-LEC.pdf`…`P5-LEC.pdf` (UCM) — secuencia paralela de 5 prácticas

Esta serie **no está citada por el guión oficial de UNIZAR** como material de esta práctica —
parece ser una versión alternativa/ampliada de la misma asignatura de la UCM, con la
secuencia de prácticas repartida en 5 entregas en lugar de 2. Se documenta su contenido para
que sirva de referencia si aparecen dudas conceptuales, pero **ninguna de sus tareas es un
requisito de esta P1 de UNIZAR**:

| Fichero | Título | Contenido relevante como referencia |
|---|---|---|
| `P1-LEC.pdf` | Arquitectura ARM, ensamblador e introducción al entorno | Repertorio ARM básico, modos de ejecución (introducción), EmbestIDE. Solapa con `P1-EC.pdf`. |
| `P2-LEC.pdf` | Paso de alto nivel a ensamblador | Pila de llamadas, marco de activación, FP/SP. Solapa con la parte de AAPCS de `P1-EC.pdf`. |
| `P3-LEC.pdf` | Mapa de memoria, modos de ejecución y gestión de excepciones | Solapa con `P2-EC.pdf` (excepciones, modos, CPSR). |
| `P4-LEC.pdf` | Sistema de E/S y dispositivos sencillos (LEDs, pulsadores, display 8 segmentos y temporizadores) | Es el equivalente, en esta serie, al ejercicio de LEDs/pulsadores/8-seg de `P2-EC.pdf` §2.7 — **tampoco aplica al Reversi**, misma salvedad que en §5.2. |
| `P5-LEC.pdf` | Gestión avanzada de interrupciones y teclado matricial | Teclado matricial (no usado en P1). |

### 5.5. Enfoque alternativo que sí puede servir de inspiración

- El planteamiento de marco de pila con push/pop de `P1-EC.pdf` (cuadros 2 y 3) es un
  ejemplo concreto y más simple que el de las diapositivas de UNIZAR (que usa
  `STMDB`/`LDMIA` explícitos): ambos son válidos y equivalentes, se puede usar el que resulte
  más cómodo siempre que se documente en la memoria.
- El ejemplo completo de subrutina `Mayor(int X, int Y)` de `P1-EC.pdf` (págs. 15-16) es un
  buen patrón de referencia para estructurar `color_ficha_arm`/`patron_volteo_arm`, ya que
  muestra prólogo, escritura de argumentos en pila, cuerpo y epílogo con la misma
  granularidad que se pide en esta práctica (aunque el propio documento aclara que una
  traducción tan literal línea a línea no es necesaria).

---

## 6. Pasos de entrega y evaluación

- **Presentación de la práctica (código, corrección presencial)**: en torno al **6 de octubre**.
- **Entrega de la memoria**: en torno al **9 de octubre**.
- Fechas definitivas: se publican en Moodle de la asignatura (no fijadas en el guión con
  precisión absoluta — ver Dudas/huecos).
- El día de la corrección hay que tener disponibles y preparadas para mostrar: las medidas
  de tamaño de código, instrucciones ejecutadas y tiempos de ejecución de todas las
  combinaciones (Paso 8) y del estudio de niveles de optimización (Paso 9); también la
  calibración de la librería de timer2 frente a `Delay()`.
- Formato y nombre del entregable: ver §4.6.

---

## 7. Fuentes de código: qué fichero es de dónde

Esta sección resuelve de forma explícita la pregunta "¿qué ficheros tengo que tocar y cuáles
son solo infraestructura?", usando `aclaracion_fuentes.pdf` (documento de 1 página dedicado
exactamente a esto) y la diapositiva 24 de `PH_pract1_2026_slides.pdf`, que coinciden:

| Categoría | Ficheros | Notas |
|---|---|---|
| **Fuentes Emulador** | `init_b_2026.asm`, `ld_script.ld` (raíz del repo) | Arranque mínimo sin excepciones/interrupciones/pilas de modos; solo inicializa `sp` y salta a `reversi8()`. Para depurar sin placa (Anexo I). Confirmado leyendo el propio fichero: solo tiene `Reset_Handler`, sin tabla de vectores real (los demás handlers están comentados/deshabilitados). |
| **Fuentes Placa** | `./common/44binit.asm`, `./common/ld_script.ld`, `main.c` (con `main()`) | Para ejecución en la placa real Embest S3CEV40; sigue el esquema del "proyecto ejemplo" del timer0. `main()` es quien, tras inicializar el sistema, debe llamar a `reversi8()` (paso 2a del guión). |
| **Fuentes Juego** | `reversi8_2026.c` | El juego en sí; **no tiene `main()`**; expone `reversi8()` como punto de entrada al juego. |

Ficheros que el usuario menciona tener en el repositorio (`led.c`, `8led.c`, `button.c`, `timer.c`,
`common/`, `emulador/`) y que **no aparecen en ninguna de las tres categorías oficiales de
arriba**:

- `timer.c` es plausible que sea parte del "proyecto ejemplo de timer0" (Paso 6) o el esqueleto
  donde hay que añadir la librería de `timer2` — pero eso no está confirmado por ningún
  documento leído (ver Dudas/huecos).
- `led.c`, `8led.c`, `button.c` coinciden en nombre con los ficheros del ejercicio de LEDs/
  pulsadores de `P2-EC.pdf` §2.7 (`led.c`, `boton.c`) — muy probablemente son plantilla
  heredada de ese ejercicio de la UCM (posiblemente incluida porque el "proyecto ejemplo"
  que da la asignatura reutiliza esa base de biblioteca de placa, aunque solo se necesite la
  parte de timer para el Reversi). **No se ha encontrado ningún documento que pida
  completar o modificar estos tres ficheros para el Reversi.**
- `common/` es coherente con la ruta `./common/44binit.asm` y `./common/ld_script.ld` de la
  tabla "Fuentes Placa".
- `emulador/` no aparece como nombre de carpeta en ningún documento (los ficheros de
  emulador citados están en la raíz del repo: `init_b_2026.asm`, `ld_script.ld`), pero es
  razonable que el propio repositorio del usuario los haya organizado en una carpeta con ese
  nombre.

---

## 8. Posible bug en el código C proporcionado (versión beta)

El propio guión advierte: *"como versión beta está sujeta a fallos, avisad si veis alguno"*. Se ha
detectado lo siguiente al leer `reversi8_2026.c` línea a línea:

- La firma de `color_ficha` es:
  `char color_ficha(char tablero[][DIM], char f, char c, int *es_posicion_correcta)`
  (tablero primero, luego fila, columna).
- Dentro de `patron_volteo`, la **primera** llamada respeta ese orden:
  `casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);`
- Pero dentro del **bucle** `while` de la misma función, la llamada usa **el orden de argumentos
  cambiado**:
  `casilla = color_ficha(FA, CA, tablero, &es_posicion_correcta);`

Esto es sospechoso de ser un error de trascripción en el fuente (los tipos `char`/`char[][DIM]`
no coincidirían con lo que espera la función, lo cual con el prototipo visible daría un warning
de tipos al compilar, no necesariamente un error duro). **No se ha corregido en este
documento ni se recomienda "arreglarlo" sin más**: siguiendo la instrucción del propio guión,
lo correcto es señalarlo y preguntar (a la profesora/profesor, o dejarlo anotado en la memoria
como incidencia encontrada, según el punto 6 del Anexo III "Descripción de los problemas
encontrados"). Si al traducir a ensamblador se reproduce fielmente el comportamiento del C
tal cual compila (incluyendo este posible bug), hay que documentarlo explícitamente como tal
en la memoria, ya que "mantener el algoritmo" es un requisito explícito de la práctica.

---

## 9. Dudas / huecos (nada de esto se ha supuesto; hace falta confirmarlo con el profesor,
compilando el código, o mirando ficheros no incluidos en este análisis)

1. **`volatile` en `fila`/`columna`/`ready`**: el propio código deja como pregunta retórica ("¿hay
   que hacer algo con ellas para que esto funcione bien?") si hace falta añadir `volatile`. No
   hay una respuesta explícita en el guión ni en las diapositivas sobre si aplica a las tres
   variables o solo a `ready`. Es una pregunta abierta del enunciado, no un hueco de este
   análisis, pero se deja constancia para que la sesión de código no la responda por su cuenta
   sin más contexto.
2. **Rol real de `led.c`, `8led.c`, `button.c`, `timer.c`** en el repositorio del usuario: no hay
   ningún documento de los revisados que los mencione como parte del entregable de esta
   P1. La hipótesis más plausible (ficheros heredados del "proyecto ejemplo" o de la plantilla
   de E/S de la UCM) está descrita en §7 y §5.2, pero no está confirmada — habría que
   revisar el contenido real de esos ficheros y/o preguntar al profesor si son necesarios.
3. **Contenido exacto del "proyecto ejemplo del timer0"** (Paso 6): el guión y las diapositivas
   lo mencionan y explican su propósito, pero no se ha localizado en la documentación
   analizada un listado de sus ficheros ni su código fuente completo. Si existe en el
   repositorio del usuario (posiblemente relacionado con `timer.c`), debería revisarse
   directamente en el código, no inferirse de este documento.
4. **Direcciones exactas de TCNTBn/TCMPBn/TCNTOn para n=2** (timer2): la tabla completa
   de direcciones está en `Entradasalida.pdf` cap. 5 (tabla 5.1) — se ha resumido el patrón
   general en §3.2 pero no se han copiado aquí las 18 direcciones exactas de todos los
   timers; para programar `timer2_inicializar()` hay que consultar esa tabla completa o el
   fichero `44b.h` del proyecto (si lo tiene) para los nombres simbólicos de registros.
5. **Fechas definitivas de entrega**: el guión da fechas aproximadas ("en torno al 6 de
   octubre" y "en torno al 9 de octubre") y remite a Moodle para las fechas definitivas, que no
   están en ningún documento analizado.
6. **Convenio de direcciones vSF/vSC → nombre de dirección**: el comentario en el código
   sugiere "0 es Norte, 1 NE, 2 E..." pero no hay una tabla explícita y verificada en la
   documentación que confirme la correspondencia completa índice↔dirección cardinal más
   allá de ese comentario suelto; no se ha tomado como верdad absoluta más allá de citar
   literalmente el comentario del propio código.
7. **`GuiaEntorno.pdf`** no se ha volcado en detalle en este documento (son pasos de uso de
   Eclipse: creación de proyecto, configuración de PATH, depuración sobre placa/simulador),
   porque no aporta información de diseño o hardware relevante para una sesión de código;
   si la sesión de Claude Code necesita instrucciones de compilación/depuración por línea de
   comandos en lugar de Eclipse, ese documento no las cubre (está orientado a la GUI).

---

## 10. Ficheros de proyecto disponibles (para referencia rápida, no reproducidos aquí)

Disponibles en el entorno de esta sesión de análisis, por si se quieren consultar directamente
en vez de fiarse de los resúmenes de este documento:

- Guion y evaluación: `PH_practica1_2026.pdf`, `PH_pract1_2026_slides.pdf`, `aclaracion_fuentes.pdf`.
- Entorno de desarrollo: `GuiaEntorno.pdf`.
- Conceptos ARM/C/ensamblador (UCM, aplican): `P1-EC.pdf`, `P2-EC.pdf`.
- Hardware de la placa (UCM, aplica solo memoria+timers): `Entradasalida.pdf`.
- Serie paralela UCM (referencia, no aplica directamente): `P1-LEC.pdf` … `P5-LEC.pdf`.
- Referencias de arquitectura: `ARM.pdf`, `ARMinstSet-ReferenciaRapida-QRC0001H_rvct_v2_1.pdf`,
  `thumb_reference_card-.pdf`, `IHI0042F_aapcs.pdf` (especificación AAPCS oficial de ARM).
- Apoyo C: `C__tipos_variables_y_ubicación_memoria.pdf`.
- Código fuente ya revisado en este documento: `reversi8_2026.c`, `init_b_2026.asm`, `ld_script.ld`.
