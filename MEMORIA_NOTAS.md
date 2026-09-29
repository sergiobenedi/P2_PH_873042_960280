# Notas para la memoria — Práctica 1 (Reversi 8, ARM7TDMI)

Autores: Sergio Benedi (NIP 873042), Pablo Gimeno (NIP 960280)
Entrega de la memoria: hacia el 9 de octubre de 2026. Corrección presencial: hacia el 6 de octubre.
Entrega en ZIP: `p1_873042-Apellidos_960280-Apellidos.zip` (memoria en PDF + fuentes).

> Este fichero NO es la memoria. Es el cuaderno donde vamos guardando lo hecho en cada sesión.
> Cada bloque está etiquetado con el apartado de la memoria en el que se usará:
> `[RES]` Resumen · `[INT]` Introducción · `[OBJ]` Objetivos · `[MET]` Metodología ·
> `[PILA]` Marco de pila · `[OPT]` Optimizaciones · `[RESUL]` Resultados ·
> `[PROB]` Problemas y soluciones · `[CONC]` Conclusiones.

---

## 0. Requisitos de la memoria (guía del profesor + Anexo III del guión)

Estructura (guía "Cómo redactar una memoria técnica"):
1. **Resumen**: autocontenido, máximo 1 cara. Qué, cómo y por qué, y las conclusiones en esbozo.
2. **Introducción**: contexto del trabajo.
3. **Objetivos**.
4. **Metodología**: pasos seguidos y decisiones de diseño. En Proyecto Hardware debe incluir:
   esquema del proyecto (ficheros y funciones), **número total de líneas de código**,
   **horas de dedicación** y el código desarrollado, comentado.
5. **Resultados**: presentarlos, explicarlos **y analizarlos**.
6. **Conclusiones**: puntos fuertes, puntos débiles y posibles mejoras.
7. Referencias, bibliografía y anexos.

Contenido obligatorio del Anexo III del guión:
- Resumen ejecutivo.
- Explicación de cómo se ha creado el marco de pila.
- Código fuente comentado. Cada función lleva una cabecera con su funcionamiento, sus parámetros
  (qué recibe y dónde) y el uso de cada registro.
- Optimizaciones realizadas.
- Comparación de resultados entre versiones.
- Problemas encontrados y sus soluciones.
- Conclusiones, firma y autoría.

Maquetación:
- Portada con título, asignatura, práctica, fecha, nombres con los dos apellidos y NIP.
- Letra de 11-12 pt.
- Secciones con título y numeradas.
- Páginas numeradas.
- Índice.
- Tablas y figuras numeradas y referenciadas en el texto.
- Pasar el corrector y releer.

---

## 1. Registro de horas `[MET]`

| Fecha | Quién | Horas | Qué |
|-------|-------|-------|-----|
Trabajo siempre en pareja (los dos a la vez), en 3 sesiones de 2 h los lunes.

| Fecha | Quién | Horas | Qué |
|-------|-------|-------|-----|
| 2026-09-14 (lunes) | Sergio y Pablo | 2 | Pasos 1 y 2: montar el proyecto en Eclipse, depurar el código C, añadir el emulador y documentar (README, PRACTICA.md) |
| 2026-09-21 (lunes) | Sergio y Pablo | 2 | Paso 3: color_ficha_arm, patron_volteo_arm, #define y prueba en el simulador |
| 2026-09-28 (lunes) | Sergio y Pablo | 2 | Pruebas del Paso 3 en placa; Pasos 4 y 5: color_ficha_thumb y las 6 combinaciones en placa |

Total: 6 h por persona (12 h-persona entre los dos).

---

## 2. Diario de sesiones

