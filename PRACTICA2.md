# PRÁCTICA 2 — Sistema de E/S y dispositivos básicos (Reversi 8 en placa)

> Resumen de trabajo sacado de `P2_DOCUMENTACION/` (`PH_practica2_2026.pdf` = guion,
> `Entradasalida.pdf`, `P2-EC.pdf`, manual `um_s3c44box.pdf`, `S3CEV40_UserGuide.pdf`, transparencias de
> E/S) y del código actual (P1). Lo que no viene en los documentos se marca como **[decisión nuestra]**
> o **[verificar en placa]**.

**Entrega:** ~23 de noviembre de 2026 (fecha definitiva en Moodle).
ZIP único por pareja: `p2_873042-Apellidos_960280-Apellidos.zip` con la memoria en PDF + código fuente.

---

## 1. Qué pide la práctica (en una línea por paso)

| Paso | Qué hay que hacer | ¿Necesita placa? |
|------|-------------------|------------------|
| 1 | Estudiar `Entradasalida.pdf` y `P2-EC.pdf` en detalle | No |
| 2 | Entender el proyecto ejemplo (botones, timer0, 8led, leds). Ver el ensamblador que genera gcc para una RSI (`__attribute__((interrupt("IRQ")))`) y compararlo con una subrutina normal. Arreglar E/S: `static`, `volatile` y no machacar bits de otros dispositivos | Parcial (se puede ver el asm en el emulador) |
| 3 | Una función que inicialice **todos** los periféricos de forma centralizada. Las RSI **no comparten variables** con el programa principal | No (diseño) |
| 4 | Pila de depuración circular de **150 eventos** (`lista.c/.h`, `eventos.h`, `guardar_eventos(uint8_t ID_evento, uint32_t instante)`), situada al final de la RAM, antes de las pilas de modo | No para escribirla, sí para probarla |
| 5 | Medir rebotes de los pulsadores con la pila de depuración y eliminarlos con una **máquina de estados con timer** (trp → encuesta cada 50 ms → trd). Dejar un **código de test** para enseñar los rebotes al profesor | **Sí** |
| 6 | Jugar con botones + 8led: máquina de estados F/C, autoincremento con **timer1**, confirmación con parpadeo del led izquierdo | **Sí** para probar |
| Opc. | Introducir valores con el **teclado matricial** (10 teclas) | **Sí** |
| Memoria | Ver §6 | No |

Regla dura del guion: **las RSI deben ser ligeras, nunca espera activa dentro de una IRQ**.

---

## 2. Lo que vamos a necesitar

### 2.1 Hardware (datos de `Entradasalida.pdf` y código)

| Dispositivo | Cómo se usa |
|-------------|-------------|
| Pulsadores | Pines 6 y 7 del puerto G → **EINT6 / EINT7** (línea `BIT_EINT4567`). Pulsado = **0** en `rPDATG` (pull-up activado con `rPUPG`). En `rEXTINTPND`: bit 2 (valor 4) = EINT6, bit 3 (valor 8) = EINT7. **[verificar en placa]** cuál es el izquierdo y cuál el derecho |
| LEDs | Bits 9 y 10 de `rPDATB`, activos a nivel bajo (`led.c`) |
| 8led | Escritura en `LED8ADDR` con la tabla de `8led.h` (ya tiene `F` = índice 15 y `C` = índice 12, y `blank`) |
| Timer0 | Máquina antirrebotes (trp, encuesta 50 ms, trd) **[decisión nuestra]** |
| Timer1 | Autoincremento cada 300 ms tras 0,5 s pulsado (**obligatorio timer1** según el guion). Puede reutilizarse para el parpadeo de confirmación, que nunca coincide en el tiempo **[decisión nuestra]** |
| Timer2 | Ya hecho en P1 (`timer2.c`): marca de tiempo en µs para `guardar_eventos` |
| Teclado (opc.) | EINT1, banco 3 (`0x0600_0000`). Escaneo leyendo las direcciones `0x0600_00FD/FB/F7/EF` (cero en una fila cada vez) y mirando D0-D3. Teclas habilitadas: figura 2 del guion |

Relojes: MCLK = 64 MHz. `f_timer = MCLK / (preescalado + 1) / divisor`. Timers 0-1 comparten preescalado
(`TCFG0[7:0]`), timers 2-3 el suyo (`TCFG0[15:8]`); cada uno su divisor en `TCFG1`.

### 2.2 Mapa de memoria y pilas (de `common/44binit.asm` y `option.h`)

