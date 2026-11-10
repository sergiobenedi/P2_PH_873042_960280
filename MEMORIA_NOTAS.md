# Notas para la memoria — Practica 2 (Sistema de E/S y dispositivos basicos)

Autores: Sergio Benedi (NIP 873042), Pablo Gimeno (NIP 960280)
Entrega: ~23 de noviembre de 2026 (confirmar en Moodle).
ZIP: `p2_873042-Apellidos_960280-Apellidos.zip` (memoria en PDF + codigo fuente).
Plan de la practica: [PRACTICA2.md](PRACTICA2.md).

> Este fichero NO es la memoria. Es el cuaderno de sesiones: cada vez que se pida "apuntalo",
> se añade una entrada nueva al final de §2 con la plantilla de §3 y una fila en §1.
> Etiquetas = apartado de la memoria (Anexo I del guion) donde se usara cada nota:
> `[RES]` Resumen ejecutivo · `[ES]` Gestion de la E/S y maquinas de estados ·
> `[PILA]` Marco de pila de las RSI · `[MED]` Medidas (rebotes, trp, trd, tiempos) ·
> `[PROB]` Problemas y soluciones · `[COD]` Codigo fuente · `[CONC]` Conclusiones.

---

## 1. Registro de horas

| Fecha | Quien | Horas | Pasos | Que |
|-------|-------|-------|-------|-----|
| 2026-10-27 | Sergio y Pablo | 2 (9:00–11:00) | 3, 4, 5 | Arreglos de E/S y avisos de compilacion; configuraciones de depuracion y marco de pila RSI vs funcion normal; `inicializar_perifericos()` probada en placa |
| 2026-11-02 | Sergio y Pablo | 2 (9:00–11:00) | 6 | Pila de depuracion (`eventos.h`, `lista.c/.h`) probada en el simulador |
| 2026-11-03 | Sergio y Pablo | 2 (9:00–11:00) | 7-11 | Maquinas de estados disenadas y dibujadas; antirrebotes, temporizador de la entrada y `entrada.c` en codigo; rebotes medidos en placa (`TEST_REBOTES`); entrada de jugada probada en la placa (numeros, autoincremento, confirmacion y cancelacion) |
| 2026-11-09 | Sergio | 2 (9:00–11:00) | 9-12 | Segunda placa: rebotes medidos otra vez, `trd` de 30 a 50 ms, antirrebotes comprobado, pila de depuracion con los ids 7 y 8, jugada comprobada en `tablero` y led izquierdo confirmado; partida de 30 jugadas (31 blancas / 33 negras) y corregido el `char` sin signo en `voltear` |
| 2026-11-10 | Sergio y Pablo | 1,5 (9:00–10:30) | 9, 12 | Final de partida con tablero lleno (contador `vacias`, E en el 8led) probado en placa; placa 7 recalibrada: rebotes medidos, trp y trd a 100 ms |
---

## 2. Sesiones

<!-- Añadir aqui cada sesion con la plantilla de §3, la mas reciente al final. -->

### 2026-10-27 — Arreglos de E/S (Paso 3)

**Quien / donde:** Sergio y Pablo · casa (sin placa, solo compilacion en Eclipse)

**Hecho:**
- Corregidos los fallos de E/S de la tabla §3.2 de PRACTICA2.md.
- Quitadas las demos de la P1: `switch_leds` (timer) e `int_count` + `D8Led_symbol` (RSI de botones).
- Quitados los avisos `array subscript has type 'char'` de `reversi8_2026.c`.
- Workspace de Eclipse limpio: cerrados P1/PruebaSim, borrados marcadores `make: ***` viejos. Compila sin errores ni avisos.
- Commit `550cfe2` subido a `develop`.

**Ficheros tocados:** `timer.c/.h` (I_ISPC, mascaras timer0, fuera `switch_leds`), `button.c` (RSI por bits, mascaras puerto G/EXTINT), `led.c` (`volatile`, mascaras por bit), `reversi8_2026.c` (indices `int`).

**Decisiones de diseño** `[ES]`:
- `rINTMOD`/`rINTCON` solo en `sys_init`; cada `*_init` solo desenmascara su linea en `rINTMSK`.
- Cada init toca solo sus bits (`(reg & ~mascara) | valor`) para no machacar otros perifericos (p. ej. timer2, que da las marcas de tiempo).
- La RSI de botones mira cada bit de `EXTINTPND` con `&` y borra solo los atendidos (pueden llegar EINT6 y EINT7 a la vez).
- En `Eint4567_init` se borran los pendientes **antes** de desenmascarar, para no atender una interrupcion espuria nada mas arrancar.

**Problemas y solucion** `[PROB]`:
- Problema: `rI_ISPC |= BIT_x` lee un registro de solo escritura → Solucion: `rI_ISPC = BIT_x`.
- Problema: `timer_init` ponia `rTCFG0 = 255`, `rTCFG1 = 0`, `rTCON = ...` y machacaba timer2 → Solucion: mascaras de los bits del timer0.
- Problema: `rPCONG = 0xffff`, `rPUPG = 0` y `rEXTINT |= 0x22222222` reconfiguraban todo el puerto G → Solucion: solo pines 6-7 (`PCONG` bits 15:12, `PUPG` bits 6-7, `EXTINT` bits 31:24).
- Problema: `int switch_leds;` definida en `timer.h` → Solucion: eliminada (era de la demo).
- Problema: Problems de Eclipse mezclaba errores de otros proyectos del workspace → Solucion: cerrarlos y borrar los marcadores viejos.

