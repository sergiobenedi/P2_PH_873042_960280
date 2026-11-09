# PRACTICA 2 — Sistema de E/S y dispositivos basicos (Reversi 8 en placa)

> Resumen de trabajo sacado de `P2_DOCUMENTACION/` (`PH_practica2_2026.pdf` = guion,
> `Entradasalida.pdf`, `P2-EC.pdf`, manual `um_s3c44box.pdf`, `S3CEV40_UserGuide.pdf`, transparencias de
> E/S) y del codigo actual (P1). Lo que no viene en los documentos se marca como **[decision nuestra]**
> o **[verificar en placa]**.

**Entrega:** ~23 de noviembre de 2026 (fecha definitiva en Moodle).
ZIP unico por pareja: `p2_873042-Apellidos_960280-Apellidos.zip` con la memoria en PDF + codigo fuente.

---

## 1. Que pide la practica (en una linea por paso)

| Paso | Que hay que hacer | ¿Necesita placa? |
|------|-------------------|------------------|
| 1 | Estudiar `Entradasalida.pdf` y `P2-EC.pdf` en detalle | No |
| 2 | Entender el proyecto ejemplo (botones, timer0, 8led, leds). Ver el ensamblador que genera gcc para una RSI (`__attribute__((interrupt("IRQ")))`) y compararlo con una subrutina normal. Arreglar E/S: `static`, `volatile` y no machacar bits de otros dispositivos | Parcial (se puede ver el asm en el emulador) |
| 3 | Una funcion que inicialice **todos** los perifericos de forma centralizada. Las RSI **no comparten variables** con el programa principal | No (diseño) |
| 4 | Pila de depuracion circular de **150 eventos** (`lista.c/.h`, `eventos.h`, `guardar_eventos(uint8_t ID_evento, uint32_t instante)`), situada al final de la RAM, antes de las pilas de modo | No para escribirla, si para probarla |
| 5 | Medir rebotes de los pulsadores con la pila de depuracion y eliminarlos con una **maquina de estados con timer** (trp → encuesta cada 50 ms → trd). Dejar un **codigo de test** para enseñar los rebotes al profesor | **Si** |
| 6 | Jugar con botones + 8led: maquina de estados F/C, autoincremento con **timer1**, confirmacion con parpadeo del led izquierdo | **Si** para probar |
| Opc. | Introducir valores con el **teclado matricial** (10 teclas) | **Si** |
| Memoria | Ver §6 | No |

Regla dura del guion: **las RSI deben ser ligeras, nunca espera activa dentro de una IRQ**.

---

## 2. Lo que vamos a necesitar

### 2.1 Hardware (datos de `Entradasalida.pdf` y codigo)

| Dispositivo | Como se usa |
|-------------|-------------|
| Pulsadores | Pines 6 y 7 del puerto G → **EINT6 / EINT7** (linea `BIT_EINT4567`). Pulsado = **0** en `rPDATG` (pull-up activado con `rPUPG`). En `rEXTINTPND`: bit 2 (valor 4) = EINT6, bit 3 (valor 8) = EINT7. **Verificado en placa (2026-11-03):** izquierdo = EINT6 (`rPDATG` bit 6), derecho = EINT7 (bit 7) |
| LEDs | Bits 9 y 10 de `rPDATB`, activos a nivel bajo (`led.c`) |
| 8led | Escritura en `LED8ADDR` con la tabla de `8led.h` (ya tiene `F` = indice 15 y `C` = indice 12, y `blank`) |
| Timer0 | Maquina antirrebotes, de un solo disparo (`timer0_programar(ms)`): trp = 60 ms, encuesta 50 ms, trd = 30 ms (medidos en placa, ver §4.1) **[decision nuestra]** |
| Timer1 | Autoincremento cada 300 ms tras 0,5 s pulsado (**obligatorio timer1** segun el guion). Se reutiliza para el parpadeo de confirmacion (167 ms), que nunca coincide en el tiempo **[decision nuestra]**. `TEMPORIZADOR_ENTRADA` en `timer.h` permite cambiarlo por la interrupcion del timer2 (1 = timer1, 2 = timer2): **pendiente decidir** |
| Timer2 | Ya hecho en P1 (`timer2.c`): marca de tiempo en µs para `guardar_eventos` |
| Teclado (opc.) | EINT1, banco 3 (`0x0600_0000`). Escaneo leyendo las direcciones `0x0600_00FD/FB/F7/EF` (cero en una fila cada vez) y mirando D0-D3. Teclas habilitadas: figura 2 del guion |