- Programa a partir de `0x0C000000`. `_ISR_STARTADDRESS = 0x0C7FFF00` (tabla `pISR_*`).
- Pilas de modo (crecen hacia abajo desde estas cimas):
  User `0x0C7FF000` (no se inicializa), SVC `0x0C7FF100`, Undef `0x0C7FF200`, Abort `0x0C7FF300`,
  IRQ `0x0C7FF400`, FIQ `0x0C7FF500`. Cada una 256 B.
- **`Main` corre en modo SVC**: su pila parte de `0x0C7FF100` y baja por debajo de `0x0C7FF000` sin límite.
- Pila de depuración: 150 eventos × 8 B (id + instante, alineado a palabra) = **1200 B (0x4B0)**.
  Propuesta **[decisión nuestra]**: base en `0x0C7FE000` (deja ~4 KB a la pila SVC) y comprobar en el
  `.map` que el fin de `.bss` queda muy por debajo. Definirlo con un `#define` en `lista.h`, no en el linker.
- La pila IRQ solo tiene 256 B: las RSI que llamen a `guardar_eventos` → `timer2_leer` gastan pila; vigilarlo.

### 2.3 Ficheros nuevos

| Fichero | Contenido |
|---------|-----------|
| `eventos.h` | Un `enum` con un ID único por evento. Propuesta: `EV_BOTON_IZQ`, `EV_BOTON_DER`, `EV_TIMER0`, `EV_TIMER1`, `EV_BOTON_SOLTADO`, `EV_FIN_TRD`, `EV_JUGADA_CONFIRMADA`, `EV_JUGADA_CANCELADA`, `EV_TECLADO`… |
| `lista.c/.h` | Lista circular de 150 entradas en dirección fija, `guardar_eventos(uint8_t, uint32_t)` y una inicialización. Índice que da la vuelta al llegar a 150 |
| `botones.c/.h` (o ampliar `button.c`) | RSI EINT4567 + máquina antirrebotes con timer0 |
| `timer1.c/.h` (o ampliar `timer.c`) | Autoincremento 300 ms y parpadeo |
| `entrada.c/.h` (nombre libre) | Máquina de estados de introducción de jugada (F → número → C → número → parpadeo → jugada) |
| `perifericos.c/.h` (o en `main.c`) | `inicializar_perifericos()`: llama a todos los `*_init` en un único sitio |
| `teclado.c/.h` | Solo si hacemos el opcional |

`uint8_t`/`uint32_t`: comprobar si el toolchain trae `<stdint.h>`; si no, `typedef` en `eventos.h`.

---

## 3. Lo que sobra o hay que cambiar del código de la P1

### 3.1 Sobra (quitar o dejar desactivado)

> **Hecho el 2026-10-26:** calibración y medidas del timer2, versiones C de `patron_volteo`/`color_ficha`,
> `color_ficha_arm`, bloque `#if 0`, `PRACTICA.md` y `memoria.md`. El juego usa ya ARM + Thumb (compila y enlaza).
> **Pendiente** (se rehace en los Pasos 2, 5 y 6): `esperar_mov`/`ready`, `int_count`, `switch_leds`, nombre del proyecto en Eclipse.

- `main.c`: `calibrar_timer2`, `CALIBRAR_TIMER2` y `tiempos_calibracion` (eran del Paso 7 de la P1).
- `reversi8_2026.c`:
  - `tiempo_volteo_us` y el `timer2_empezar/parar` alrededor de `actualizar_tablero` (medida de la P1).
  - `esperar_mov()` con espera activa sobre `ready`, y las variables globales `fila`, `columna`, `ready`
    escritas desde el depurador: se sustituyen por la máquina de estados de entrada.
  - `color_ficha_arm` y `patron_volteo` C / `color_ficha` C como versiones activas: el guion pide partir
    de **la implementación más eficiente de la P1 sin optimización del compilador**, que según nuestra
    memoria es **`patron_volteo_arm` + `color_ficha_thumb`** (284 → 194 µs, 198 B). Dejar
    `#define PATRON_VOLTEO patron_volteo_arm` en `reversi8_2026.c` y `#define COLOR_FICHA color_ficha_thumb`
    en **`reversi8_arm.asm`** (es `patron_volteo_arm` quien llama a `color_ficha`). Después sobran
    `color_ficha_arm` y las versiones C de `patron_volteo`/`color_ficha`.
  - El bloque `#if 0` de `init_table`.