**Para la memoria:**
- `[PROB]` I_ISPC es de solo escritura: con `|=` se escribe lo leido (indefinido) y se pueden borrar peticiones de otras lineas.
- `[ES]` Inicializacion respetando los bits de otros dispositivos: patron lectura-mascara-escritura.

**Pendiente para la proxima sesion:**
- [ ] Paso 4: en el emulador, comparar el prologo/epilogo de una RSI con una funcion normal (marco de pila `[PILA]`).
- [ ] Paso 5: `inicializar_perifericos()` llamando a `timer_init` y `Eint4567_init` (aun no se llaman).

### 2026-10-27 — Marco de pila: RSI frente a funcion normal (Paso 4)

**Quien / donde:** Sergio y Pablo · casa (simulador: `arm-none-eabi-gdb`, `target sim`)

**Hecho:**
- Configuraciones de depuracion de la P2 copiadas de la P1: `SIM_P2_873042_960280.launch` (config. Debug, `target sim`) y `HW_P2_873042_960280.launch` (config. Placa, JTAG via openOCD en `localhost:3333`).
- Desensamblado de `Eint4567_ISR` y `led1_on` en la vista Disassembly de Eclipse (capturas hechas).
- Marco de pila real de `init_table` (funcion normal) y de `Eint4567_ISR` (entrada en IRQ forzada) leido con GDB en el simulador.

**Ficheros tocados:** `SIM_P2_873042_960280.launch`, `HW_P2_873042_960280.launch` (nuevos).

**Datos / medidas** `[PILA]`:
- Compilacion `-O0 -mapcs-frame`.
- **Funcion normal `init_table`** (modo SVC, CPSR `0x13`). Prologo `mov ip,sp` / `push {fp,ip,lr,pc}` / `sub fp,ip,#4` / `sub sp,sp,#16`. `sp` 0x3EA0 → 0x3E90 (16 B), `fp` = 0x3E9C:
  ```
  0x3E90: 0x00003FFC  fp anterior
  0x3E94: 0x00003EA0  ip = sp de entrada
  0x3E98: 0x0C001FDC  lr = vuelta a reversi8
  0x3E9C: 0x0C000F70  pc = dir. del push + 8   <- fp
  ```
  Retorno: `sub sp,fp,#12` / `ldm sp,{fp,sp,lr}` / `bx lr`.
- **RSI `Eint4567_ISR`** (forzada: CPSR = `0xD2`, `sp_irq` = `0x0C7FF400`, `lr` = instruccion interrumpida + 4; r2/r3/r11/r12 con valores marcados). Prologo `push {ip}` / `mov ip,sp` / `push {r2,r3,fp,ip,lr,pc}` / `sub fp,ip,#4` / `sub sp,sp,#8`. Total **36 B** de pila IRQ:
  ```
  0x0C7FF3FC: 0xCCCCCCCC  r12 original      <- push {ip}
  0x0C7FF3F8: 0x0C000C1C  pc                <- fp
  0x0C7FF3F4: 0x0C000F68  lr_irq
  0x0C7FF3F0: 0x0C7FF3FC  ip
  0x0C7FF3EC: 0xBBBBBBBB  fp anterior
  0x0C7FF3E8: 0x33333333  r3
  0x0C7FF3E4: 0x22222222  r2
  0x0C7FF3E0: 0x00000000  pnd (local)
  0x0C7FF3DC:                               <- sp
  ```
  Epilogo `sub sp,fp,#20` / `ldm sp,{r2,r3,fp,sp,lr}` / `pop {ip}` / `subs pc,lr,#4`: r2, r3, r11, r12 y `sp` (0x0C7FF400) vuelven a sus valores; `pc` = `lr − 4`; CPSR ← SPSR_irq.

**Decisiones de diseño** `[ES]`:
- `.launch` guardados dentro del proyecto (ficheros compartidos) para tenerlos los dos y no depender del `.metadata` del workspace.

**Problemas y solucion** `[PROB]`:
- Problema: el breakpoint en `main.c` no saltaba en el simulador → Causa: `emulador/init_b_2026.asm` pone `sp = 0x4000` y salta directo a `reversi8`; `Main`, `sys_init` y `timer2_inicializar` no se ejecutan en el simulador → Solucion: usar como funcion normal una llamada desde `reversi8` (`init_table`).
- Problema: la RSI no salta en el simulador (no hay perifericos) → Solucion: forzar la entrada desde GDB (`set $cpsr = 0xd2`, `$sp`, `$lr`, `$pc = Eint4567_ISR`). En la placa se vera saltar de verdad.
- Al forzar la entrada el hardware no guarda CPSR en SPSR_irq, asi que `subs pc,lr,#4` dejo CPSR = 0 (basura): demuestra que el retorno restaura el CPSR desde el SPSR.

