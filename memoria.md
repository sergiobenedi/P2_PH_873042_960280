# Práctica 1: Desarrollo de código para el procesador ARM. Reversi de 8 casillas

**Asignatura:** Proyecto Hardware 1 (5.º semestre, 3.er curso, Grado en Ingeniería Informática)
**Universidad:** Universidad de Zaragoza, Departamento de Informática e Ingeniería de Sistemas
**Práctica:** Práctica 1, curso 2026/2027
**Fecha:** 15 de octubre de 2026

**Autores:**

- Sergio Benedi [SEGUNDO APELLIDO], NIP 873042
- Pablo Gimeno [SEGUNDO APELLIDO], NIP 960280

---

## Índice

[Insertar aquí el índice automático: Insertar → Índice en Google Docs]

---

## 1. Resumen

Se ha implementado y evaluado el núcleo de cálculo del juego Reversi de 8 casillas sobre la placa Embest S3CEV40 (SoC Samsung S3C44B0X, núcleo ARM7TDMI a 64 MHz). Partiendo del código C suministrado, se han reescrito en ensamblador las dos funciones críticas, `color_ficha` (versiones ARM y Thumb) y `patron_volteo` (versión ARM). Las versiones ensambladas mantienen la estructura del C, la llamada real a `color_ficha` con parámetros según AAPCS y el marco de pila que genera el compilador. Se ha desarrollado una librería de medición de tiempos sobre el timer2 (resolución de 1/32 µs, basada en interrupciones IRQ) y se ha calibrado contra `Delay()`.

Con la librería se han medido en placa las 6 combinaciones C/ARM/Thumb para la jugada de referencia (ficha negra en fila 2, columna 3) y los niveles de optimización del compilador `-O0`, `-O1`, `-O2`, `-O3` y `-Os` para la combinación C-C. Todas las combinaciones producen el mismo tablero.

Resultados principales: la mejor combinación en ensamblador (patron_volteo ARM + color_ficha Thumb) reduce el tiempo un 32 % (284 µs → 194 µs) y es también la más pequeña (198 bytes frente a 456). El compilador a `-O3` obtiene 65 µs (−77 %), mejor que cualquier versión en ensamblador. Esta comparación no es equivalente: en las versiones ensambladas el resto del juego sigue compilado a `-O0`. Conclusión: la reescritura manual mejora claramente el código sin optimizar, pero un compilador con optimización alta la supera con mucho menos esfuerzo y el tiempo sigue casi linealmente al número de instrucciones (~270 ns por instrucción en esta placa). Durante el trabajo apareció un problema no trivial: sin `volatile` en las variables de sincronización con el depurador, el bucle de espera se compila a un salto a sí mismo a partir de `-O1`.

---

## 2. Introducción

La asignatura Proyecto Hardware plantea el trabajo de una empresa ficticia que recibe en versión beta un juego de Reversi de 8 casillas escrito en C y debe optimizar su zona crítica para un procesador ARM embebido. La práctica 1 recorre ese proceso: depurar el C, traducir a ensamblador ARM y Thumb, programar un temporizador con interrupciones para medir tiempos y comparar los resultados con la optimización del compilador.

El juego se juega contra la máquina: el humano lleva negras y empieza, la máquina lleva blancas. En esta práctica no se utilizan pulsadores, LEDs ni el display de 8 segmentos. El jugador humano introduce su jugada escribiendo en memoria, desde el depurador, las variables `fila`, `columna` y `ready`.

Entorno de trabajo:

- Eclipse con ARM Sourcery GCC 2011.03 (`arm-none-eabi-gcc`).
- openOCD 0.7.0 y JTAG para depurar en la placa.
- Simulador ARM7TDMI del propio entorno (plugin Zylin), con el que se cuentan las instrucciones ejecutadas.

---

## 3. Objetivos

1. Interactuar con la placa real y con el simulador, y depurar el código C del juego observando el tablero en memoria.
2. Implementar en ensamblador ARM `patron_volteo_arm` y `color_ficha_arm`, equivalentes a las funciones C, respetando la modularidad, el AAPCS y la estructura del código C.
3. Implementar `color_ficha_thumb` en Thumb para reducir el tamaño de código.
4. Comprobar las 6 combinaciones de versiones (patron_volteo C/ARM × color_ficha C/ARM/Thumb).
5. Entender cómo se gestionan las interrupciones IRQ a partir del proyecto del timer0.
6. Desarrollar una librería de medición de tiempos con el timer2 (`timer2_inicializar`, `timer2_empezar`, `timer2_leer`, `timer2_parar`) y calibrarla.
7. Comparar tamaño de código, instrucciones ejecutadas y tiempo de ejecución entre las combinaciones, y estudiar el efecto de los niveles de optimización del compilador.
8. (Opcional) Repetir la comparación con una jugada que voltea más fichas.

---

## 4. Metodología

### 4.1. Esquema del proyecto: ficheros y funciones

| Fichero | Origen | Funciones |
|---|---|---|
| `main.c` | Adaptado | `Main` (llama a `sys_init`, `timer2_inicializar` y `reversi8`); `calibrar_timer2` (solo con `CALIBRAR_TIMER2 = 1`) |
| `reversi8_2026.c` / `.h` | Suministrado, adaptado | `init_table`, `esperar_mov`, `color_ficha`, `patron_volteo`, `voltear`, `actualizar_tablero`, `elegir_mov`, `contar`, `actualizar_candidatas`, `reversi8` (la única visible desde fuera) |
| `reversi8_arm.asm` | **Desarrollado** | `color_ficha_arm`, `patron_volteo_arm` |
| `reversi8_thumb.asm` | **Desarrollado** | `color_ficha_thumb` |
| `timer2.c` / `.h` | **Desarrollado** | `timer2_inicializar`, `timer2_empezar`, `timer2_leer`, `timer2_parar`; `timer2_ISR` (privada) |
| `common/` | Suministrado (sin modificar) | `44b.h`, `44blib.c/.h` (`sys_init`, `Delay`), `44binit.asm`, `ld_script.ld`, etc. |
| `emulador/` | Suministrado (sin modificar) | `init_b_2026.asm`, `ld_script.ld` (arranque para el simulador) |
| `timer.c`, `led.c`, `8led.c`, `button.c` (y `.h`) | Suministrado (sin modificar) | Proyecto de ejemplo del timer0 y soporte de placa; no forman parte del juego |