### 2026-09-15 / 16 — Preparación (Pasos 1 y 2) `[MET]`
- Proyecto creado en Eclipse con ARM Sourcery GCC 2011.03 y openOCD. Se añaden rutas relativas.
- `main()` llama a `sys_init()` y después a `reversi8()`.
- Las funciones que no se llaman desde `main()` se declaran `static`.
- Se añade la carpeta `emulador/`, con `init_b_2026.asm` (sp = 0x4000, salta a `reversi8`) y su propio `ld_script.ld`.
- **[PROB]** El guión avisa de que el C es una versión beta. En el fuente original, la llamada del
  `while` de `patron_volteo` tenía los argumentos en otro orden: `color_ficha(FA, CA, tablero, …)`.
  En el código actual ya aparece como `color_ficha(tablero, FA, CA, …)`.
  _Confirmar si lo corregimos nosotros en el commit "Código depurado" y comentarlo al profesor._

### 2026-09-21 — Paso 3: funciones en ensamblador ARM `[MET][OPT][PILA]`
- Nuevo fichero `reversi8_arm.asm` con `color_ficha_arm` y `patron_volteo_arm`.
- Encima de cada función va comentado el código C original. Así se sigue la estructura
  C → ARM, que es lo que pide el guión.
- `color_ficha` deja de ser `static` porque `patron_volteo_arm` la llama desde el `.asm`.
  El guión permite que `patron_volteo_arm` llame a la versión C, a la ARM o a la Thumb.
- En `reversi8_2026.c`, el `#define PATRON_VOLTEO patron_volteo | patron_volteo_arm` elige qué
  versión usa el juego. Se usa en `actualizar_tablero` y en `elegir_mov`.
- La versión de `color_ficha` se elige con un `#define COLOR_FICHA color_ficha | color_ficha_arm`:
  - en `reversi8_2026.c`, para la que llama `patron_volteo` (C);
  - en `reversi8_arm.asm`, para la que llama `patron_volteo_arm`. Funciona porque Eclipse
    ensambla con `-x assembler-with-cpp`.
- 2026-09-28: se borran las cabeceras duplicadas con comentarios `#` del `.asm`. Estaban
  incompletas (faltaba la línea del `if`) y llevaban un `# else` suelto, que es el mismo
  riesgo del problema 1.

#### Decisiones de diseño `[MET][OPT]`
- **`char` sin signo en arm-none-eabi-gcc.**
  - Un índice negativo (0 + (−1)) queda como 0xFF.
  - Por eso **una sola comparación sin signo** (`cmp r1,#DIM` + `bhs`) cubre `f < DIM` y `f >= 0` a la vez.
  - En `color_ficha_arm` basta con 2 comparaciones en lugar de 4.
  - Tras cada suma de `FA`/`CA` se trunca con `and #0xFF`, para comportarse igual que el C.
- **Acceso a `tablero[f][c]`**: `add r0, r0, r1, lsl #3` + `ldrb r0, [r0, r2]`.
  - La multiplicación por 8 se hace con un desplazamiento integrado en el `add`.
  - Se usa direccionamiento base + registro.
- **Registros callee-saved r4-r10** en `patron_volteo_arm`, para que los valores sobrevivan a la llamada a `color_ficha`:
  - r4 = tablero
  - r5 = longitud
  - r6 = FA
  - r7 = CA
  - r8 = SF
  - r9 = SC
  - r10 = color

  Así no hay que recargar nada de pila dentro del bucle.
- **`es_posicion_correcta` en pila** (`[sp]` = `[fp,#-44]`). Tiene que estar en memoria porque se pasa su dirección a `color_ficha`.
- **Ejecución condicional** al final:
  - `cmp r1,#1` / `cmpeq r0,r10` / `moveq r0,#1` / `movne r0,#0`.
  - Evalúa las dos condiciones del `if` y el `return` sin ningún salto.
- **Parámetros 5.º a 7.º** (`SF`, `SC`, `color`): llegan en la pila del llamador, en `[fp,#4]`, `[fp,#8]` y `[fp,#12]`. Se leen con `ldrb`.
- **Modularidad** (norma de suspenso):
  - Se mantiene la llamada real a `color_ficha`, con los parámetros en r0-r3 según AAPCS.
  - No se accede a variables locales de otra función.
  - No se cambia el algoritmo.