**Para la memoria:**
- `[PILA]` La RSI salva ademas r2, r3 y r12 (scratch segun AAPCS): la interrupcion llega en cualquier instruccion y nadie los ha guardado por ella. La funcion normal no los salva porque el llamador asume que se pierden.
- `[PILA]` `sp` y `lr` son bancados: al pasar a modo IRQ se ven otros `sp`/`lr` (en el simulador valian 0).
- `[PILA]` Retorno: `bx lr` frente a `subs pc,lr,#4` (−4 porque `lr_irq` = instruccion interrumpida + 4 por el pipeline; la `s` con destino `pc` copia SPSR → CPSR).
- `[PILA]` El `pc` apilado vale la direccion del `push` + 8 (pipeline de 3 etapas).
- `[PILA]` La RSI no salva el SPSR: no admite interrupciones anidadas.
- `[PILA]` Hoy la RSI es hoja (36 B). Al llamar a `guardar_eventos`/`timer2_leer` (Paso 5) salvara r0–r3, r12 y lr y cada llamada añade marco: vigilar los 256 B de la pila IRQ.
- `[COD]` Las "instrucciones" raras tras el `subs`/`bx` del desensamblado son el *literal pool* (direcciones de `EXTINTPND`, `I_ISPC`, `led_state`) leido como codigo.

**Pendiente para la proxima sesion:**
- [ ] Paso 5: `inicializar_perifericos()` y llamarla desde `Main`.
- [ ] En el laboratorio: cambiar la ruta de GDB en `HW_P2` a `C:\Program Files\EclipseARM\...` y ver saltar el breakpoint de la RSI de verdad.

### 2026-10-27 — Inicializacion centralizada de perifericos (Paso 5)

**Quien / donde:** Sergio y Pablo · placa

**Hecho:**
- `inicializar_perifericos()` (static en `main.c`), llamada desde `Main` justo despues de `sys_init()`.
- Probado en placa: al arrancar los dos LEDs quedan apagados y el 8led muestra 0.

**Ficheros tocados:** `main.c` (nueva funcion, `#include` de `timer.h`, `button.h`, `led.h`, `8led.h`).

**Decisiones de diseño** `[ES]`:
- En `main.c` y no en un `perifericos.c` propio: es una sola funcion corta.
- Orden: `sys_init()` (INTMOD, INTCON, puertos, BIT_GLOBAL) → `timer2_inicializar()` (marca de tiempo de la pila de depuracion, lo primero) → `leds_off()` → `D8Led_init()` → `timer_init()` (timer0) → `Eint4567_init()` (lo ultimo: al desenmascarar EINT4567 ya pueden llegar IRQ y todo lo demas debe estar listo).
- Cada `*_init` solo desenmascara su linea en `rINTMSK` y toca sus bits; la configuracion global del controlador esta solo en `sys_init()`.

**Problemas y solucion** `[PROB]`:
- `main.c` no se puede probar en el simulador (no ejecuta `Main`) → probado directamente en placa.

**Para la memoria:**
- `[ES]` Una unica funcion de inicializacion; orden pensado para que no llegue ninguna IRQ antes de que su RSI y los recursos que usa esten preparados.

**Pendiente para la proxima sesion:**
- [ ] El timer0 queda en marcha con auto-reload (IRQ cada ~0,52 s con RSI vacia): en la maquina antirrebotes debe arrancarse solo al pulsar.
- [ ] El guion pide el 8led apagado al inicio: cambiar `cero` por `blank` al montar la maquina de entrada (Paso 6 del guion).
- [ ] Paso 6 (nuestro): `eventos.h` + `lista.c/.h` + `guardar_eventos`.

### 2026-11-02 — Pila de depuracion (Paso 6)

**Quien / donde:** Sergio y Pablo · casa (simulador)

**Hecho:**
- `eventos.h`: un `enum` con un ID por evento (`EV_NINGUNO` = 0 para hueco vacio, `EV_BOTON_IZQ/DER`, `EV_BOTON_SOLTADO`, `EV_TIMER0`, `EV_FIN_TRD`, `EV_TIMER1`, `EV_JUGADA_CONFIRMADA/CANCELADA`, `EV_TECLADO`, `EV_PRUEBA`).
- `lista.c/.h`: lista circular de 150 eventos en direccion fija, `lista_inicializar()` y `guardar_eventos(uint8_t ID_evento, uint32_t instante)`.
- `main.c`: `inicializar_perifericos()` llama a `lista_inicializar()` y, tras `timer2_inicializar()`, a `timer2_empezar()` (sin esto el timer2 queda parado y todos los instantes valdrian 0).
- Prueba en el simulador con `pruebas/lista_sim.gdb` (comando de terminal) y a mano desde la consola de GDB de Eclipse con la vista Memory en `0x0C7FE000`.

**Ficheros tocados:** `eventos.h`, `lista.c`, `lista.h`, `pruebas/lista_sim.gdb` (nuevos), `main.c`, `CLAUDE.md`/`README.md` (estructura).