*Tabla 1. Ficheros y funciones del proyecto.*

Solo `reversi8()` es visible desde `main()`. El resto de funciones del juego son `static`. Se exceptúa `color_ficha`, que no puede serlo porque la llama `patron_volteo_arm` desde otro fichero.

La versión de cada función que utiliza el juego se elige en tiempo de compilación con tres definiciones:

| Definición | Fichero | Qué elige |
|---|---|---|
| `PATRON_VOLTEO` | `reversi8_2026.c` | `patron_volteo` (C) o `patron_volteo_arm` (ARM), usada por `actualizar_tablero` y `elegir_mov` |
| `COLOR_FICHA` | `reversi8_2026.c` | Versión de `color_ficha` que llama `patron_volteo` (C) |
| `COLOR_FICHA` | `reversi8_arm.asm` | Versión de `color_ficha` que llama `patron_volteo_arm` (C, ARM o Thumb) |

*Tabla 2. Selección de versiones por `#define`.*

El `.asm` se ensambla con `-x assembler-with-cpp`, por lo que el preprocesador de C funciona también ahí.

### 4.2. Líneas de código y horas de dedicación

**Líneas de código** (`wc -l`, líneas totales y, entre paréntesis, no vacías):

| Fichero | Líneas |
|---|---|
| `reversi8_2026.c` | 487 (421) |
| `reversi8_2026.h` | 14 (11) |
| `reversi8_arm.asm` | 188 (173) |
| `reversi8_thumb.asm` | 78 (72) |
| `timer2.c` | 91 (73) |
| `timer2.h` | 17 (14) |
| `main.c` | 49 (41) |
| **Total** | **924 (805)** |

*Tabla 3. Líneas de código del proyecto. No se cuentan los ficheros suministrados sin modificar (`common/`, `emulador/`, `timer.c`, `led.c`, `8led.c`, `button.c`).*

De ellas, 374 líneas (ensamblador ARM, Thumb y librería timer2) se han escrito desde cero. El resto corresponde al juego y a `main.c`, suministrados y adaptados.

**Horas de dedicación.** El trabajo se ha hecho siempre en pareja, con los dos integrantes trabajando a la vez.

| Fecha | Horas (por persona) | Trabajo |
|---|---|---|
| 14-09-2026 | 2 | Pasos 1 y 2: proyecto en Eclipse, depuración del C, emulador, documentación |
| 21-09-2026 | 2 | Paso 3: `color_ficha_arm`, `patron_volteo_arm`, `#define` y prueba en el simulador |
| 28-09-2026 | 2 | Pruebas del Paso 3 en placa; Pasos 4 y 5: `color_ficha_thumb` y las 6 combinaciones |
| 05-10-2026 | 2 | Pasos 7 a 9: timer2, calibración y tiempos de las 6 combinaciones y de `-O0` a `-Os` |
| **Total** | **8 por persona (16 h-persona)** | |

*Tabla 4. Dedicación.*

### 4.3. Decisiones de diseño

**Preparación del C (Pasos 1 y 2).** `main()` llama a `sys_init()` y después a `reversi8()`. Las variables `fila`, `columna` y `ready` se declaran `volatile` y `esperar_mov` recibe un `volatile char *` (ver el apartado 5.5). Se detectó en el código beta una llamada a `color_ficha` con los argumentos en distinto orden (`FA, CA, tablero`); se corrigió para respetar el prototipo, y la incidencia se anota aquí como pide el guion.

**`char` sin signo.** En `arm-none-eabi-gcc` el tipo `char` es sin signo, de modo que un índice negativo (0 + (−1)) queda como 0xFF. Una sola comparación sin signo (`cmp r1, #DIM` + `bhs`) cubre a la vez `f < DIM` y `f >= 0`: en `color_ficha_arm` bastan 2 comparaciones en lugar de 4. Tras cada suma de `FA`/`CA` se trunca con `and #0xFF` para comportarse igual que el C.

**Acceso a `tablero[f][c]`.** `add r0, r0, r1, lsl #3` más `ldrb r0, [r0, r2]`. La multiplicación por 8 (`DIM`) va integrada en el `add` y la lectura usa direccionamiento base + registro.

**Variables en registros callee-saved (`r4`-`r10`).** En `patron_volteo_arm` los parámetros se copian a registros que sobreviven a la llamada a `color_ficha` y no se recargan de pila dentro del bucle. Solo `es_posicion_correcta` va en pila, porque se pasa su dirección a `color_ficha`.

**Ejecución condicional.** El `if` final y el `return` se evalúan sin ningún salto: `cmp` / `cmpeq` / `moveq` / `movne`.

**Modularidad.** Se mantiene la llamada real a `color_ficha` con parámetros en `r0`-`r3` según AAPCS. No se accede a variables locales de otra función ni se modifica el algoritmo.

**Thumb.** ARMv4T no tiene `blx`. La llamada ARM → Thumb se deja como un `bl` normal: al ser `.thumb_func`, el enlazador inserta un veneer que carga la dirección con el bit 0 a 1 y hace `bx`. Así cambiar de versión sigue siendo solo cambiar un `#define`. El retorno se hace con `bx lr`, no con `pop {pc}`, que en ARMv4T no cambia de estado.

