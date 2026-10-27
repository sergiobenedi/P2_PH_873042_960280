# Notas para la memoria — Práctica 2 (Sistema de E/S y dispositivos básicos)

Autores: Sergio Benedi (NIP 873042), Pablo Gimeno (NIP 960280)
Entrega: ~23 de noviembre de 2026 (confirmar en Moodle).
ZIP: `p2_873042-Apellidos_960280-Apellidos.zip` (memoria en PDF + código fuente).
Plan de la práctica: [PRACTICA2.md](PRACTICA2.md).

> Este fichero NO es la memoria. Es el cuaderno de sesiones: cada vez que se pida "apúntalo",
> se añade una entrada nueva al final de §2 con la plantilla de §3 y una fila en §1.
> Etiquetas = apartado de la memoria (Anexo I del guion) donde se usará cada nota:
> `[RES]` Resumen ejecutivo · `[ES]` Gestión de la E/S y máquinas de estados ·
> `[PILA]` Marco de pila de las RSI · `[MED]` Medidas (rebotes, trp, trd, tiempos) ·
> `[PROB]` Problemas y soluciones · `[COD]` Código fuente · `[CONC]` Conclusiones.

---

## 1. Registro de horas

| Fecha | Quién | Horas | Pasos | Qué |
|-------|-------|-------|-------|-----|
| 2026-10-27 | Sergio | ? | 3 | Arreglos de E/S (§3.2 de PRACTICA2.md) y avisos de compilación |

---

## 2. Sesiones

<!-- Añadir aquí cada sesión con la plantilla de §3, la más reciente al final. -->

### 2026-10-27 — Arreglos de E/S (Paso 3)

**Quién / dónde:** Sergio · casa (sin placa, solo compilación en Eclipse)

**Hecho:**
- Corregidos los fallos de E/S de la tabla §3.2 de PRACTICA2.md.
- Quitadas las demos de la P1: `switch_leds` (timer) e `int_count` + `D8Led_symbol` (RSI de botones).
- Quitados los avisos `array subscript has type 'char'` de `reversi8_2026.c`.
- Workspace de Eclipse limpio: cerrados P1/PruebaSim, borrados marcadores `make: ***` viejos. Compila sin errores ni avisos.
- Commit `550cfe2` subido a `develop`.

**Ficheros tocados:** `timer.c/.h` (I_ISPC, máscaras timer0, fuera `switch_leds`), `button.c` (RSI por bits, máscaras puerto G/EXTINT), `led.c` (`volatile`, máscaras por bit), `reversi8_2026.c` (índices `int`).

**Decisiones de diseño** `[ES]`:
- `rINTMOD`/`rINTCON` solo en `sys_init`; cada `*_init` solo desenmascara su línea en `rINTMSK`.
- Cada init toca solo sus bits (`(reg & ~mascara) | valor`) para no machacar otros periféricos (p. ej. timer2, que da las marcas de tiempo).
- La RSI de botones mira cada bit de `EXTINTPND` con `&` y borra solo los atendidos (pueden llegar EINT6 y EINT7 a la vez).
- En `Eint4567_init` se borran los pendientes **antes** de desenmascarar, para no atender una interrupción espuria nada más arrancar.

**Problemas y solución** `[PROB]`:
- Problema: `rI_ISPC |= BIT_x` lee un registro de solo escritura → Solución: `rI_ISPC = BIT_x`.
- Problema: `timer_init` ponía `rTCFG0 = 255`, `rTCFG1 = 0`, `rTCON = ...` y machacaba timer2 → Solución: máscaras de los bits del timer0.
- Problema: `rPCONG = 0xffff`, `rPUPG = 0` y `rEXTINT |= 0x22222222` reconfiguraban todo el puerto G → Solución: solo pines 6-7 (`PCONG` bits 15:12, `PUPG` bits 6-7, `EXTINT` bits 31:24).
- Problema: `int switch_leds;` definida en `timer.h` → Solución: eliminada (era de la demo).
- Problema: Problems de Eclipse mezclaba errores de otros proyectos del workspace → Solución: cerrarlos y borrar los marcadores viejos.

**Para la memoria:**
- `[PROB]` I_ISPC es de solo escritura: con `|=` se escribe lo leído (indefinido) y se pueden borrar peticiones de otras líneas.
- `[ES]` Inicialización respetando los bits de otros dispositivos: patrón lectura-máscara-escritura.

**Pendiente para la próxima sesión:**
- [ ] Paso 4: en el emulador, comparar el prólogo/epílogo de una RSI con una función normal (marco de pila `[PILA]`).
- [ ] Paso 5: `inicializar_perifericos()` llamando a `timer_init` y `Eint4567_init` (aún no se llaman).

---

## 3. Plantilla de sesión (copiar, no rellenar aquí)

```markdown
### AAAA-MM-DD — <título corto> (Paso N)

**Quién / dónde:** Sergio y Pablo · laboratorio (placa nº __) | casa (emulador)

**Hecho:**
- ...

**Ficheros tocados:** `fichero.c` (qué), ...

**Datos / medidas** `[MED]`:
- ... (valor, unidad, placa, cuántas repeticiones)

**Decisiones de diseño** `[ES]`:
- ... y por qué

**Problemas y solución** `[PROB]`:
- Problema: ... → Solución: ...

**Para la memoria:**
- `[ETIQUETA]` frase o idea que irá en la memoria

**Pendiente para la próxima sesión:**
- [ ] ...
```