- **Sin `blx`**: ARMv4T (ARM7TDMI) no lo tiene. Para llamar a Thumb (Paso 4) se usará
  `ldr ip,=func+1` / `mov lr,pc` / `bx ip`.

#### Marco de pila `[PILA]`
Es igual que el que genera el compilador con `-mapcs-frame`:
```
mov   ip, sp
stmdb sp!, {<regs>, fp, ip, lr, pc}
sub   fp, ip, #4
...
ldmdb fp, {<regs>, fp, sp, pc}
```
`color_ficha_arm` (hoja con respecto a la pila, no guarda registros):
```
[fp]      pc
[fp,#-4]  lr
[fp,#-8]  sp anterior (ip)
[fp,#-12] fp anterior
```
`patron_volteo_arm`:
```
[fp,#12]  color   (7.º parámetro, pila del llamador)
[fp,#8]   SC      (6.º)
[fp,#4]   SF      (5.º)
[fp]      pc
[fp,#-4]  lr
[fp,#-8]  sp anterior
[fp,#-12] fp anterior
[fp,#-16] r10 ... [fp,#-40] r4
[fp,#-44] es_posicion_correcta  <- sp
```
Marco que genera el compilador (`-O0 -mapcs-frame`) para `patron_volteo` en C:
```
mov   ip, sp
push  {fp, ip, lr, pc}
sub   fp, ip, #4
sub   sp, sp, #24          ; todos los parámetros y locales van a pila
str   r0, [fp,#-24]        ; tablero
str   r1, [fp,#-28]        ; longitud
strb  r2, [fp,#-29]        ; FA
strb  r3, [fp,#-30]        ; CA
...                        ; SF/SC en [fp,#4]/[fp,#8]; es_posicion_correcta en [fp,#-20]
```
- **La estructura es la misma**: `mov ip,sp` / `stmdb {…,fp,ip,lr,pc}` / `sub fp,ip,#4`, y los parámetros 5.º a 7.º en `[fp,#4…]`.
- **Diferencia**: el compilador a `-O0` guarda todo en pila y recarga en cada uso.
  Nosotros guardamos r4-r10 y dejamos en pila solo `es_posicion_correcta`, que es lo que
  permite el comentario del C ("si se colocan en un registro no hace falta… reservar espacio").

#### Problemas y soluciones `[PROB]`
1. **El preprocesador rompía el `.asm`.** Eclipse ensambla con `gcc -x assembler-with-cpp`,
   así que las líneas con `#` en la columna 0 se interpretaban como directivas del preprocesador.
   - Errores: `token "[" is not valid in preprocessor expressions` y `unterminated #else`.
   - Solución: todos los comentarios pasan a ser `/* … */`.
2. **Suposiciones erróneas del esqueleto inicial.** Proponía usar `blx`, que no existe en ARMv4T,
   y extender el signo de los `char`, que en realidad son sin signo. Se corrigieron las dos cosas.
3. **Cómo meter jugadas en el simulador.**
   - Breakpoint en `esperar_mov(&ready)`, en la línea 451 de `reversi8_2026.c`.
   - Escribir `fila`, `columna` y por último `ready = 1`, desde la vista Expressions o el monitor de memoria. Después, F8.
   - En nuestro build: `&tablero` = 0x0C0027D0 y `&fila` = 0x0C002810 (`columna` y `ready` en los bytes siguientes).
   - Confusión inicial: las **negras son `02`** y las **blancas `01`**.
4. **En la placa parecía que el programa se colgaba** (2026-09-28).
   - "Target not available" en Expressions y la memoria en `00` rojo solo indican que la placa está ejecutando: GDB no puede leer.
   - El programa no se había colgado. Estaba en `esperar_mov` esperando la siguiente jugada, y el tablero ya estaba bien. Se ve pulsando Suspend.
   - En la placa las direcciones cambian (otro linker script): `&tablero` = 0x0C0027B0, `&fila` = 0x0C0027F0. Conviene usar `&tablero` en el monitor en lugar de direcciones fijas.
   - Dentro de `esperar_mov`, `ready` en Expressions es el parámetro `char *` y no la variable global.