Relojes: MCLK = 64 MHz. `f_timer = MCLK / (preescalado + 1) / divisor`. Timers 0-1 comparten preescalado
(`TCFG0[7:0]`), timers 2-3 el suyo (`TCFG0[15:8]`); cada uno su divisor en `TCFG1`.

### 2.2 Mapa de memoria y pilas (de `common/44binit.asm` y `option.h`)

- Programa a partir de `0x0C000000`. `_ISR_STARTADDRESS = 0x0C7FFF00` (tabla `pISR_*`).
- Pilas de modo (crecen hacia abajo desde estas cimas):
  User `0x0C7FF000` (no se inicializa), SVC `0x0C7FF100`, Undef `0x0C7FF200`, Abort `0x0C7FF300`,
  IRQ `0x0C7FF400`, FIQ `0x0C7FF500`. Cada una 256 B.
- **`Main` corre en modo SVC**: su pila parte de `0x0C7FF100` y baja por debajo de `0x0C7FF000` sin limite.
- Pila de depuracion: 150 eventos × 8 B (id + instante, alineado a palabra) = **1200 B (0x4B0)**.
  Propuesta **[decision nuestra]**: base en `0x0C7FE000` (deja ~4 KB a la pila SVC) y comprobar en el
  `.map` que el fin de `.bss` queda muy por debajo. Definirlo con un `#define` en `lista.h`, no en el linker.
- La pila IRQ solo tiene 256 B: las RSI que llamen a `guardar_eventos` → `timer2_leer` gastan pila; vigilarlo.

### 2.3 Ficheros nuevos

| Fichero | Contenido |
|---------|-----------|
| `eventos.h` | Un `enum` con un ID unico por evento. Propuesta: `EV_BOTON_IZQ`, `EV_BOTON_DER`, `EV_TIMER0`, `EV_TIMER1`, `EV_BOTON_SOLTADO`, `EV_FIN_TRD`, `EV_JUGADA_CONFIRMADA`, `EV_JUGADA_CANCELADA`, `EV_TECLADO`… |
| `lista.c/.h` | Lista circular de 150 entradas en direccion fija, `guardar_eventos(uint8_t, uint32_t)` y una inicializacion. Indice que da la vuelta al llegar a 150 |
| `button.c/.h` (ampliado) | RSI EINT4567 + maquina antirrebotes con timer0. `TEST_REBOTES` = modo de test que solo apila eventos |
| `timer.c/.h` (ampliado) | `timer0_programar` (un disparo) y temporizador de la entrada (`temporizador_entrada_*`: autoincremento 300 ms y parpadeo) |
| `entrada.c/.h` | Maquina de estados de introduccion de jugada (INICIO → F → numero → C → numero → parpadeo → jugada) |
| `inicializar_perifericos()` en `main.c` | Llama a todos los `*_init` en un unico sitio |
| `teclado.c/.h` | Solo si hacemos el opcional |

`uint8_t`/`uint32_t`: comprobar si el toolchain trae `<stdint.h>`; si no, `typedef` en `eventos.h`.

---

## 3. Lo que sobra o hay que cambiar del codigo de la P1

### 3.1 Sobra (quitar o dejar desactivado)

> **Hecho el 2026-10-26:** calibracion y medidas del timer2, versiones C de `patron_volteo`/`color_ficha`,
> `color_ficha_arm`, bloque `#if 0`, `PRACTICA.md` y `memoria.md`. El juego usa ya ARM + Thumb (compila y enlaza).
> **Hecho el 2026-10-27 (Paso 3):** `int_count`, `switch_leds` y todos los fallos de la tabla §3.2.
> **Hecho el 2026-11-03 (Paso 7):** `esperar_mov`/`ready` y las globales `fila`/`columna` sustituidos por `entrada.c` (`entrada_jugada_lista`).
> **Pendiente:** nombre del proyecto en Eclipse.