**Datos / medidas** `[MED]`:
- Zona de la pila: `0x0C7FE000`–`0x0C7FE4AF` (150 × 8 B = 1200 B = 0x4B0). Fin de `.bss` en `0x0C0027FC`; deja ~3 KB (0xC50) a la pila SVC, cuya cima esta en `0x0C7FF100`.
- Prueba (zona rellena antes con `0xDEADBEEF`):
  - Tras `lista_inicializar()`: `0x0C7FE000`–`0x0C7FE4AF` a 0; `0x0C7FE4B0` sigue `0xDEADBEEF`.
  - Tras 150 eventos (`id` = 10, `instante` = n·1000): `indice` = 0; entrada 0 = `{0xA, 1000}`, entrada 149 = `{0xA, 150000}`.
  - Evento 151: `indice` = 1; entrada 0 = `{0xB, 151000}`, entrada 1 sigue `{0xA, 2000}`; `0x0C7FE4B0` sigue `0xDEADBEEF`.

**Decisiones de diseño** `[ES]`:
- Direccion fija con `#define PILA_DEP_BASE` en `lista.h` (no en el linker): siempre en el mismo sitio de la vista Memory.
- Entrada de 8 B (`id` en una palabra completa + `instante`): alineada a palabra y facil de leer en Memory como pares `id | instante`.
- `indice` (siguiente hueco) `static volatile` dentro de `lista.c`: no se comparte con nadie, se accede por funciones. El ultimo evento esta en `indice − 1`.
- El instante lo pasa quien llama (`timer2_leer()`), asi `lista.c` no depende del timer.
- Sin exclusion mutua en `guardar_eventos`: correcto mientras solo la llamen RSI (no se anidan). Si la llama `Main`, deshabilitar IRQ alrededor.

**Problemas y solucion** `[PROB]`:
- Problema: el timer2 se quedaba parado tras `timer2_inicializar()` → Solucion: `timer2_empezar()` en la inicializacion (instante 0 = arranque).
- Problema: la prueba con `call` en GDB no ejecutaba nada cuando la funcion estaba en la direccion de entrada del programa (GDB pone ahi el retorno de los `call`) → Solucion: llamar desde el `.elf` del proyecto, parado en `esperar_mov`.
- Problema: Eclipse no mostraba la consola de GDB → Solucion: en la vista Debug seleccionar la linea `arm-none-eabi-gdb` (o elegirla en el ▼ de la vista Console).

**Para la memoria:**
- `[ES]` Estructura de la pila de depuracion: direccion, tamaño, formato de entrada, indice circular y donde esta el ultimo evento.
- `[MED]` Tabla de la prueba de la vuelta (evento 151) con la zona marcada con `0xDEADBEEF`.

**Pendiente para la proxima sesion:**
- [ ] Paso 7: maquinas de estados del §4 de PRACTICA2.md (antirrebotes y entrada de jugada): codigo y diagramas.

### 2026-11-03 — Maquinas de estados, antirrebotes y medida de rebotes (Pasos 7 y 9)

**Quien / donde:** Sergio y Pablo · laboratorio (placa nº 12)

**Hecho:**
- Disenadas y dibujadas las dos maquinas de estados y contrastadas con las diapositivas del guion (32: pasos de la gestion de rebotes; 11-13: entrada de la jugada).
  - Antirrebotes: `ESPERANDO → RETARDO_TRP → ENCUESTA → RETARDO_TRD → ESPERANDO`, transiciones T1-T5 (T2 se bifurca en T2a/T2b, ver abajo).
  - Entrada de jugada: `INICIO → FILA → COLUMNA → CONFIRMAR → FILA`, transiciones E1-E4.
- Codigo del antirrebotes (`button.c`), timer0 de un solo disparo y temporizador de la entrada (`timer.c`, con la interrupcion del timer2 como alternativa en `timer2.c`), `entrada.h` con las funciones que llaman las RSI.
- Modo de test `TEST_REBOTES` (`button.h`): la RSI solo apila eventos, con flancos de bajada y de subida. Usado en la placa para medir los rebotes.
- Escrito `entrada.c` (maquina de la entrada de jugada) y adaptado `reversi8()` (`while (!entrada_jugada_lista(&fila, &columna))` en lugar de `esperar_mov`/`ready`/`fila`/`columna`). Compila y enlaza con timer1 y con timer2; SIN PROBAR en la placa. `TEST_REBOTES` vuelto a 0.

**Ficheros tocados:** `button.c/.h` (maquina antirrebotes, `TEST_REBOTES`), `timer.c/.h` (`timer0_programar`, `temporizador_entrada_*`, `TEMPORIZADOR_ENTRADA`), `timer2.c/.h` (`timer2_aviso_*`), `entrada.h` (nuevo), `CLAUDE.md`/`README.md` (estructura).

**Datos / medidas** `[MED]`: (placa, un boton por toma; ids de la pila: 1 = izquierdo, 2 = derecho, 3 = soltado)
- Boton izquierdo = EINT6 (SB2), derecho = EINT7 (SB3), como se suponia.
- Rafaga de pulsar: 40,3 ms (izquierdo, 6 eventos) y 41,5 ms (derecho, 19 eventos). Las demas pulsaciones: un solo flanco.
- Rafaga de soltar: 12,4 ms (derecho, 4 eventos), 1,4 ms (derecho, 2 eventos). Izquierdo: un solo flanco.
- Entre flancos de una misma rafaga: desde 38 µs hasta 21 ms. Un evento suelto a mitad de una pulsacion (~0,57 s despues de la rafaga): perdida breve de contacto con el dedo.
- Valores puestos: `TRP_MS` = 60 ms (peor caso 41,5 ms) y `TRD_MS` = 30 ms (peor caso 12,4 ms). Pocas muestras: provisionales.