5. **El orden de las variables globales cambia con el nivel de optimización** (2026-09-29, Paso 9).
   - Síntoma: a `-O1` escribimos 2, 3 y 1 en tres bytes seguidos de la vista Memory, con el orden de
     `-O0` (`fila`, `columna`, `ready`). El programa jugó (3,1), una jugada inválida: no volteó nada
     y el tiempo medido (116 µs) no servía.
   - Qué pasaba: a `-O1` el orden es otro. Los valores fueron a la variable de al lado (2 → `ready`,
     3 → `fila`, 1 → `columna`). La información de depuración del `.elf` era correcta: `&fila` en
     Expressions daba 0x0C001861.

     | Opciones | Orden en `.bss` |
     |----------|-----------------|
     | `-O0` | `fila`, `columna`, `ready` |
     | `-O1`, `-O2`, `-O3` | `ready` (0x0C001860), `fila` (0x..861), `columna` (0x..862) |
     | `-O1 -fno-section-anchors` | `fila`, `columna`, `ready` |
     | `-O1 -fno-toplevel-reorder` | `fila`, `columna`, `ready` |

   - Causa: **`-fsection-anchors`**, que GCC activa en ARM a partir de `-O1`. Agrupa las variables
     globales del fichero en un bloque y accede a todas con una única dirección base y un
     desplazamiento (`ldrb r3,[r2,#64]` para `ready`, `ldrb r3,[r2,#65]` para `fila`). Ahorra cargar
     la dirección de cada variable, pero el compilador decide su orden y deja de ser el del código.
   - Solución: escribir las variables por nombre desde Expressions (clic en la celda Value o
     botón derecho → Change Value), con `ready` siempre la última. No suponer nunca la posición
     relativa de variables distintas.
6. **A partir de `-O1` el breakpoint de `esperar_mov(&ready)` (línea 460) no para antes de la espera**
   (2026-09-29, Paso 9).
   - Síntoma: a `-O0` bastaba con F8 para llegar a la espera. A `-O1` y `-O3` había que pulsar F8 y
     luego Suspend para poder meter la jugada.
   - Causa: `esperar_mov` se **integra (inlining)** en `reversi8` y la línea 460 se queda sin
     instrucciones propias; ningún nivel optimizado le asocia código (comprobado con `objdump -dl`).
     Eclipse mueve el breakpoint a la siguiente línea con código, la 462, que queda **después** del
     bucle de espera. El programa entra en el bucle sin parar y solo se detiene al poner `ready = 1`.
   - Solución: breakpoint en la línea 469 (después de `actualizar_candidatas`), que tiene código en
     todos los niveles. F8 + Suspend para meter la jugada; al darle a F8 para justo después de la
     medición y se lee `tiempo_volteo_us`.

### 2026-09-28 — Paso 4: color_ficha_thumb `[MET][OPT][PILA]`
- Nuevo fichero `reversi8_thumb.asm` con `color_ficha_thumb` (`.thumb` + `.thumb_func`).
- Misma estructura que `color_ficha_arm`, con dos cambios obligados por Thumb:
  - no hay desplazamiento integrado en `add`: `lsl r1,r1,#3` + `add r0,r0,r1`;
  - no hay ejecución condicional ni `ldmdb`: se usan saltos y `bx lr`.
- **Marco de pila: ninguno** (el guión permite reducirlo en Thumb). Es hoja, usa solo r0-r3 y no
  llama a nadie, así que ni `lr` ni ningún registro callee-saved se modifican.
- **Retorno con `bx lr`**: el llamador es ARM (bit 0 de `lr` = 0), así que vuelve a estado ARM.
  `pop {pc}` no valdría: en ARMv4T no cambia de estado.