- `main.c`: `calibrar_timer2`, `CALIBRAR_TIMER2` y `tiempos_calibracion` (eran del Paso 7 de la P1).
- `reversi8_2026.c`:
  - `tiempo_volteo_us` y el `timer2_empezar/parar` alrededor de `actualizar_tablero` (medida de la P1).
  - `esperar_mov()` con espera activa sobre `ready`, y las variables globales `fila`, `columna`, `ready`
    escritas desde el depurador: se sustituyen por la maquina de estados de entrada.
  - `color_ficha_arm` y `patron_volteo` C / `color_ficha` C como versiones activas: el guion pide partir
    de **la implementacion mas eficiente de la P1 sin optimizacion del compilador**, que segun nuestra
    memoria es **`patron_volteo_arm` + `color_ficha_thumb`** (284 → 194 µs, 198 B). Dejar
    `#define PATRON_VOLTEO patron_volteo_arm` en `reversi8_2026.c` y `#define COLOR_FICHA color_ficha_thumb`
    en **`reversi8_arm.asm`** (es `patron_volteo_arm` quien llama a `color_ficha`). Despues sobran
    `color_ficha_arm` y las versiones C de `patron_volteo`/`color_ficha`.
  - El bloque `#if 0` de `init_table`.
- `button.c`: `int_count` y el `D8Led_symbol` dentro de la RSI (era solo la demo).
- `timer.c/.h`: `switch_leds` (demo de la P1, y ademas esta **definida en el `.h`**).
- `PRACTICA.md` (referencia de la P1) y `memoria.md` (memoria de la P1, ya esta en PDF en la carpeta de documentacion).
- `.project` se llama `P1_873042_960280` y `Placa/` tiene los binarios de la P1: renombrar el proyecto en Eclipse a `P2_873042_960280` si queremos que los nombres cuadren.

### 3.2 Fallos de E/S que el Paso 2 pide corregir (encontrados en el codigo actual)

> **Hecho el 2026-10-27 (commit `550cfe2`):** todos los de la tabla. De paso, quitados los avisos
> `array subscript has type 'char'` de `reversi8_2026.c` (indices `int` en `voltear` y `actualizar_candidatas`).
> `timer_init` y `Eint4567_init` estan arregladas pero **aun nadie las llama** (lo hara `inicializar_perifericos()`).

| Donde | Problema | Arreglo |
|-------|----------|---------|
| `timer.c` `timer_ISR`, `button.c` RSI e init | `rI_ISPC \|= ...`: `I_ISPC` es de solo escritura (ya lo corregimos en `timer2.c`) | `rI_ISPC = BIT_...` |
| `timer.c` `timer_init` | `rTCFG0 = 255`, `rTCFG1 = 0`, `rTCON = 0x2 / 0x09` machacan la configuracion del **timer2** (y del resto) | Tocar solo los bits del timer0 con mascara, como en `timer2.c` |
| `button.c` `Eint4567_init` | `rPCONG = 0xffff` y `rPUPG = 0` reconfiguran todo el puerto G; `rEXTINT \|= 0x22222222` no limpia los bits antes de poner los nuevos | Mascaras sobre los bits 6-7 (y EINT1 si hay teclado) |
| `button.c`, `timer.c` init | Reescriben `rINTMOD`/`rINTCON` (ya lo hace `sys_init`) | Quitarlo o centralizarlo en la init unica |
| `button.c` RSI | `switch (rEXTINTPND)` con igualdad: si estan los dos bits a la vez no hace nada. Borra los 4 bits de `EXTINTPND` aunque solo atienda uno | Mirar bits con `&` y borrar solo el atendido |
| `timer.h` | `int switch_leds;` definido en la cabecera | Variable `static volatile` dentro del `.c` y funcion de acceso |
| `led.c` | Mascaras `0x5ff`/`0x3ff` dependen de que `PDATB` sea de 11 bits | `& ~(1 << 9)` / `& ~(1 << 10)` |
| Todas las variables que toca una RSI | No son `volatile` (salvo en `timer2.c`) | `static volatile` en el modulo de la RSI |