- `button.c`: `int_count` y el `D8Led_symbol` dentro de la RSI (era solo la demo).
- `timer.c/.h`: `switch_leds` (demo de la P1, y además está **definida en el `.h`**).
- `PRACTICA.md` (referencia de la P1) y `memoria.md` (memoria de la P1, ya está en PDF en la carpeta de documentación).
- `.project` se llama `P1_873042_960280` y `Placa/` tiene los binarios de la P1: renombrar el proyecto en Eclipse a `P2_873042_960280` si queremos que los nombres cuadren.

### 3.2 Fallos de E/S que el Paso 2 pide corregir (encontrados en el código actual)

| Dónde | Problema | Arreglo |
|-------|----------|---------|
| `timer.c` `timer_ISR`, `button.c` RSI e init | `rI_ISPC \|= ...`: `I_ISPC` es de solo escritura (ya lo corregimos en `timer2.c`) | `rI_ISPC = BIT_...` |
| `timer.c` `timer_init` | `rTCFG0 = 255`, `rTCFG1 = 0`, `rTCON = 0x2 / 0x09` machacan la configuración del **timer2** (y del resto) | Tocar solo los bits del timer0 con máscara, como en `timer2.c` |
| `button.c` `Eint4567_init` | `rPCONG = 0xffff` y `rPUPG = 0` reconfiguran todo el puerto G; `rEXTINT \|= 0x22222222` no limpia los bits antes de poner los nuevos | Máscaras sobre los bits 6-7 (y EINT1 si hay teclado) |
| `button.c`, `timer.c` init | Reescriben `rINTMOD`/`rINTCON` (ya lo hace `sys_init`) | Quitarlo o centralizarlo en la init única |
| `button.c` RSI | `switch (rEXTINTPND)` con igualdad: si están los dos bits a la vez no hace nada. Borra los 4 bits de `EXTINTPND` aunque solo atienda uno | Mirar bits con `&` y borrar solo el atendido |
| `timer.h` | `int switch_leds;` definido en la cabecera | Variable `static volatile` dentro del `.c` y función de acceso |
| `led.c` | Máscaras `0x5ff`/`0x3ff` dependen de que `PDATB` sea de 11 bits | `& ~(1 << 9)` / `& ~(1 << 10)` |
| Todas las variables que toca una RSI | No son `volatile` (salvo en `timer2.c`) | `static volatile` en el módulo de la RSI |

---

## 4. Diseño de las dos máquinas de estados

### 4.1 Antirrebotes (Paso 5) — vive en las RSI de EINT4567 y timer0

```
ESPERANDO --(EINT6/7)--> desactivar EINT4567 (rINTMSK), guardar botón, programar timer0 = trp --> RETARDO_TRP
RETARDO_TRP --(timer0)--> programar timer0 cada 50 ms --> ENCUESTA
ENCUESTA --(timer0, botón sigue a 0 en rPDATG)--> sigue en ENCUESTA (aviso "mantenido" para el autoincremento)
ENCUESTA --(timer0, botón a 1)--> programar timer0 = trd --> RETARDO_TRD
RETARDO_TRD --(timer0)--> limpiar EXTINTPND, reactivar EINT4567 --> ESPERANDO
```

- Cada transición hace `guardar_eventos(ID, timer2_leer())`.
- trp y trd: medirlos en placa **antes** de activar la máquina: RSI de botón que solo apila eventos y
  ver en la pila cuánto duran las ráfagas al pulsar y al soltar. Poner trp/trd con margen. Ese test se
  deja compilable con un `#define` para enseñarlo al profesor.
- El módulo de botones no expone variables: expone funciones (`boton_pulsado()`, o una notificación al
  módulo de entrada).

### 4.2 Entrada de la jugada (Paso 6)

```
INICIO (8led apagado) --cualquier botón--> init tablero y candidatas; 8led = F --> FILA
FILA / COLUMNA:
  izq pulsado      -> n = n % 8 + 1, mostrar n (primera pulsación muestra 1)
  izq > 0,5 s      -> timer1 cada 300 ms: n = n % 8 + 1   (para al soltar; el número queda fijo)
  der pulsado      -> confirmar n. Si era FILA: 8led = C --> COLUMNA. Si era COLUMNA --> CONFIRMAR
CONFIRMAR: led izquierdo parpadea 3 veces/s durante 3 s (timer1)
  cualquier botón  -> jugada cancelada --> FILA
  fin de los 3 s   -> jugada lista (fila-1, columna-1) para reversi8 --> FILA
```

- El juego usa índices 0-7; el 8led muestra 1-8.
- En la P1, fila o columna = 8 significaba "paso". Con 1-8 en el 8led no hay forma de pasar.
  **[pregunta al profesor]** si hace falta representarlo (p. ej. un noveno valor).