**Decisiones de diseño** `[ES]`:
- Un solo antirrebotes para los dos botones (la linea EINT4567 es compartida): mientras uno esta en la maquina, el otro es invisible. Limitacion aceptada.
- La RSI enmascara EINT4567 al pulsar y al final (T5) borra `EXTINTPND`, luego `I_ISPC`, y solo despues desenmascara: asi un rebote de la suelta no dispara una IRQ falsa.
- T2 lee el pin al acabar trp (diapositiva 32, «comprobar si sigue pulsado»): a 0 → T2a, se confirma y se avisa a la entrada; a 1 → T2b, pulso espurio sin aviso. Ya no se avisa en T1.
- timer0 en un solo disparo, reprogramado en cada estado (solo sus bits de `TCON`).
- Los 0,5 s de mantenido los mide la entrada con su temporizador, no la encuesta: la encuesta solo detecta la suelta.
- `TEMPORIZADOR_ENTRADA`: 1 = timer1, 2 = interrupcion del timer2 (tic de 2,048 ms). La diapositiva 12 dice «temporizador 2» y el guion pide el timer1: se deja elegible.
- trp mas largo descarta las pulsaciones mas cortas que el (T2b): por eso 60 ms y no 100 ms (se vio una pulsacion real de 36,7 ms).
- Sin «paso» por ahora: con 1-8 en el 8led no se puede representar fila/columna = 8. Pendiente de preguntar al profesor.

**Problemas y solucion** `[PROB]`:
- Problema: el build normal no enlazaba (`undefined reference` a `entrada_*`) → Solucion: faltaba `entrada.c`; con `TEST_REBOTES = 1` esas llamadas no se compilan y enlaza.
- Problema: la placa quedo en modo Abort con el PC en una direccion basura y el programa no llegaba a `esperar_mov`, ni siquiera sin pulsar → Solucion: apagar y encender la placa y repetir la carga; paso a paso desde `Main` llego a `esperar_mov`. Causa exacta no determinada, no se repitio tras el reinicio limpio.

**Para la memoria:**
- `[ES]` Diagramas de las dos maquinas de estados y tabla de transiciones (T1-T5, E1-E4).
- `[MED]` Tabla de rafagas de rebote por boton y como se sacaron trp y trd.
- `[PROB]` Placa en modo Abort tras un fallo: apagar y encender antes de relanzar la depuracion.

**Pendiente para la proxima sesion:**
- [x] Probar `entrada.c` en la placa (pasos 8-11 del guion): 8led apagado al inicio, F, numeros 1-8, autoincremento, C, parpadeo del led y cancelacion. (Hecho, ver la sesion siguiente.)
- [ ] Medir la pila IRQ en la placa (rellenar con `0xDEADBEEF` y ver hasta donde se pisa); estimacion por el ensamblador: ~130 B de 256.
- [ ] Mas muestras de rebotes (unas 10 pulsaciones por boton) para confirmar trp y trd.
- [ ] Decidir si la suelta exige dos lecturas seguidas a 1 (perdida de contacto a mitad de pulsacion).
- [ ] Decidir el temporizador de la entrada (timer1 o timer2) y preguntar lo del «paso» al profesor.
- [ ] Verificar cual de los dos LEDs es el izquierdo.

### 2026-11-03 — Entrada de jugada probada en la placa (Pasos 8-11)

**Quien / donde:** Sergio y Pablo · laboratorio (placa nº 12)

**Hecho:**
- Probada en la placa la maquina de entrada completa, con `TEST_REBOTES = 0` y `TEMPORIZADOR_ENTRADA = 1` (timer1).
- Secuencia probada y correcta:
  - 8led apagado al arrancar; cualquier boton empieza la partida y muestra `F`.
  - Derecho sin numero elegido: no hace nada.
  - Izquierdo: 1, 2... 8 y vuelta a 1; mantenido, autoincrementa y al soltar el numero queda fijo.
  - Derecho: confirma la fila y muestra `C`; igual con la columna.
  - Tras confirmar la columna, el led parpadea 3 s y vuelve a `F`.
  - Cancelacion pulsando cualquier boton durante el parpadeo: el led se apaga, vuelve `F` y la jugada no se realiza.
- Jugada confirmada (fila 3, columna 5): `reversi8()` coloca la ficha negra en `tablero[2][4]` (el juego usa indices 0-7, el 8led muestra 1-8).

**Ficheros tocados:** ninguno (solo prueba).

**Datos / medidas** `[MED]`:
- Tras la jugada fila 3 / columna 5: `tablero[2][4] = 2` (negra, la del humano) y `tablero[2][2] = 1` (blanca, respuesta de la maquina; su legalidad no se ha comprobado).