---

## 4. Diseño de las dos maquinas de estados

### 4.1 Antirrebotes (Paso 5) — vive en las RSI de EINT4567 y timer0

> **Hecho y probado en placa (2026-11-03).** Rebotes medidos con `TEST_REBOTES = 1`: rafaga al pulsar de
> 40,3 ms (izq, 6 eventos) y 41,5 ms (der, 19 eventos); al soltar 12,4 ms (4 eventos) y 1,4 ms (2 eventos).
> **Placa 4 (2026-11-09)**, 4 pulsaciones de 1 s por boton + un toque rapido por boton (SB2 = izq, SB3 = der).
> Rafagas al pulsar (primer a ultimo evento): izq 0 / 0 / 22,8 / 22,4 ms (toque rapido 13,0 ms), der 0,5 / 28,5 /
> 93,7 ms (un caso suelto: el boton se vio a 1 durante 84 → 94 ms) (toque rapido sin rebotes). Al soltar: izq 1,6 /
> 26,6 / 18,3 / 23,9 ms (toque 4,3 ms), der 5,0 (23 eventos) / 38,2 / 20,7 ms. Toques rapidos reales de ~100-110 ms.
> Elegidos **trp = 60 ms, trd = 50 ms** (`button.c`): trd sube de 30 a 50 para cubrir los 38,2 ms al soltar de la
> placa 4 (con 30 ms un rebote tras reactivar EINT podria dar una pulsacion falsa). trp no mayor: se han visto
> pulsaciones reales de 36,7 ms (placa 12) que se descartarian como espurias, y un trp de ~90 ms caeria en el
> hueco del caso de 94 ms. Las ráfagas de pulsar y soltar cambian de una placa a otra: se dejan valores que valen
> para las dos. **Provisional**: pocas muestras.

```
ESPERANDO --(EINT6/7)--> desactivar EINT4567 (rINTMSK), guardar boton, programar timer0 = trp --> RETARDO_TRP
RETARDO_TRP --(timer0, boton a 0 en rPDATG)--> avisar "pulsado", programar timer0 = 50 ms --> ENCUESTA
RETARDO_TRP --(timer0, boton a 1: pulso espurio)--> sin aviso, programar timer0 = trd --> RETARDO_TRD
ENCUESTA --(timer0, boton sigue a 0)--> reprogramar timer0 = 50 ms, sigue en ENCUESTA
ENCUESTA --(timer0, boton a 1)--> avisar "soltado", programar timer0 = trd --> RETARDO_TRD
RETARDO_TRD --(timer0)--> limpiar EXTINTPND y rI_ISPC, reactivar EINT4567 --> ESPERANDO
```

- Cada transicion hace `guardar_eventos(ID, timer2_leer())`.
- trp y trd: medirlos en placa **antes** de activar la maquina: RSI de boton que solo apila eventos y
  ver en la pila cuanto duran las rafagas al pulsar y al soltar. Poner trp/trd con margen. Ese test se
  deja compilable con un `#define` para enseñarlo al profesor.
- El modulo de botones no expone variables: expone funciones (`boton_pulsado()`, o una notificacion al
  modulo de entrada).

### 4.2 Entrada de la jugada (Paso 6)

> **Hecho y probado en placa (2026-11-03)** en `entrada.c`: secuencia completa y cancelacion.
> Jugada fila 3 / columna 5 → `tablero[2][4] = 2` y respuesta de la maquina. Ojo: aun no comprobamos
> que la jugada del usuario sea legal (como en la P1).