- **Llamada ARM/C → Thumb**: se deja el `bl` normal (en C y en `patron_volteo_arm`). Como el
  símbolo es `.thumb_func` y ARMv4T no tiene `blx`, el enlazador inserta un **veneer** que carga
  la dirección con bit 0 = 1 y hace `bx`. Así el cambio de versión sigue siendo solo el `#define`.
  Comprobado en `Placa/P1_873042_960280.lst` (2026-09-28): los 4 `bl` (2 de `patron_volteo`,
  2 de `patron_volteo_arm`) van a `__color_ficha_thumb_from_arm` (0x0c002710):
  ```
  ldr  ip, [pc]
  bx   ip
  .word 0x0c00249d     ; color_ficha_thumb (0x0c00249c) | 1 -> estado Thumb
  ```
  El veneer va al final de `.text`, no en `.glue_7` (que aparece con tamaño 0 en el `.map`).
  Coste: 12 bytes (+4 de relleno) y **2 instrucciones ARM extra por llamada** (contarlas en el Paso 8).
- **Tamaños (`.map`)**: `color_ficha_thumb` = 0x1e = 30 bytes (15 instr. de 2 bytes);
  `color_ficha_arm` = 0x48 = 72 bytes.
- En el camino `else` se reutiliza el 0 de `r0` para escribir `*es_posicion_correcta = 0`.
- **Cambio de estado comprobado en placa** (2026-09-28, C-Thumb, jugada (2,3), breakpoint en
  la línea 216 de `reversi8_2026.c` y Step Into en modo instrucción):

  | Momento | `cpsr` | Bit T | Estado |
  |---------|--------|-------|--------|
  | Antes del `bl` (0x0c00136c) | 0x80000013 | 0 | ARM, Supervisor |
  | Tras el `bx ip` del veneer (0x0c00249c) | 0x80000033 | 1 | Thumb |
  | Tras el `bx lr` (0x0c001374) | 0x60000013 | 0 | ARM (r0 = 0: casilla vacía/fuera) |

  Las banderas cambian (N → Z,C) por las comparaciones de `color_ficha_thumb`; no importa porque
  AAPCS no obliga a conservarlas.

### 2026-09-28 — Paso 7: librería timer2 (sin placa) `[MET][PROB]`
- Nuevos `timer2.c` / `timer2.h` con `timer2_inicializar`, `timer2_empezar`, `timer2_leer` y `timer2_parar`.
- **Máxima precisión**: preescalado 0 (bits 8-15 de `TCFG0`) y divisor 1/2 (bits 8-11 de `TCFG1`).
  F = 64 MHz / (1·2) = **32 MHz → 1 tick = 1/32 µs**.
- `TCNTB2 = 65535` con auto-reload: cada vuelta son 65536 ticks = **2048 µs** y genera una IRQ.
  La ISR (`__attribute__((interrupt("IRQ")))`) incrementa `timer2_num_int` y borra `BIT_TIMER2` en `I_ISPC`.
- `timer2_leer` = `num_int·2048 + (65535 − TCNTO2)/32` µs. Calculado así no desborda hasta 2³² µs (~71 min).
  Multiplicar primero por 65536 ticks desbordaría a los ~134 s.
- **Condición de carrera**: si la IRQ salta entre leer `timer2_num_int` y `TCNTO2`, los dos valores no
  casan. Se repite la lectura mientras `timer2_num_int` cambie.
- Solo se tocan los bits del timer2 en `TCFG0`, `TCFG1` y `TCON` (lectura-modificación-escritura) para no
  romper el timer0. `sys_init()` ya deja `INTMOD`/`INTCON` y `BIT_GLOBAL`; basta con desenmascarar `BIT_TIMER2`.
- **Calibración** en `main.c` (`#define CALIBRAR_TIMER2 1`): mide `Delay(10/100/10000/100000)`
  (1 ms, 10 ms, 1 s, 10 s) y lo deja en `tiempos_calibracion[4]`. Resultado en placa (2026-09-29):

  | Delay | Esperado (µs) | timer2 (µs) | Desviación |
  |-------|---------------|-------------|------------|
  | 1 ms  | 1000     | 1023     | +2,3 % |
  | 10 ms | 10000    | 10169    | +1,7 % |
  | 1 s   | 1000000  | 1016745  | +1,7 % |
  | 10 s  | 10000000 | 10167390 | +1,7 % |

  Desviación constante del ~1,7 %: la produce `Delay()`, no el timer2. `Delay(0)` calibra
  `delayLoopCount` con el watchdog y la división entera redondea (8000000/(i·64)). En 1 ms pesa
  además el coste fijo de llamar a `timer2_empezar`/`parar` y a `Delay`.
  **[PROB]** Al primer intento solo `[0]` tenía un valor válido: seguía activo el breakpoint de la ISR
  y el programa se paraba en la primera IRQ, antes de escribir `[1]`.