**Librería timer2.** Máxima precisión: preescalado 0 y divisor 1/2, de modo que F = 64 MHz / (1 · 2) = 32 MHz y 1 tick = 1/32 µs. Con `TCNTB2 = 65535` y auto-reload cada vuelta son 65 536 ticks, es decir 2048 µs, y se genera una IRQ. La ISR incrementa `timer2_num_int` y borra `BIT_TIMER2` en `I_ISPC`.

`timer2_leer` devuelve `num_int · 2048 + (65535 − TCNTO2) / 32` µs. Calculado así no desborda hasta 2³² µs (unos 71 min); multiplicar primero por 65 536 ticks desbordaría a los 134 s. Si la IRQ salta entre leer `timer2_num_int` y `TCNTO2`, ambos valores no casan, por lo que se repite la lectura mientras `timer2_num_int` cambie. Solo se tocan los bits del timer2 en `TCFG0`, `TCFG1` y `TCON` (lectura-modificación-escritura) para no afectar al timer0.

**Medida del Paso 8.** `timer2_empezar()` y `timer2_parar()` rodean la llamada a `actualizar_tablero` de la jugada humana en `reversi8()`, y el resultado queda en `tiempo_volteo_us`. Incluye las 8 llamadas a `patron_volteo` y el `voltear`, que es igual en todas las combinaciones. El número de instrucciones se cuenta en el simulador (`arm-none-eabi-gdb` con `target sim`, `stepi` entre la llamada a `actualizar_tablero` y su retorno, veneer incluido). El tiempo se mide en la placa, con 3 repeticiones por combinación.

### 4.4. Marco de pila

Las funciones en ensamblador ARM usan el mismo marco que genera el compilador con `-mapcs-frame`:

```
mov   ip, sp
stmdb sp!, {<regs>, fp, ip, lr, pc}
sub   fp, ip, #4
...
ldmdb fp, {<regs>, fp, sp, pc}
```

Figura 1: marco de `color_ficha_arm` (función hoja, no guarda registros):

```
[fp]       pc
[fp,#-4]   lr
[fp,#-8]   sp anterior (ip)
[fp,#-12]  fp anterior
```

Figura 2: marco de `patron_volteo_arm`. Los parámetros 5.º a 7.º los pasa el llamador en su pila y se leen con `ldrb`:

```
[fp,#12]   color   (7.º parámetro, pila del llamador)
[fp,#8]    SC      (6.º)
[fp,#4]    SF      (5.º)
[fp]       pc
[fp,#-4]   lr
[fp,#-8]   sp anterior
[fp,#-12]  fp anterior
[fp,#-16]  r10
   ...
[fp,#-40]  r4
[fp,#-44]  es_posicion_correcta    <- sp
```

Para comparar, el compilador a `-O0 -mapcs-frame` genera para `patron_volteo` en C:

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

La estructura es la misma (`mov ip,sp` / `stmdb {…,fp,ip,lr,pc}` / `sub fp,ip,#4`, y parámetros 5.º a 7.º en `[fp,#4…]`). La diferencia es que el compilador a `-O0` guarda todo en pila y lo recarga en cada uso, mientras que nuestra versión guarda `r4`-`r10` y deja en pila solo `es_posicion_correcta`.

**Thumb.** `color_ficha_thumb` no crea marco. Es una función hoja, solo usa `r0`-`r3` (caller-saved) y no llama a nadie, por lo que ni `lr` ni ningún registro privado de otra función se modifican. El guion permite reducir el marco en Thumb.

**Rutina de interrupción.** Para `timer2_ISR` (`__attribute__((interrupt("IRQ")))`) GCC genera:

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

Diferencias con una función normal:

- Guarda también registros caller-saved (`r2`, `r3`, `ip`), porque la IRQ llega en cualquier instrucción y el código interrumpido no los ha salvado.
- `lr_irq` vale instrucción interrumpida + 4, así que se retorna con `lr − 4`.
- `subs pc, …` copia SPSR en CPSR: vuelve al modo SVC y reactiva las IRQ.

Camino de la interrupción: vector IRQ (0x18) → controlador vectorizado → `HandlerTIMER2` (`44binit.asm`) → `pISR_TIMER2` → `timer2_ISR`. Se comprobó en la placa con un breakpoint en la ISR: la pila de llamadas muestra `timer2_ISR` ← `Delay`.

### 4.5. Optimizaciones realizadas

1. Comparación sin signo única para el rango de fila y columna (2 comparaciones en lugar de 4).
2. Desplazamiento integrado en el `add` para multiplicar por 8 (en Thumb, `lsl` + `add`).
3. Variables en registros `r4`-`r10`: no hay accesos a pila dentro del bucle, salvo `es_posicion_correcta`.
4. Ejecución condicional en el cierre de `patron_volteo_arm` (sin saltos).
5. `color_ficha_thumb` sin marco de pila y con 30 bytes de código (15 instrucciones de 16 bits). Reutiliza el 0 de `r0` para escribir `*es_posicion_correcta = 0`.
6. Marco de pila mínimo en las funciones hoja.

### 4.6. Código desarrollado

Se incluye el código de las funciones en ensamblador, la librería timer2 y los cambios en `main.c` y en el juego. Cada función va con su cabecera, que indica entrada, salida y uso de registros.

#### 4.6.1. `reversi8_arm.asm`

Cabecera y constantes:

```asm
.text
.arm

.equ DIM,               8
.equ CASILLA_VACIA,     0
.equ NO_HAY_MODELO,     0
.equ MODELO_ENCONTRADO, 1

/* Versión de color_ficha que llama patron_volteo_arm: color_ficha (C),
   color_ficha_arm (ARM) o color_ficha_thumb (Thumb) */
#define COLOR_FICHA color_ficha
```

`color_ficha_arm`:

```asm
/*******************************************************************************************
* C original:
*
* char color_ficha(char tablero[][DIM], char f, char c, int *es_posicion_correcta)
* {
*     char ficha;
*     if ((f < DIM) && (f >= 0) && (c < DIM) && (c >= 0) && (tablero[f][c] != CASILLA_VACIA))
*     {
*         *es_posicion_correcta = 1;
*         ficha = tablero[f][c];
*     }
*     else
*     {
*         *es_posicion_correcta = 0;
*         ficha = CASILLA_VACIA;
*     }
*     return ficha;
* }
*
* Entrada:  r0 = tablero, r1 = f, r2 = c, r3 = es_posicion_correcta (puntero)
* Salida:   r0 = ficha
* Locales:  ficha -> r0 (no necesita pila)
*******************************************************************************************/
.global color_ficha_arm
color_ficha_arm:
        mov     ip, sp
        stmdb   sp!, {fp, ip, lr, pc}
        sub     fp, ip, #4

        cmp     r1, #DIM                /* sin signo: cubre f < DIM y f >= 0 */
        bhs     cf_else
        cmp     r2, #DIM                /* sin signo: cubre c < DIM y c >= 0 */
        bhs     cf_else
        add     r0, r0, r1, lsl #3      /* r0 = &tablero[f][0] (DIM = 8 = 2^3 bytes por fila) */
        ldrb    r0, [r0, r2]            /* ficha = tablero[f][c] */
        cmp     r0, #CASILLA_VACIA
        beq     cf_else

        mov     r1, #1
        str     r1, [r3]                /* *es_posicion_correcta = 1 */
        ldmdb   fp, {fp, sp, pc}

cf_else:
        mov     r1, #0
        str     r1, [r3]                /* *es_posicion_correcta = 0 */
        mov     r0, #CASILLA_VACIA      /* ficha = CASILLA_VACIA */
        ldmdb   fp, {fp, sp, pc}
```

`patron_volteo_arm`:

```asm
/*******************************************************************************************
* C original:
*
* int patron_volteo(char tablero[][DIM], int *longitud, char FA, char CA, char SF, char SC, char color)
* {
*     int es_posicion_correcta;
*     char casilla;
*
*     FA = FA + SF;
*     CA = CA + SC;
*     casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
*     while ((es_posicion_correcta == 1) && (casilla != color))
*     {
*         FA = FA + SF;
*         CA = CA + SC;
*         *longitud = *longitud + 1;
*         casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
*     }
*     if ((es_posicion_correcta == 1) && (casilla == color))
*         return MODELO_ENCONTRADO;
*     else
*         return NO_HAY_MODELO;
* }
*
* Entrada:  r0 = tablero, r1 = longitud (puntero), r2 = FA, r3 = CA
*           [fp,#4] = SF, [fp,#8] = SC, [fp,#12] = color   (en la pila del llamador)
* Salida:   r0 = MODELO_ENCONTRADO / NO_HAY_MODELO
*
* Registros (r4-r10 sobreviven a la llamada a color_ficha):
*   r4 = tablero   r5 = longitud   r6 = FA   r7 = CA
*   r8 = SF        r9 = SC         r10 = color   r0 = casilla (valor devuelto)
*******************************************************************************************/
.global patron_volteo_arm
.extern color_ficha
patron_volteo_arm:
        mov     ip, sp
        stmdb   sp!, {r4-r10, fp, ip, lr, pc}
        sub     fp, ip, #4
        sub     sp, sp, #4              /* int es_posicion_correcta -> [sp] */

        mov     r4, r0                  /* r4 = tablero */
        mov     r5, r1                  /* r5 = longitud */
        ldrb    r8, [fp, #4]            /* r8 = SF */
        ldrb    r9, [fp, #8]            /* r9 = SC */
        ldrb    r10, [fp, #12]          /* r10 = color */

        add     r6, r2, r8              /* FA = FA + SF */
        and     r6, r6, #0xFF           /* FA es char: truncar a 8 bits */
        add     r7, r3, r9              /* CA = CA + SC */
        and     r7, r7, #0xFF
        mov     r0, r4
        mov     r1, r6
        mov     r2, r7
        mov     r3, sp                  /* &es_posicion_correcta */
        bl      COLOR_FICHA             /* casilla = color_ficha(...) */

pv_while:
        ldr     r1, [sp]
        cmp     r1, #1                  /* es_posicion_correcta == 1 */
        bne     pv_fin_while
        cmp     r0, r10                 /* casilla != color */
        beq     pv_fin_while

        add     r6, r6, r8              /* FA = FA + SF */
        and     r6, r6, #0xFF
        add     r7, r7, r9              /* CA = CA + SC */
        and     r7, r7, #0xFF
        ldr     r1, [r5]                /* *longitud = *longitud + 1 */
        add     r1, r1, #1
        str     r1, [r5]
        mov     r0, r4
        mov     r1, r6
        mov     r2, r7
        mov     r3, sp
        bl      COLOR_FICHA
        b       pv_while

pv_fin_while:
        ldr     r1, [sp]
        cmp     r1, #1                  /* es_posicion_correcta == 1 */
        cmpeq   r0, r10                 /* y casilla == color */
        moveq   r0, #MODELO_ENCONTRADO
        movne   r0, #NO_HAY_MODELO
        ldmdb   fp, {r4-r10, fp, sp, pc}
```

#### 4.6.2. `reversi8_thumb.asm`