- `reversi8()` pasa de `esperar_mov(&ready)` a esperar a que el módulo de entrada le dé una jugada
  (función del módulo, no variable compartida). Esta espera está en el programa principal, no en una RSI,
  así que puede ser un bucle.

---

## 5. Paso a paso

**Fuera del laboratorio (antes de la primera sesión con placa)**

1. Leer `Entradasalida.pdf` (cap. 3 GPIO, 4 display, 5 temporizadores, 6 teclado) y `P2-EC.pdf`
   (modos, excepciones, mapa de memoria, RSI en C).
2. Limpieza de la P1 (§3.1): fuera calibración y medidas, `#define` a ARM + Thumb, quitar demos.
3. Arreglos de E/S del §3.2 (`static`, `volatile`, máscaras, `rI_ISPC =`).
4. En el emulador: poner un breakpoint en una RSI y en una función normal, abrir el desensamblado y
   apuntar las diferencias del prólogo/epílogo (registros que apila, `subs pc, lr, #4`, uso de la pila IRQ).
   **Esto va explicado en la memoria (marco de pila).**
5. `inicializar_perifericos()` y llamarla desde `Main` después de `sys_init()`.
6. `eventos.h` + `lista.c/.h` + `guardar_eventos`. Probarla en el emulador con llamadas a mano
   (comprobar que da la vuelta en el evento 151 y que no pisa nada).
7. Escribir las máquinas de estados del §4 (código y diagramas para la memoria).

**En el laboratorio**

8. Comprobar en placa qué EINT es el botón izquierdo y cuál el derecho.
9. Test de rebotes: RSI que solo apila eventos; pulsar y soltar varias veces y leer la pila de depuración.
   Apuntar trp y trd medidos (por placa, cambian entre placas).
10. Activar la máquina antirrebotes y comprobar en la pila que cada pulsación da un único evento al pulsar
    y uno al soltar.
11. Probar la entrada F/C: incremento, vuelta 8 → 1, autoincremento 0,5 s + 300 ms, confirmación,
    parpadeo 3 s y cancelación.
12. Jugar partidas completas contra la máquina.
13. (Opcional) Teclado matricial: RSI EINT1, escaneo, mapear las 10 teclas.

**Cierre**

14. Memoria (§6) y ZIP de entrega.

---

## 6. Memoria (Anexo I del guion)

Diseño libre, pero obligatorio:

1. **Resumen ejecutivo** (una cara como máximo, se lee como documento independiente).
2. **Gestión de la E/S**: diagramas de las máquinas de estados comentados, decisiones de diseño,
   pseudocódigo o fragmentos de código. Incluir también el **marco de pila de las RSI** (lo pide el Paso 2).
3. **Problemas encontrados y soluciones.**
4. **Anexo: código fuente comentado**, solo el nuestro, con cabecera en cada función.
5. **Conclusiones.**

Claro y conciso. Seguir `Notas_memoria_tecnica_2026_v1.pdf` (en la carpeta de prácticas).

---

## 7. Material de apoyo y para qué sirve

| Documento | Para qué |
|-----------|----------|
| `PH_practica2_2026.pdf` (raíz) | **Guion**: la fuente de verdad |
| `CUADERNOS_COMPLUTENSE_APOYO/Entradasalida.pdf` | GPIO, pulsadores, LEDs, 8led, timers, teclado matricial |
| `CUADERNOS_COMPLUTENSE_APOYO/P2-EC.pdf` | Modos del ARM, excepciones, pilas, mapa de memoria, RSI |
| `CUADERNOS_COMPLUTENSE_APOYO/GuiaEntorno.pdf` | Eclipse + openOCD (igual que en la P1) |
| `MATERIAL_COMPLEMENTARIO/um_s3c44box.pdf` | Manual del SoC: registros exactos (INTMSK, TCON, PCONG, EXTINT…) |
| `MATERIAL_COMPLEMENTARIO/S3CEV40_UserGuide.pdf` | Esquema de la placa |
| `MATERIAL_COMPLEMENTARIO/ARM-Interrupts.pdf`, `PH_practica2_2026.pdf` (36 págs.), `Tema7_AOC1.pdf` | Teoría de E/S, interrupciones y DMA |
| `MATERIAL_COMPLEMENTARIO/P1..P5-LEC.pdf`, `P3-EC.pdf`, `P4-LEC.pdf` | Cuadernos de laboratorio de la UCM (consulta) |
| `MEMORIA_P1_873042_960280.pdf` | Nuestra memoria de la P1 (tiempos de las versiones ARM/Thumb) |