- **Medición del Paso 8**: `timer2_empezar()`/`timer2_parar()` alrededor de `actualizar_tablero` de la
  jugada humana en `reversi8()`; el resultado queda en `tiempo_volteo_us`. Incluye las 8 llamadas a
  `patron_volteo` y el `voltear`, que es igual en todas las combinaciones.

#### Problema: `ready` sin `volatile` `[PROB][OPT]`
- `fila`, `columna` y `ready` se escriben desde fuera (depurador) pero el compilador no lo sabe.
- A `-O0` funciona por casualidad (siempre recarga de memoria). **A `-O2` el bucle de `esperar_mov`
  se compila como `b .` (salto a sí mismo en 0x7ac)**: lee `ready` una vez y, si vale 0, se cuelga.
- Solución: `volatile char fila, columna, ready` y `esperar_mov(volatile char *ready)`. Comprobado
  con `objdump`: ningún bucle a sí mismo en `-O0/-O1/-O2/-O3/-Os`. Imprescindible para el Paso 9.
- Responde a la pregunta que deja el propio código ("¿hay que hacer algo con ellas…?").

### 2026-09-29 — Paso 6: rutina de interrupción en ensamblador `[MET][PILA]`
- Comprobado en placa: breakpoint en `timer2_ISR` (timer2.c:36); la pila de llamadas muestra
  `timer2_ISR` ← `Delay` (el timer2 interrumpe el bucle de `Delay` durante la calibración).
- Código que genera GCC para `__attribute__((interrupt("IRQ")))` (`-O0 -mapcs-frame`):
  ```
  push  {ip}                         ; r12 aparte: el marco APCS lo va a machacar
  mov   ip, sp
  push  {r2, r3, fp, ip, lr, pc}     ; marco + solo los registros que usa la ISR
  sub   fp, ip, #4
  ...                                ; timer2_num_int++ ; rI_ISPC = BIT_TIMER2
  sub   sp, fp, #20
  ldm   sp, {r2, r3, fp, sp, lr}
  pop   {ip}
  subs  pc, lr, #4                   ; vuelve a la instrucción interrumpida y CPSR <- SPSR_irq
  ```
- Diferencias con una función normal:
  - Guarda también registros *caller-saved* (r2, r3, ip): la IRQ llega en cualquier instrucción y
    el código interrumpido no los ha salvado. Solo guarda los que usa.
  - `lr_irq` = instrucción interrumpida + 4 → se retorna con `lr − 4`. GCC lo hace al salir, no al entrar.
  - `subs pc, …` (la `s` con destino pc) copia SPSR en CPSR: vuelve a modo SVC y reactiva las IRQ.
- Camino: vector IRQ 0x18 → controlador vectorizado → `HandlerTIMER2` (44binit.asm) → carga
  `pISR_TIMER2` (0x0c7fff4c) → `timer2_ISR`.
- **[PROB]** `I_ISPC` es de solo escritura: `rI_ISPC |= BIT_TIMER2` leía basura y podía borrar otras
  pendientes. Se cambia por `rI_ISPC = BIT_TIMER2`.

---

## 3. Resultados `[RESUL]`

### 3.1 Pruebas funcionales (Paso 3 / Paso 5)
Jugada de referencia: ficha negra en (2,3) → `fila=2`, `columna=3`, `ready=1`.