```asm
.text
.thumb

.equ DIM,           8
.equ CASILLA_VACIA, 0

/* Mismo C original que color_ficha_arm.
* Entrada:  r0 = tablero, r1 = f, r2 = c, r3 = es_posicion_correcta (puntero)
* Salida:   r0 = ficha
* Marco de pila: ninguno (función hoja, solo r0-r3, no llama a nadie).
* Retorno con bx lr: el llamador es ARM (bit 0 de lr = 0), así que vuelve a estado ARM. */
.global color_ficha_thumb
.type   color_ficha_thumb, %function
.thumb_func
color_ficha_thumb:
        cmp     r1, #DIM                /* sin signo: cubre f < DIM y f >= 0 */
        bhs     cft_else
        cmp     r2, #DIM                /* sin signo: cubre c < DIM y c >= 0 */
        bhs     cft_else
        lsl     r1, r1, #3              /* Thumb no tiene desplazamiento integrado en add */
        add     r0, r0, r1              /* r0 = &tablero[f][0] */
        ldrb    r0, [r0, r2]            /* ficha = tablero[f][c] */
        cmp     r0, #CASILLA_VACIA
        beq     cft_else

        mov     r1, #1
        str     r1, [r3]                /* *es_posicion_correcta = 1 */
        bx      lr

cft_else:
        mov     r0, #CASILLA_VACIA      /* ficha = CASILLA_VACIA (= 0) */
        str     r0, [r3]                /* *es_posicion_correcta = 0 (reutiliza el 0 de r0) */
        bx      lr
```

#### 4.6.3. Librería `timer2.c`

```c
/*--- constantes ---*/
/* Máxima precisión: preescalado 0 y divisor 1/2 -> F = 64 MHz / (1 * 2) = 32 MHz,
 * 32 ticks por us */
#define TICKS_POR_US    32
#define CUENTA_MAX      65535   // TCNTB2: de 65535 a 0, periodo de 65536 ticks = 2048 us

/* Bits del timer2 en TCON (bits 12-15) */
#define T2_START        (1 << 12)
#define T2_MANUAL_UPD   (1 << 13)
#define T2_INVERTER     (1 << 14)
#define T2_AUTO_RELOAD  (1 << 15)
#define T2_BITS         (T2_START | T2_MANUAL_UPD | T2_INVERTER | T2_AUTO_RELOAD)

/* nº de periodos completos del timer2 desde timer2_empezar; la modifica la ISR */
static volatile unsigned int timer2_num_int = 0;

static void timer2_ISR(void) __attribute__((interrupt("IRQ")));

static void timer2_ISR(void)
{
    timer2_num_int++;

    /* I_ISPC es de solo escritura: con |= se leería basura */
    rI_ISPC = BIT_TIMER2;
}

void timer2_inicializar(void)
{
    /* sys_init() ya deja INTMOD en IRQ, INTCON vectorizado y BIT_GLOBAL desenmascarado */
    pISR_TIMER2 = (unsigned) timer2_ISR;
    rINTMSK &= ~(BIT_TIMER2);

    /* preescalado 1 (timers 2 y 3, bits 8-15 de TCFG0) = 0, sin tocar el del timer0 */
    rTCFG0 &= ~(0xFF << 8);
    /* mux del timer2 (bits 8-11 de TCFG1) = 0000 -> divisor 1/2 */
    rTCFG1 &= ~(0xF << 8);

    rTCNTB2 = CUENTA_MAX;
    rTCMPB2 = 0;

    /* timer2 parado; solo se tocan sus bits para no afectar al resto de timers */
    rTCON &= ~T2_BITS;
}

void timer2_empezar(void)
{
    timer2_num_int = 0;

    /* manual update carga TCNTB2 en el contador; después se arranca con auto-reload
     * y a la vez se quita el manual update */
    rTCON = (rTCON & ~T2_BITS) | T2_MANUAL_UPD;
    rTCON = (rTCON & ~T2_BITS) | T2_START | T2_AUTO_RELOAD;
}

unsigned int timer2_leer(void)
{
    unsigned int n, cuenta;

    /* si la ISR salta entre las dos lecturas, n y cuenta no casan: se repite */
    do {
        n = timer2_num_int;
        cuenta = rTCNTO2;
    } while (n != timer2_num_int);

    /* cada periodo completo son 65536 / 32 = 2048 us */
    return n * ((CUENTA_MAX + 1) / TICKS_POR_US) + (CUENTA_MAX - cuenta) / TICKS_POR_US;
}

unsigned int timer2_parar(void)
{
    unsigned int t = timer2_leer();

    rTCON &= ~T2_START;
    return t;
}
```

#### 4.6.4. `main.c` y cambios en `reversi8_2026.c`

```c
/* main.c */
void Main(void)
{
    sys_init();             // Inicialización de la placa, interrupciones y puertos
    timer2_inicializar();

#if CALIBRAR_TIMER2
    calibrar_timer2();      // Delay(10/100/10000/100000) -> tiempos_calibracion[4]
#endif

    reversi8();
}
```

```c
/* reversi8_2026.c: variables de sincronización con el depurador */
volatile char fila = 0, columna = 0, ready = 0;
static void esperar_mov(volatile char *ready);

/* us que tarda actualizar_tablero en la última jugada humana */
unsigned int tiempo_volteo_us = 0;

/* Selección de versiones */
#define PATRON_VOLTEO patron_volteo     /* o patron_volteo_arm */
#define COLOR_FICHA   color_ficha       /* o color_ficha_arm / color_ficha_thumb */

/* en reversi8(): medida de la jugada humana */
tablero[fila][columna] = FICHA_NEGRA;
timer2_empezar();
actualizar_tablero(tablero, fila, columna, FICHA_NEGRA);
tiempo_volteo_us = timer2_parar();
```

---

## 5. Resultados

### 5.1. Pruebas funcionales (Pasos 3 y 5)

Jugada de referencia: ficha negra en (2,3), es decir `fila = 2`, `columna = 3`, `ready = 1`. Tras la jugada humana y la respuesta de la máquina (blanca en (2,2)) el tablero esperado es (0 = vacía, 1 = blanca, 2 = negra):

```
fila 2: 00 00 01 02 00 00 00 00
fila 3: 00 00 00 01 02 00 00 00
fila 4: 00 00 00 02 01 00 00 00
```

La negra en (2,3) voltea (3,3); la blanca en (2,2) vuelve a voltearla por la diagonal.