**Decisiones de diseño** `[ES]`:
- Sin cambios: se mantiene el timer1 como temporizador de la entrada.

**Problemas y solucion** `[PROB]`:
- Ninguno en esta prueba.

**Para la memoria:**
- `[ES]` Secuencia de prueba de la entrada de jugada con el resultado esperado y observado en cada paso.
- `[MED]` Captura de `tablero` tras confirmar una jugada.

**Pendiente para la proxima sesion:**
- [x] Mirar la pila de depuracion tras una prueba completa: `EV_JUGADA_CONFIRMADA` (7) y `EV_JUGADA_CANCELADA` (8) a partir de `indice − 1`; guardar una captura para la memoria. (Hecho el 2026-11-09.)
- [x] Confirmar que el led que parpadea es el izquierdo (`led1`); si es el otro, cambiar a `led2` en `entrada.c`. (Es el izquierdo, 2026-11-09.)
- [ ] Hacer commit de todo lo hecho en esta sesion.

### 2026-11-09 — Segunda placa: rebotes, antirrebotes, ids 7 y 8, tablero y partida de 30 jugadas (Pasos 8-12)

**Quien / donde:** Sergio (9:00–11:00, aun sin terminar) · laboratorio (placa nº 4, distinta de la nº 12 del 3-nov, mismo programa)

**Hecho:**
- Rebotes medidos con `TEST_REBOTES = 1`: 4 pulsaciones de ~1 s de SB2 (izq) y de SB3 (der) y un toque rapido de cada uno (102 eventos en la pila).
- Subido `TRD_MS` de 30 a 50 ms (`TRP_MS` sigue en 60), `TEST_REBOTES = 0`, y repetida la misma prueba con el antirrebotes activo (132 eventos).
- Pila de depuracion con una jugada confirmada (id 7) y otra cancelada (id 8).
- Jugada fila 3 / columna 4 comprobada en `tablero` con Expressions.
- Confirmado que el led que parpadea (`led1`) es el izquierdo.
- Partida de 30 jugadas legales contra la maquina (unas 300 pulsaciones), simulada antes en Python con la misma logica que `reversi8_2026.c`; `tablero` comprobado tras la 10, la 20 y la 30. Tras la 30 el tablero queda lleno y coincide casilla por casilla con la simulacion: 31 blancas (maquina) y 33 negras (humano), contado con `pruebas/contar.gdb`.
- Pila de depuracion coherente tras la partida: da la vuelta, parpadeos (id 6) cada ~167 ms y una confirmacion (id 7) al final de cada tanda.

**Ficheros tocados:** `button.c` (`TRD_MS` 50 y comentario con las medidas), `button.h` (`TEST_REBOTES` a 0), `entrada.c` (comentario del led izquierdo), `reversi8_2026.c` (`signed char`), `pruebas/contar.gdb` (nuevo), `PRACTICA2.md`.

**Datos / medidas** `[MED]` (rafaga = del primer al ultimo evento; 4 pulsaciones por boton + 1 toque rapido):
- Al pulsar, izq: 0 / 0 / 22,8 / 22,4 ms (toque rapido 13,0 ms). Der: 0,5 / 28,5 / 93,7 ms (toque rapido sin rebotes). El caso de 93,7 ms: el boton se vio a 1 entre los 84 y los 94 ms.
- Al soltar, izq: 1,6 / 26,6 / 18,3 / 23,9 ms (toque rapido 4,3 ms). Der: 5,0 ms (23 eventos) / 38,2 / 20,7 ms.
- Placa anterior (3-nov): 41,5 ms al pulsar y 12,4 ms al soltar. Peor caso entre las dos placas: ~41,5 ms al pulsar y 38,2 ms al soltar.
- Toques rapidos reales de unos 100-110 ms con el programa de test.
- Con el antirrebotes activo: 10 pulsaciones reales dieron 10 avisos de pulsado y 10 de soltado. Aviso de pulsado a los 60,0 ms, encuesta cada 50,1 ms, `trd` de 50,1 ms (marcas de `timer2`).
- Autoincremento: primer evento del timer1 a los 560 ms (500 ms despues de confirmar la pulsacion); parpadeo: 18 eventos del timer1 cada 167,008 ms, id 7 a los 3,006 s y 60 µs despues del ultimo; id 8 apilado 34 µs despues de confirmar la pulsacion que cancela.
- Tablero tras la jugada fila 3 / columna 4 (`tablero[2][3]`): `tablero[2] = {0,0,1,2,0,0,0,0}`, `tablero[3] = {0,0,0,1,2,0,0,0}`, `tablero[4] = {0,0,0,2,1,0,0,0}`; la maquina responde en `[2][2]`. Coincide con la simulacion a mano.
- Ficheros: volcados y capturas en `pruebas/rebotes_placa4/`.
- Partida: con el fallo de `voltear` (ver `[PROB]`), el tablero tras la jugada 20 difería en 18 casillas del esperado y la simulacion imitando el fallo daba 0 diferencias con el volcado de la placa. Con la correccion: 0 diferencias en las jugadas 10, 20 y 30.