Tablero esperado tras la jugada humana y la respuesta de la máquina (blanca en (2,2)):
```
fila 2: 00 00 01 02 00 00 00 00
fila 3: 00 00 00 01 02 00 00 00
fila 4: 00 00 00 02 01 00 00 00
```
Qué ocurre: la negra en (2,3) voltea (3,3). La blanca en (2,2) vuelve a voltear (3,3) por la diagonal.

Cómo se elige la combinación (por defecto C + C):
- `PATRON_VOLTEO` en `reversi8_2026.c`: `patron_volteo` o `patron_volteo_arm`.
- `COLOR_FICHA` en `reversi8_2026.c`: qué versión llama `patron_volteo` (C).
- `COLOR_FICHA` en `reversi8_arm.asm`: qué versión llama `patron_volteo_arm`.
- Después de cada cambio, Clean + Build.

| patron_volteo | color_ficha | Jugada (2,3) | Estado |
|---------------|-------------|--------------|--------|
| C   | C     | tablero correcto | ✅ probado 2026-09-21 |
| ARM | C     | tablero correcto | ✅ simulador y placa 2026-09-28 |
| ARM | ARM   | tablero correcto | ✅ 2026-09-28 |
| C   | Thumb | tablero correcto | ✅ placa 2026-09-28 |
| ARM | Thumb | tablero correcto | ✅ placa 2026-09-28 |
| C   | ARM   | tablero correcto | ✅ 2026-09-28 |

### 3.2 Rendimiento (Pasos 8 y 9) — tamaños hechos, faltan instrucciones y tiempos
Tamaños por función (bytes, `-O0 -mapcs-frame`; C del `.elf` de Placa con `nm -S`, asm con `size`):
`patron_volteo` = 260 · `color_ficha` = 196 · `patron_volteo_arm` = 168 · `color_ficha_arm` = 72 ·
`color_ficha_thumb` = 30 · veneer ARM→Thumb = 12 (+4 de relleno).

| patron_volteo | color_ficha | Tamaño (bytes) | Instrucciones | Tiempo (µs) |
|---------------|-------------|----------------|---------------|-------------|
| C   | C     | 456 | | 284 (284, 284, 285) |
| C   | ARM   | 332 | | 228 (229, 228, 228) |
| C   | Thumb | 290 (+12 veneer) | | 222 (222, 222, 222) |
| ARM | C     | 364 | | 258 (258, 259, 258) |
| ARM | ARM   | 240 | | 200 (200, 201, 200) |
| ARM | Thumb | 198 (+12 veneer) | | 194 (194, 194, 193) |

Tiempos medidos en placa el 2026-09-29 con `tiempo_volteo_us` (`actualizar_tablero` completo de la jugada
negra (2,3), `-O0`, 3 repeticiones; la variación entre repeticiones es de ±1 µs).

Análisis de tiempos:
- Respecto a C-C (284 µs): C-ARM −20 %, C-Thumb −22 %, ARM-C −9 %, ARM-ARM −30 %, ARM-Thumb **−32 %**.
- Cambiar `color_ficha` rinde más que cambiar `patron_volteo` (C-ARM 228 frente a ARM-C 258):
  `color_ficha` se llama en cada paso del bucle y la versión C a `-O0` guarda y recarga todo de pila.
- **Thumb es más rápido que ARM** (222 < 228 y 194 < 200) aunque cada llamada pasa por el veneer
  (2 instrucciones más). Explicación probable: la memoria de la placa va con **bus de datos de 16 bits**
  (`Port_Init`, "16bit data bus configuration") y la caché no se activa (nadie escribe `rSYSCFG`;
  `CACHECFG` de option.h no se usa). Cada instrucción ARM de 32 bits necesita 2 accesos a memoria y
  una Thumb de 16 bits solo 1, así que se ahorra más en la búsqueda de instrucciones de lo que
  cuesta el veneer. _Comentarlo con el profesor._
- ARM-Thumb es la combinación más rápida y también la más pequeña (198 bytes + 12 del veneer).

C-C por nivel de optimización (bytes de `.text` de `reversi8_2026.o`, ya con `volatile`):