| patron_volteo | color_ficha | Resultado | Entorno |
|---|---|---|---|
| C | C | Tablero correcto | Simulador |
| ARM | C | Tablero correcto | Simulador y placa |
| ARM | ARM | Tablero correcto | Simulador y placa |
| C | Thumb | Tablero correcto | Placa |
| ARM | Thumb | Tablero correcto | Placa |
| C | ARM | Tablero correcto | Simulador y placa |

*Tabla 5. Comprobación funcional de las 6 combinaciones.*

Cambio de estado ARM ↔ Thumb comprobado en placa (C-Thumb, jugada (2,3), Step Into en modo instrucción):

| Momento | `cpsr` | Bit T | Estado |
|---|---|---|---|
| Antes del `bl` (0x0c00136c) | 0x80000013 | 0 | ARM, Supervisor |
| Tras el `bx ip` del veneer (0x0c00249c) | 0x80000033 | 1 | Thumb |
| Tras el `bx lr` (0x0c001374) | 0x60000013 | 0 | ARM (r0 = 0: casilla vacía o fuera) |

*Tabla 6. Evolución del bit T de CPSR en una llamada a `color_ficha_thumb`.*

El enlazador coloca el veneer `__color_ficha_thumb_from_arm` al final de `.text`: 12 bytes (más 4 de relleno) y 2 instrucciones ARM extra por llamada.

### 5.2. Calibración de la librería timer2 (Paso 7)

Se mide con timer2 el tiempo de `Delay()` (que espera n · 100 µs) para 1 ms, 10 ms, 1 s y 10 s:

| `Delay` | Esperado (µs) | timer2 (µs) | Desviación |
|---|---|---|---|
| 1 ms | 1 000 | 1 023 | +2,3 % |
| 10 ms | 10 000 | 10 169 | +1,7 % |
| 1 s | 1 000 000 | 1 016 745 | +1,7 % |
| 10 s | 10 000 000 | 10 167 390 | +1,7 % |

*Tabla 7. Calibración de timer2 frente a `Delay()` (placa, 29-09-2026).*

**Análisis.** La desviación es constante (~1,7 %) y se atribuye a `Delay()`, no a timer2. `Delay(0)` calibra `delayLoopCount` con el watchdog y la división entera redondea (8 000 000 / (i · 64)). En el caso de 1 ms, además, pesa el coste fijo de llamar a `timer2_empezar`, `timer2_parar` y `Delay`. La librería mide de forma coherente desde 1 ms hasta 10 s.

### 5.3. Tamaño, instrucciones y tiempo de las 6 combinaciones (Paso 8)

Tamaños de las funciones (bytes, `-O0 -mapcs-frame`): `patron_volteo` 260, `color_ficha` 196, `patron_volteo_arm` 168, `color_ficha_arm` 72, `color_ficha_thumb` 30, veneer ARM→Thumb 12 (+4 de relleno).

| patron_volteo | color_ficha | Tamaño (bytes) | Instrucciones | Tiempo (µs) | ns/instrucción | Mejora de tiempo |
|---|---|---|---|---|---|---|
| C | C | 456 | 1037 | 284 | 274 | — |
| C | ARM | 332 | 821 | 228 | 278 | −20 % |
| C | Thumb | 290 (+12 veneer) | 815 | 222 | 272 | −22 % |
| ARM | C | 364 | 950 | 258 | 272 | −9 % |
| ARM | ARM | 240 | 734 | 200 | 272 | −30 % |
| ARM | Thumb | 198 (+12 veneer) | 728 | 194 | 266 | −32 % |

*Tabla 8. Comparación de las combinaciones, jugada negra (2,3), `-O0`. Instrucciones contadas en simulador; tiempos medidos en placa (3 repeticiones, variación de ±1 µs) el 29-09-2026.*

**Análisis.**

- El tiempo sigue casi linealmente al número de instrucciones, con unos 270 ns por instrucción, que son unos 17 ciclos a 64 MHz. Está muy lejos del ideal de 1-3 ciclos, lo que es coherente con una memoria externa lenta.
- Cambiar `color_ficha` rinde más que cambiar `patron_volteo` (C-ARM 228 µs frente a ARM-C 258 µs). `color_ficha` se llama en cada paso del bucle y la versión C a `-O0` guarda y recarga todo de pila.
- Thumb es más rápido que ARM (222 < 228 y 194 < 200) aunque cada llamada pasa por el veneer, que añade 2 instrucciones. La explicación probable es que la memoria de la placa va con bus de datos de 16 bits y la caché no está activada: cada instrucción ARM de 32 bits necesita 2 accesos y una Thumb solo 1, y se ahorra en la búsqueda de instrucciones más de lo que cuesta el veneer. Es una hipótesis razonada a partir de la configuración de la placa; no se ha verificado con una medida específica.
- ARM-Thumb es a la vez la combinación más rápida y la más pequeña.

### 5.4. Niveles de optimización del compilador (Paso 9)

Combinación C-C, jugada (2,3), tiempos en placa el 05-10-2026 (3 repeticiones):

| Nivel | `.text` de `reversi8_2026.o` (bytes) | `patron_volteo` (bytes) | `color_ficha` (bytes) | Instrucciones | Tiempo (µs) | Mejora frente a `-O0` |
|---|---|---|---|---|---|---|
| `-O0` | 5208 | 260 | 196 | 1037 | 284 | — |
| `-O1` | 1524 | 212 | 56 | 539 | 125 | −56 % |
| `-O2` | 2068 | 220 (`.clone`) | 56 | 481 | 101 | −64 % |
| `-O3` | 1856 | integrada en `actualizar_tablero` | 56 | 378 | **65** | **−77 %** |
| `-Os` | 1368 | 164 (`.clone`) | 52 | 582 | 122 | −57 % |

*Tabla 9. Efecto del nivel de optimización en la combinación C-C.*

**Análisis (qué ha hecho el compilador).**