**Decisiones de diseño** `[ES]`:
- `trd` = 50 ms: cubre la rafaga de 38,2 ms al soltar de la placa 4; con 30 ms un rebote tras reactivar EINT podria dar una pulsacion falsa.
- `trp` = 60 ms sin cambios: no se sube para no descartar toques cortos (36,7 ms vistos en la placa 12), y un `trp` de ~90 ms caeria en el hueco del caso de 93,7 ms. Valores elegidos para que valgan en las dos placas.

**Problemas y solucion** `[PROB]`:
- Problema: en 3 pulsaciones del izquierdo el primer pulso se leia a 1 a los 60 ms (descartado como espurio, sin aviso) y un segundo flanco 9-100 ms despues empezaba un ciclo bueno → Solucion: ninguna necesaria, la maquina lo resuelve sola (el espurio pasa por `trd` y reactiva EINT); el aviso de pulsado llega entre 100 y 200 ms tarde.
- Problema: una marca de `timer2` salio unos 2 ms tarde (el timer0 aparece a 58,0 ms en vez de 60) → Solucion: pendiente; se parece al desfase de 2,048 ms al desbordar `timer2_leer()`; no afecta al comportamiento, solo al instante apilado.
- Problema: con el test de rebotes, una rafaga al pulsar acabo en lectura a 1 (id 3) con el boton pulsado; parece que el ultimo flanco se pierde porque la RSI limpia el pendiente al terminar (hipotesis, sin comprobar) → Solucion: no afecta al antirrebotes, que lee el nivel pasado `trp`.
- Problema: en ARM `char` es sin signo, asi que `vSF`/`vSC` con -1 valian 255 y `voltear` hacia `FA = FA + 255`: no volteaba en las direcciones con componente negativa (N, NE, O, NO, SO) y escribia fuera de `tablero` → Solucion: `vSF`, `vSC` y los parametros/locales `SF`, `SC` de `voltear` y `actualizar_tablero` pasan a `signed char`. El ensamblador ya truncaba a 8 bits y su comparacion sin signo (`cmp` + `bhs`) cubre bien `f >= 0` y `f < DIM`, asi que no se toca. No se habia visto antes porque las primeras jugadas no volteaban hacia arriba/izquierda.

**Para la memoria:**
- `[PROB]` `char` sin signo en ARM y los desplazamientos -1 de `voltear`; como se detecto (comparando el volcado de `tablero` con la simulacion).
- `[MED]` Tablero final de la partida de 30 jugadas y su recuento (31/33).
- `[MED]` Tabla de rafagas de las dos placas y razon de `trp = 60` / `trd = 50`.
- `[MED]` Capturas de la pila con los ids 7 y 8 y de `tablero` tras una jugada.
- `[ES]` El pulso espurio se resuelve sin perder la pulsacion: pasa por `trd` y el siguiente flanco abre otro ciclo.

**Pendiente para la proxima sesion:**
- [ ] Anotar el numero de placa de la sesion.
- [ ] Mirar la desviacion de ~2 ms de `timer2_leer()` al desbordar.
- [ ] Decidir si la suelta exige dos lecturas seguidas a 1 en la encuesta.
- [ ] Decidir timer1 o timer2 y preguntar lo del «paso» al profesor.
- [ ] Medir la pila IRQ en la placa (rellenar con `0xDEADBEEF`).
- [x] Jugar una partida completa contra la maquina. (30 jugadas, 2026-11-09.)
- [ ] Preguntar al profesor por el "paso" y cuando se llama a `contar()` (con el tablero lleno el programa espera una jugada 31).
- [ ] Hacer commit de lo hecho hoy.

### 2026-11-10 — Final de partida con tablero lleno y calibracion de la placa 7 (Pasos 9 y 12)

**Quien / donde:** Sergio y Pablo (9:00–10:30) · laboratorio (placa nº 7)

**Hecho:**
- El profesor pide que el juego detecte el tablero lleno. Contador `vacias` en `reversi8()`: empieza en 60 (64 casillas - 4 fichas iniciales), baja con cada ficha puesta en casilla libre y el bucle acaba al llegar a 0.
- Al acabar la partida, `Main` enmascara `BIT_EINT4567` (para que la entrada no pise el display) y muestra una **E** en el 8led.
- Probado en la placa 7: partida hasta quedar 2 casillas libres (`tablero[7][3]` y `[7][6]`, comprobado con `p 'reversi8_2026.c'::tablero`); tras jugar fila 8 / columna 4 el tablero se llena y sale la E.
- Placa 7 recalibrada: los botones iban mal con trp 60 / trd 50. Rebotes medidos con `TEST_REBOTES = 1`: una prueba mixta (10 pulsaciones de 1 s y 5 toques rapidos por boton) y despues 5 pulsaciones de 1 s por boton, cada boton por separado.
- `pruebas/rafagas.py`: agrupa en rafagas un volcado de la pila (`x/300xw 0x0C7FE000` pegado tal cual) y da la mas larga. Comprobado con el volcado de la placa 4 (da los mismos 38,2 ms).

**Ficheros tocados:** `reversi8_2026.c` (`vacias`), `main.c` (E al acabar y `while (1)`), `button.c` (`TRP_MS`/`TRD_MS` a 100 y comentario con las medidas), `pruebas/rafagas.py` (nuevo), `pruebas/rebotes_placa7/` (volcados), `PRACTICA2.md`, `CLAUDE.md`, `README.md`.