```
INICIO (8led apagado) --cualquier boton--> 8led = F --> FILA
FILA / COLUMNA:
  izq pulsado      -> n = n % 8 + 1, mostrar n (primera pulsacion muestra 1)
  izq > 0,5 s      -> timer1 cada 300 ms: n = n % 8 + 1   (para al soltar; el numero queda fijo)
  der pulsado      -> confirmar n (se ignora si n = 0). Si era FILA: 8led = C --> COLUMNA. Si era COLUMNA --> CONFIRMAR
CONFIRMAR: led izquierdo parpadea 3 veces/s durante 3 s (timer1: 18 conmutaciones de 167 ms)
  cualquier boton  -> jugada cancelada (EV_JUGADA_CANCELADA) --> FILA
  fin de los 3 s   -> jugada lista (fila-1, columna-1) para reversi8 (EV_JUGADA_CONFIRMADA) --> FILA
```

- El tablero y las candidatas se inicializan una vez al empezar `reversi8()`, no en INICIO.
- El led que parpadea es `led1`, que es el izquierdo (verificado en placa el 2026-11-09).

- El juego usa indices 0-7; el 8led muestra 1-8.
- En la P1, fila o columna = 8 significaba "paso". Con 1-8 en el 8led no hay forma de pasar.
  **[pregunta al profesor]** si hace falta representarlo (p. ej. un noveno valor).
  Por ahora, sin paso: la entrada solo da 0-7 y `fin` no llega a valer 1.
- `reversi8()` pasa de `esperar_mov(&ready)` a esperar a que el modulo de entrada le de una jugada
  (funcion del modulo, no variable compartida). Esta espera esta en el programa principal, no en una RSI,
  asi que puede ser un bucle.

---

## 5. Paso a paso

Estado: ✅ hecho · ⬜ pendiente. Detalle de cada sesion en [MEMORIA_NOTAS.md](MEMORIA_NOTAS.md).

**Fuera del laboratorio (antes de la primera sesion con placa)**

1. ✅ Leer `Entradasalida.pdf` (cap. 3 GPIO, 4 display, 5 temporizadores, 6 teclado) y `P2-EC.pdf`
   (modos, excepciones, mapa de memoria, RSI en C).
2. ✅ Limpieza de la P1 (§3.1): fuera calibracion y medidas, `#define` a ARM + Thumb, quitar demos. *(2026-10-26 y 27)*
3. ✅ Arreglos de E/S del §3.2 (`static`, `volatile`, mascaras, `rI_ISPC =`). *(2026-10-27, `550cfe2`)*
4. ✅ En el emulador: poner un breakpoint en una RSI y en una funcion normal, abrir el desensamblado y
   apuntar las diferencias del prologo/epilogo (registros que apila, `subs pc, lr, #4`, uso de la pila IRQ).
   **Esto va explicado en la memoria (marco de pila).** *(2026-10-27, ver MEMORIA_NOTAS.md)*
5. ✅ `inicializar_perifericos()` y llamarla desde `Main` despues de `sys_init()`. *(2026-10-27, probado en placa)*
6. ✅ `eventos.h` + `lista.c/.h` + `guardar_eventos`. Probarla en el emulador con llamadas a mano
   (comprobar que da la vuelta en el evento 151 y que no pisa nada). *(2026-11-02, `pruebas/lista_sim.gdb`)*
7. ✅ Escribir las maquinas de estados del §4 (codigo y diagramas para la memoria). *(2026-11-03, `button.c`, `timer.c`, `entrada.c`; diagramas en §4, falta dibujarlos para la memoria)*

**En el laboratorio**

8. ✅ Comprobar en placa que EINT es el boton izquierdo y cual el derecho. *(izq = EINT6, der = EINT7)*
9. ✅ Test de rebotes: RSI que solo apila eventos; pulsar y soltar varias veces y leer la pila de depuracion.
   Apuntar trp y trd medidos (por placa, cambian entre placas). *(trp 60 ms, trd 30 ms; repetir con mas muestras)*