- `-O1`: `color_ficha` y `patron_volteo` siguen siendo funciones, pero `color_ficha` pasa de 196 a 56 bytes porque deja de guardar los parámetros en pila. Ya mejora a la mejor versión en ensamblador (125 µs frente a 194 µs).
- `-O2`: `color_ficha` se integra en un clon de `patron_volteo` y desaparece la llamada.
- `-O3`: se integra también `patron_volteo` en `actualizar_tablero` y no queda ninguna llamada. Es el más rápido (65 µs) y no el más pequeño.
- `-Os`: es el más pequeño (1368 bytes). Conserva la llamada a `color_ficha` y paga el marco de pila en cada una, por lo que queda entre `-O1` y `-O2` en tiempo.
- A partir de `-O1` desaparecen como funciones `voltear`, `contar`, `elegir_mov`, `init_table` y `esperar_mov`, integradas en quien las llama.
- Compromiso tamaño/velocidad: `-Os` minimiza el tamaño y `-O3` el tiempo.

### 5.5. Apartado opcional: jugada que voltea más fichas

Desde el tablero inicial toda primera jugada voltea 1 sola ficha, y la segunda negra también. Con el simulador se buscó una secuencia de partida, con las respuestas reales de `elegir_mov`, en la que el humano voltea 3 fichas:

| Turno | Humano (negras) | Respuesta de la máquina |
|---|---|---|
| 1 | (2,3) | Blanca en (2,2) |
| 2 | (5,4) | Blanca en (5,5) |
| 3 | (5,6) | Blanca en (2,4) |
| 4 | **(1,4)** | Voltea (2,4), (3,4) y (4,4): **3 fichas** en la misma dirección (sur) |

*Tabla 10. Secuencia de partida que lleva a una jugada con 3 volteos.*

| Combinación | Instrucciones, jugada 4 | Instrucciones, jugada (2,3) |
|---|---|---|
| C-C | 1245 | 1037 |
| C-ARM | 951 | 821 |
| C-Thumb | 944 | 815 |
| ARM-C | 1138 | 950 |
| ARM-ARM | 844 | 734 |
| ARM-Thumb | 837 | 728 |

*Tabla 11. Instrucciones de `actualizar_tablero` en la jugada de 3 volteos (simulador, `-O0`).*

Para la combinación C-C por nivel de optimización:

| Nivel | Jugada 4: instr. | Jugada 4: µs | Jugada 4: ns/instr. | Jugada (2,3): instr. | Jugada (2,3): µs | Jugada (2,3): ns/instr. |
|---|---|---|---|---|---|---|
| `-O0` | 1245 | 340 | 273 | 1037 | 284 | 274 |
| `-O1` | 649 | 144 | 222 | 539 | 125 | 232 |
| `-O2` | 567 | 108 | 190 | 481 | 101 | 210 |
| `-O3` | 464 | 76 | 164 | 378 | 65 | 172 |
| `-Os` | 695 | 142 | 204 | 582 | 122 | 210 |

*Tabla 12. Jugada de 3 volteos frente a la de referencia, C-C por nivel de optimización (tiempos en placa, instrucciones en simulador).*

**Análisis.** Voltear 3 fichas en lugar de 1 cuesta entre un 15 % y un 23 % más de instrucciones y, en C-C, entre un 7 % (`-O2`) y un 20 % (`-O0`) más de tiempo; el orden de las combinaciones y de los niveles no cambia. Los ns/instrucción bajan al optimizar (de 273 en `-O0` a 164 en `-O3`): el código optimizado ejecuta menos instrucciones y, además, cada una sale más barata. No se ha investigado la causa. Los tiempos de la jugada 4 solo se midieron para C-C por nivel de optimización; para las 6 combinaciones solo se cuentan instrucciones.

### 5.6. Problemas encontrados y soluciones