**Datos / medidas** `[MED]` (placa 7; rafaga = del primer al ultimo evento separados menos de 30 ms):
- La pila da la vuelta incluso con 5 pulsaciones (>150 eventos): esta placa rebota muchisimo, con rafagas de hasta 39 y 61 eventos.
- Prueba mixta (`volcado_pila_hex.txt`, `indice` = 33): solo quedan los toques rapidos. Rafagas de hasta 64,3 ms al pulsar y 59,3 ms al soltar. Toques rapidos de 53, 67, 80 y 116 ms entre pulsar y soltar.
- 5 × 1 s izquierdo (`izq_5x1s.txt`, `indice` = 137): rafagas de hasta 67,7 ms (quedan los ultimos ~5 s).
- 5 × 1 s derecho (`der_5x1s.txt`, `indice` = 12): la mayoria entre 10 y 55 ms; un caso de 119,6 ms (entre 3,8 y 4,1 s el boton estuvo ~300 ms rebotando).
- Comparacion: placa 12 ~41,5 ms, placa 4 38,2 ms (y un caso suelto de 94 ms), placa 7 hasta 67,7 ms (y uno de 119,6 ms).

**Decisiones de diseño** `[ES]`:
- Contador de casillas vacias en lugar de recorrer el tablero cada turno: se actualiza en O(1) al poner ficha. Solo se resta si la casilla estaba libre, porque la jugada del humano no se valida y podria caer en una ocupada.
- Si el humano pone la ultima ficha se sale con `break` antes de `elegir_mov`: si no, la maquina no encuentra jugada, `move` vale 1 y el programa se quedaria esperando otra jugada con el tablero lleno.
- E (de *end*) porque F y C ya los usa la entrada. Se enmascaran los botones al acabar para que una pulsacion no cambie el display.
- trp = trd = 100 ms: cubre todas las rafagas de la placa 7 salvo el caso de 119,6 ms. Coste: se descartan los toques de menos de 100 ms (de los 4 toques rapidos medidos, solo el de 116 ms pasaria). Para elegir fila/columna se pulsa sin prisa y existe el autoincremento, asi que se acepta. Si al jugar se pierden pulsaciones, bajar trp a 70 ms y dejar trd en 100.

**Problemas y solucion** `[PROB]`:
- Problema: los botones de la placa 7 daban pulsaciones dobles o perdidas → Solucion: medir los rebotes y subir trp/trd de 60/50 a 100 ms.
- Problema: tras cargar, el programa no llegaba al breakpoint de `Main`; al suspender, el PC estaba en `timer2_parar` (que no se llama nunca), el desensamblado no cuadraba con el fuente y el marco anterior era `0xea000052` (una instruccion `b`, no una direccion) → Solucion: igual que el 3-nov, la placa ejecutaba basura; apagar y encender la placa y volver a cargar.
- Problema: en la primera version del contador, una jugada humana sobre una casilla ocupada tambien restaba → Solucion: restar solo si `tablero[fila][columna] == CASILLA_VACIA`.

**Para la memoria:**
- `[ES]` Deteccion del final: contador `vacias`, salida con `break` si el humano llena el tablero y E en el 8led con los botones enmascarados.
- `[MED]` Tabla de rafagas de las tres placas y por que trp/trd pasan a 100 ms (y lo que cuesta en toques rapidos).
- `[PROB]` Placa ejecutando basura antes de `Main`: como se reconoce (PC en una funcion que no se llama, marco `0xea000052`).
- `[CONC]` Los rebotes cambian mucho de una placa a otra: los tiempos del antirrebotes hay que medirlos en cada placa.

**Pendiente para la proxima sesion:**
- [ ] Jugar con trp = 100 ms y ver si se pierden pulsaciones; si es asi, probar trp = 70 ms.
- [ ] Decidir si la suelta exige dos lecturas seguidas a 1 en la encuesta (la placa 7 rebota a mitad de pulsacion).
- [ ] Preguntar al profesor si hace falta el «paso» ahora que la partida acaba con el tablero lleno.
- [ ] Opcional: mostrar el ganador al acabar (`contar()` ya calcula blancas y negras).
- [ ] Medir la pila IRQ en la placa (rellenar con `0xDEADBEEF`).
- [ ] Hacer commit de lo hecho hoy.

---

## 3. Plantilla de sesion (copiar, no rellenar aqui)

```markdown
### AAAA-MM-DD — <titulo corto> (Paso N)

**Quien / donde:** Sergio y Pablo · laboratorio (placa nº __) | casa (emulador)

**Hecho:**
- ...

**Ficheros tocados:** `fichero.c` (que), ...

**Datos / medidas** `[MED]`:
- ... (valor, unidad, placa, cuantas repeticiones)

**Decisiones de diseño** `[ES]`:
- ... y por que

**Problemas y solucion** `[PROB]`:
- Problema: ... → Solucion: ...

**Para la memoria:**
- `[ETIQUETA]` frase o idea que ira en la memoria

**Pendiente para la proxima sesion:**
- [ ] ...
```