| Nivel | patron_volteo | color_ficha | .text del fichero | Instrucciones | Tiempo |
|-------|---------------|-------------|-------------------|---------------|--------|
| -O0 | 260 | 196 | 5208 | | |
| -O1 | 212 | 56 | 1524 | | |
| -O2 | 220 (`.clone`) | 56 | 2068 | | |
| -O3 | — (integrada en `actualizar_tablero`, 596) | 56 | 1856 | | |
| -Os | 164 (`.clone`) | 52 | 1368 | | |

Observaciones:
- A partir de `-O1` desaparecen `voltear`, `contar`, `elegir_mov`, `init_table` y `esperar_mov`:
  el compilador las integra en quien las llama (inlining).
- `-O2` y `-O3` generan clones especializados (`.clone`). A `-O3`, `patron_volteo` ni siquiera existe
  como función, así que no se puede medir por separado.
- `-O3` ocupa menos que `-O2` en total porque integra más código en `actualizar_tablero` y `reversi8`
  se reduce. `-Os` es el más pequeño.
- `color_ficha` pasa de 196 a 56 bytes solo con `-O1`: deja de guardar parámetros en pila.

---

## 4. Datos del proyecto `[MET]`
| Fichero | Funciones | Líneas |
|---------|-----------|--------|
| reversi8_arm.asm | color_ficha_arm, patron_volteo_arm | 184 |
| reversi8_thumb.asm | color_ficha_thumb | 79 |
| reversi8_2026.c | juego (proporcionado, modificado) | 476 |

Actualizar con `wc -l` antes de entregar.

---

## 5. Ideas para las conclusiones `[CONC]`
- El `char` sin signo simplifica las comprobaciones de rango en ensamblador.
- Mantener las variables en registros callee-saved elimina los accesos a memoria dentro del bucle.

---

## 6. Nota de diagnóstico y defensa sobre `volatile` / optimización `[PROB][OPT][CONC]`

### Resumen del problema
- La práctica usa variables externas a la CPU (`fila`, `columna` y `ready`) que no son modificadas por el
  propio flujo del programa, sino por el depurador o por la entrada del usuario.
- El compilador no “sabe” que estas variables cambian fuera del código normal, por lo que puede
  mantener sus valores en registros o reordenar accesos cuando se activa la optimización.
- A `-O0` funciona por casualidad porque suele recargar la variable desde memoria cada vez que la usa;
  a `-O1/O2/O3` el compilador puede eliminar o reusar lecturas y, si no se marca la variable como
  `volatile`, el bucle de espera puede quedar optimizado a un salto a sí mismo o a una lectura estática.

### Qué hicimos y por qué funcionó
- Se demostró que la solución correcta era:
  ```c
  volatile char fila, columna, ready;
  static void esperar_mov(volatile char *ready) {
      while (*ready == 0) {}
      *ready = 0;
  }
  ```
- La palabra clave `volatile` fuerza al compilador a leer y escribir el valor desde memoria en cada acceso,
  evitando la optimización errónea de una variable de sincronización con el depurador.
- Se comprobó con desensamblado (`objdump`) y depuración paso a paso que el programa estaba correcto y
  que el fallo observadose debía a la semántica del compilador, no a una lógica incorrecta del Reversi.

### Cómo explicarlo en la defensa
- El problema no es que el compilador “haga trampas”; es que aplica reglas de optimización sobre variables
  que, en este caso, pueden cambiar fuera del flujo normal del programa.
- La solución no es cambiar la lógica del juego, sino marcar esas variables como `volatile` y documentar
  la interacción con el depurador y con el monitor de memoria.
- La diferencia entre simulador y placa debe explicarse como un efecto de disponibilidad del depurador y del
  acceso real a memoria, no como un fallo del algoritmo.
- La evidencia importante es que el programa queda esperando en `esperar_mov` y no “se rompe”, sino que
  está bloqueado esperando la siguiente jugada válida.

### Comando útil para resetear la rama develop
```bash
git fetch origin
git reset --hard origin/develop
```

Este comando deja la rama en el estado exacto del remoto y descarta los cambios locales.

---