10. ✅ Activar la maquina antirrebotes y comprobar en la pila que cada pulsacion da un unico evento al pulsar
    y uno al soltar. *(2026-11-03; repetido en la placa 4 el 2026-11-09 con trp 60 / trd 50: 10 pulsaciones reales = 10 avisos de pulsado y 10 de soltado, y un toque espurio descartado sin aviso)*
11. ✅ Probar la entrada F/C: incremento, vuelta 8 → 1, autoincremento 0,5 s + 300 ms, confirmacion,
    parpadeo 3 s y cancelacion. *(2026-11-03)*
12. ✅ Jugar una partida completa contra la maquina. *(2026-11-09: 30 jugadas hasta llenar el tablero, 31 blancas / 33 negras, comprobado con `pruebas/contar.gdb`. Corregido `char` sin signo en `voltear`. Sin "paso" no hay final de partida: pendiente del profesor)*

**Pendiente de lo ya hecho** (detalle en MEMORIA_NOTAS.md)

- (Hecho 2026-11-09, placa 4) Pila de depuracion con los ids 7 y 8: ver `pruebas/rebotes_placa4/` (capturas y volcados para la memoria).
- Mas muestras de rebotes (~10 por boton) para confirmar trp/trd.
- Valorar exigir dos lecturas seguidas de "soltado" en ENCUESTA (glitch de contacto a mitad de pulsacion).
- Decidir timer1 o timer2 para la entrada (`TEMPORIZADOR_ENTRADA`).
- Preguntar al profesor por el "paso" (hoy no se puede pasar turno).
- Medir el uso real de la pila IRQ en placa (rellenar con `0xDEADBEEF`; estimado ~130 B de 256 B).
13. ⬜ (Opcional) Teclado matricial: RSI EINT1, escaneo, mapear las 10 teclas.

**Cierre**

14. ⬜ Memoria (§6) y ZIP de entrega.

---

## 6. Memoria (Anexo I del guion)

Diseño libre, pero obligatorio:

1. **Resumen ejecutivo** (una cara como maximo, se lee como documento independiente).
2. **Gestion de la E/S**: diagramas de las maquinas de estados comentados, decisiones de diseño,
   pseudocodigo o fragmentos de codigo. Incluir tambien el **marco de pila de las RSI** (lo pide el Paso 2).
3. **Problemas encontrados y soluciones.**
4. ⬜ **Anexo: codigo fuente comentado**, solo el nuestro, con cabecera en cada funcion.
5. ⬜ **Conclusiones.**

Claro y conciso. Seguir `Notas_memoria_tecnica_2026_v1.pdf` (en la carpeta de practicas).

---

## 7. Material de apoyo y para que sirve

| Documento | Para que |
|-----------|----------|
| `PH_practica2_2026.pdf` (raiz) | **Guion**: la fuente de verdad |
| `CUADERNOS_COMPLUTENSE_APOYO/Entradasalida.pdf` | GPIO, pulsadores, LEDs, 8led, timers, teclado matricial |
| `CUADERNOS_COMPLUTENSE_APOYO/P2-EC.pdf` | Modos del ARM, excepciones, pilas, mapa de memoria, RSI |
| `CUADERNOS_COMPLUTENSE_APOYO/GuiaEntorno.pdf` | Eclipse + openOCD (igual que en la P1) |
| `MATERIAL_COMPLEMENTARIO/um_s3c44box.pdf` | Manual del SoC: registros exactos (INTMSK, TCON, PCONG, EXTINT…) |
| `MATERIAL_COMPLEMENTARIO/S3CEV40_UserGuide.pdf` | Esquema de la placa |
| `MATERIAL_COMPLEMENTARIO/ARM-Interrupts.pdf`, `PH_practica2_2026.pdf` (36 pags.), `Tema7_AOC1.pdf` | Teoria de E/S, interrupciones y DMA |
| `MATERIAL_COMPLEMENTARIO/P1..P5-LEC.pdf`, `P3-EC.pdf`, `P4-LEC.pdf` | Cuadernos de laboratorio de la UCM (consulta) |
| `MEMORIA_P1_873042_960280.pdf` | Nuestra memoria de la P1 (tiempos de las versiones ARM/Thumb) |
