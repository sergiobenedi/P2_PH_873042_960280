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
| 2026-09-15 | | | Montar el proyecto en Eclipse, depurar el código C y añadir el emulador |
| 2026-09-16 | | | Documentación del proyecto (README, PRACTICA.md) |
| 2026-09-21 | | | Paso 3: color_ficha_arm, patron_volteo_arm, #define y prueba en el simulador |

Total: _ h (pendiente de rellenar)

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
| C   | Thumb | | pendiente (Paso 4) |
| ARM | Thumb | | pendiente (Paso 4) |
| C   | ARM   | tablero correcto | ✅ 2026-09-28 |

### 3.2 Rendimiento (Pasos 8 y 9) — pendiente
| Combinación | Tamaño (bytes) | Instrucciones | Tiempo (µs) |
|-------------|----------------|---------------|-------------|

| Optimización (C-C) | Tamaño | Instrucciones | Tiempo |
|--------------------|--------|---------------|--------|
| -O0 | | | |
| -O1 | | | |
| -O2 | | | |
| -O3 | | | |
| -Os | | | |

---

## 4. Datos del proyecto `[MET]`
| Fichero | Funciones | Líneas |
|---------|-----------|--------|
| reversi8_arm.asm | color_ficha_arm, patron_volteo_arm | 184 |
| reversi8_2026.c | juego (proporcionado, modificado) | 476 |

Actualizar con `wc -l` antes de entregar.

---

## 5. Ideas para las conclusiones `[CONC]`
- El `char` sin signo simplifica las comprobaciones de rango en ensamblador.
- Mantener las variables en registros callee-saved elimina los accesos a memoria dentro del bucle.