1. **El preprocesador rompía el `.asm`.** Eclipse ensambla con `gcc -x assembler-with-cpp`, así que las líneas con `#` en la columna 0 se interpretaban como directivas (`token "[" is not valid in preprocessor expressions`, `unterminated #else`). Solución: todos los comentarios del `.asm` pasan a ser `/* … */`.
2. **Suposiciones erróneas del esqueleto inicial.** Se proponía `blx`, que no existe en ARMv4T, y extender el signo de los `char`, que en realidad son sin signo. Se corrigieron ambas cosas.
3. **Cómo introducir jugadas en el simulador.** Breakpoint en `esperar_mov(&ready)`, escribir `fila`, `columna` y por último `ready = 1`, y reanudar con F8. En la placa las direcciones cambian por usar otro linker script. Conviene usar `&tablero` en el monitor de memoria en lugar de direcciones fijas. Las negras son `02` y las blancas `01`.
4. **En la placa parecía que el programa se colgaba.** Los mensajes "Target not available" y la memoria en rojo solo indican que la placa está ejecutando y GDB no puede leer. El programa estaba en `esperar_mov` esperando la siguiente jugada; se ve pulsando Suspend.
5. **`ready` sin `volatile`.** `fila`, `columna` y `ready` las escribe el depurador y el compilador no lo sabe. A `-O0` funciona por casualidad (siempre recarga de memoria), pero a `-O2` el bucle de `esperar_mov` se compila como `b .` (salto a sí mismo). Solución: `volatile char fila, columna, ready` y `esperar_mov(volatile char *ready)`. Comprobado con `objdump`: ningún bucle a sí mismo de `-O0` a `-Os`. Este cambio es imprescindible para el Paso 9.
6. **El orden de las variables globales cambia con la optimización.** A `-O1` en adelante GCC activa `-fsection-anchors`, que agrupa las globales y las direcciona con una única base y un desplazamiento, y decide su orden (`ready`, `fila`, `columna`, frente a `fila`, `columna`, `ready` en `-O0`). Al escribir tres bytes seguidos con el orden de `-O0` se obtuvo una jugada inválida y una medida inservible (116 µs). Solución: escribir las variables siempre por nombre desde Expressions, con `ready` la última, y no suponer nunca la posición relativa. A `-O3` y `-Os`, la variable `ready` hay que escribirla como `::ready`, porque `esperar_mov` se integra y el nombre resuelve al parámetro (`<value optimized out>`).
7. **El breakpoint de `esperar_mov(&ready)` no para a partir de `-O1`.** `esperar_mov` se integra y la línea se queda sin instrucciones propias; Eclipse mueve el breakpoint a una línea posterior al bucle de espera. Solución: breakpoint en la línea siguiente a la medición (la de `actualizar_candidatas`), que tiene código en todos los niveles; F8 + Suspend para introducir la jugada.
8. **A `-O2`, `tiempo_volteo_us` vale 0 al parar en el breakpoint.** El planificador de instrucciones mezcla las de las líneas de `timer2_parar`, la asignación y `actualizar_candidatas`, y el breakpoint salta antes del `str` que guarda la variable. La medida es correcta y solo se lee demasiado pronto. Solución: leer `r0` en Registers (es lo que devolvió `timer2_parar`) o dar un par de Step Over.
9. **La calibración daba solo el primer valor.** Seguía activo un breakpoint en la ISR y el programa se paraba en la primera IRQ. Solución: quitar el breakpoint.
10. **`I_ISPC` es de solo escritura.** `rI_ISPC |= BIT_TIMER2` leía basura y podía borrar otras interrupciones pendientes. Solución: `rI_ISPC = BIT_TIMER2`.
11. **Al cambiar el nivel de optimización y hacer solo Build no se recompila.** `make` solo mira las fechas de los `.c`. Síntoma: el mismo tiempo en los 5 niveles. Dos de las medidas de la jugada 4 estaban mal por esto y se repitieron (`-O1`: 124 → 144 µs; `-O2`: 101 → 108 µs). Solución: Clean antes de cada medida y comprobar el `.text` de `reversi8_2026.o` (5208, 1524, 2068, 1856 y 1368 bytes para `-O0`, `-O1`, `-O2`, `-O3` y `-Os`). Lección: hay que cruzar el tiempo con el número de instrucciones; un valor suelto no delata el error.
12. **Hay tres configuraciones de compilación (Debug, Release y Placa)** y el nivel de optimización se guarda en el `.cproject` de cada una. Si se cambia una y se compila otra, el resultado no cambia. Además, al compartir el `.cproject`, cada integrante puede tener un nivel distinto.

---

## 6. Conclusiones

**Puntos fuertes**

- Las tres versiones en ensamblador son funcionalmente equivalentes al C en las 6 combinaciones, probadas en simulador y en placa, respetando la modularidad, el AAPCS y el marco de pila del compilador.
- Con la reescritura manual se consigue hasta un −32 % de tiempo y un −57 % de tamaño (456 → 198 bytes, sin contar los 12 del veneer) frente al C sin optimizar. La combinación ARM + Thumb es la mejor en ambos criterios.
- La librería timer2 mide de 1 ms a 10 s con una desviación frente a `Delay()` constante y atribuible a esta última.
- Con optimización del compilador (`-O3`) se obtiene un −77 % en tiempo con un cambio de una opción del proyecto.

**Puntos débiles**

- La comparación entre ensamblador y niveles de optimización no es equivalente: en las versiones ensambladas el resto del juego sigue a `-O0`. No se ha medido ensamblador combinado con `-O2` o `-O3`.
- Cada medida es de una sola jugada, tomada en 3 repeticiones, e incluye `voltear` y las 8 direcciones de `actualizar_tablero`; no se mide `patron_volteo` aislada.
- El número de instrucciones se obtiene en el simulador y el tiempo en la placa, por lo que se han de cruzar a mano.
- La explicación de por qué Thumb supera a ARM es una hipótesis no verificada con una medida específica.
- La medida depende del depurador (introducir jugadas por memoria) y es sensible a errores de procedimiento (ver los problemas 6 a 8 y 11).

**Mejoras posibles**

- Medir las versiones en ensamblador con el resto del juego a `-O2`/`-O3`.
- Medir `patron_volteo` y `color_ficha` de forma individual, y no solo `actualizar_tablero`.
- Activar la caché de la placa o medir con ella y comprobar la hipótesis del bus de 16 bits.
- Automatizar la entrada de jugadas y las medidas para evitar errores manuales.

**Mensaje final.** Con un tiempo de unos 270 ns por instrucción, reducir instrucciones es reducir tiempo. La reescritura a mano es una buena herramienta cuando el compilador se usa sin optimizar, pero no sustituye a activar `-O2`/`-O3`. Y cualquier optimización del compilador exige que las variables compartidas con el exterior (depurador, interrupciones) estén bien declaradas como `volatile`.

**Firma y autoría.** Esta memoria y el código que la acompaña han sido realizados por Sergio Benedi (NIP 873042) y Pablo Gimeno (NIP 960280).

---

## 7. Referencias

1. *Guion de la Práctica 1, Proyecto Hardware 2026/2027* y transparencias asociadas (`PH_practica1_2026.pdf`, `PH_pract1_2026_slides.pdf`, `aclaracion_fuentes.pdf`).
2. *Guía del entorno de desarrollo* (`GuiaEntorno.pdf`).
3. *Práctica 1. Lenguajes de alto nivel y ensamblador* y *Práctica 2. Modos de ejecución, gestión de excepciones y E/S mediante interrupciones* (`P1-EC.pdf`, `P2-EC.pdf`), Universidad Complutense de Madrid.
4. *Sistema de memoria y de entrada/salida en la placa S3CEV40* (`Entradasalida.pdf`).
5. ARM Ltd., *Procedure Call Standard for the ARM Architecture* (AAPCS, IHI0042F).
6. *Cómo redactar una memoria técnica*, Proyecto Hardware 1, Universidad de Zaragoza (rev. septiembre 2026).
