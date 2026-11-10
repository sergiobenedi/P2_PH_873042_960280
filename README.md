# P2_873042_960280 — Proyecto Hardware, Practica 2 (UNIZAR)

Autores: Sergio Benedi (NIP 873042), Pablo Gimeno (NIP 960280).

> ⚠️ Este CLAUDE.md y [README.md](README.md) son gemelos: cualquier cambio relevante en uno debe reflejarse en el otro.

## Que es esto

Practica 2 de la asignatura Proyecto Hardware (3º Ingenieria Informatica, UNIZAR):
**sistema de E/S y dispositivos basicos**. Juego **Reversi de 8 casillas** sobre una placa ARM7
con SoC **Samsung S3C44B0X**, jugado con los dos pulsadores y el display de 8 segmentos,
con rutinas de interrupcion en C, antirrebotes por timer y una pila de depuracion de eventos.
Parte del codigo de la Practica 1 (logica del juego con `patron_volteo` en ARM y `color_ficha` en Thumb).
Toolchain: Eclipse + ARM Sourcery GCC + openOCD (JTAG).

## Instrucciones para Eclipse

1. Colocar los fuentes en vuestro workspace

2. En la configuracion del proyecto hay que incluir el directorio common para que encuentre los archivos.
   Esto se hace en la configuracion del proyecto. Lo hareis en tres sitios:
	- ARM Sourcery Windows GCC Assembler >> Directories
	- ARM Sourcery Windows GCC C compiler >> Directories
   En cada sitio se selecciona add y se añade la carpeta common

3. Seguir las indicaciones de GuiaEntorno.pdf para crear el proyecto,
   utilizar una herramienta externa (openOCD) y
   configurar la depuracion (ver apartado 1.1.2. Depuracion sobre la placa ARM)

4. En el apartado 1.1.2.3 piden introducir la ubicacion del ejecutable openocd.
   Este ejecutable se encuentra en:
        C:\Program Files\EclipseARM\openocd-0.7.0\bin\openocd-0.7.0.exe
 
5. Habra que añadir el path tanto en el proyecto
   como en la configuracion del Jtag (openOCD).
   En el segundo caso hay que ponerlo en environment.

6. Al configurar el modo de depuracion en el punto 4 del guion haremos dos cambios. 
	- Para evitar problemas con el path en lugar de poner arm-none-eabi-gdb pondremos la direccion absoluta: 
		C:\Program Files\EclipseARM\sourcery-g++-lite-arm-2011.03\bin\arm-none-eabi-gdb.exe
	- En la pestaña startup no activaremos la opcion 'Set Breakpoint at' ni 'resume'.
	  No pasa nada por hacerlo, sencillamente como en el codigo no hay ningun start, se ignora

## Estructura

- [main.c](main.c) — punto de entrada (`Main`): `sys_init()`, `inicializar_perifericos()` y `reversi8()`.
- [reversi8_2026.c](reversi8_2026.c) / [.h](reversi8_2026.h) — logica del juego (de la P1). La partida acaba cuando `vacias` (casillas en blanco) llega a 0; `Main` muestra entonces una E en el 8led.
- [reversi8_arm.asm](reversi8_arm.asm) — `patron_volteo_arm` (ARM), llama a `color_ficha_thumb`.
- [reversi8_thumb.asm](reversi8_thumb.asm) — `color_ficha_thumb` (Thumb).
- [led.c](led.c) / [.h](led.h) — control de los 2 LEDs de la placa.
- [8led.c](8led.c) / [.h](8led.h) — control del display de 8 segmentos (tabla de simbolos por bitmap).
- [button.c](button.c) / [.h](button.h) — pulsadores por interrupcion externa (EINT6-7) y maquina antirrebotes (ESPERANDO, RETARDO_TRP, ENCUESTA, RETARDO_TRD). `TEST_REBOTES` en el `.h`: modo de test que solo apila eventos. El teclado matricial (EINT1) pasa por la misma maquina (`BOTON_TECLADO`).
- [teclado.c](teclado.c) / [.h](teclado.h) — opcional: teclado matricial 4x4 en el banco 3. `teclado_leer` escanea las 4 filas y devuelve 1-8, `TECLA_OK`, `TECLA_NO` o `TECLA_NINGUNA` (solo 10 teclas habilitadas, figura 2 del guion). `TECLADO` en el `.h`: 1 = habilitado.
- [timer.c](timer.c) / [.h](timer.h) — timer0 de un solo disparo (`timer0_programar`, retardos del antirrebotes) y temporizador de la entrada de jugada (`temporizador_entrada_*`). `TEMPORIZADOR_ENTRADA` en el `.h`: 1 = timer1, 2 = interrupcion del timer2.
- [entrada.c](entrada.c) / [.h](entrada.h) — maquina de estados de la entrada de jugada (INICIO, FILA, COLUMNA, CONFIRMAR) con el 8led y el led izquierdo. Las RSI llaman a `entrada_boton_pulsado/soltado`, `entrada_tecla` y `entrada_temporizador`; `reversi8()` espera con `entrada_jugada_lista`.
- [timer2.c](timer2.c) / [.h](timer2.h) — libreria de medicion de tiempos con timer2 (de la P1): `timer2_inicializar/empezar/leer/parar`, resolucion 1/32 µs. Da la marca de tiempo de la pila de depuracion.
- [eventos.h](eventos.h) — IDs de los eventos de la pila de depuracion.
- [lista.c](lista.c) / [.h](lista.h) — pila de depuracion: lista circular de 150 eventos (id + instante, 8 B) en `0x0C7FE000`; `lista_inicializar`, `guardar_eventos`.
- [pruebas/](pruebas/) — scripts de GDB (`lista_sim.gdb`, `contar.gdb`), `rafagas.py` (agrupa en rafagas un volcado de la pila de depuracion para medir rebotes) y volcados/capturas por placa (`rebotes_placa4/`, `rebotes_placa7/`). Eclipse no los compila.
- [common/](common/) — soporte de placa suministrado (`44b.h`, `44blib.*`, `44binit.asm`,
  script de arranque, linker script). **No modificar salvo indicacion explicita.**
- [emulador/](emulador/) — arranque para el modo emulador (`init_b_2026.asm`, linker script propio).
- [Debug/](Debug/) — salida de compilacion de Eclipse (objetos, .elf, .hex, .map). Generado, no editar a mano.

## Convenciones del codigo base

- Cabecera de fichero estandar (Fichero/Autor/Descrip/Version) en cada .c/.h — mantenerla al crear ficheros nuevos.
- Comentarios y nombres en español, coherente con el resto del proyecto.
- Las funciones marcadas `(*)` en los .h ya vienen invocadas desde codigo suministrado; el resto son las que hay que completar/usar en la practica.
- No tocar `common/` ni los scripts de arranque/linker salvo que el guion lo pida explicitamente: son la base que da el profesorado.

## Estado

- Practica 2 (E/S con botones, 8led, timers y pila de depuracion) partiendo del codigo de la P1.
- Plan, lo que sobra de la P1 y paso a paso: [PRACTICA2.md](PRACTICA2.md).
- Cuaderno de sesiones: [MEMORIA_NOTAS.md](MEMORIA_NOTAS.md). Cuando se pida "apuntalo", añadir
  una entrada con la plantilla de su §3 y una fila en el registro de horas.

## Documentacion de la practica

Fuera del repo, en `../P2_DOCUMENTACION/` (guion `PH_practica2_2026.pdf`, cuadernos de la UCM,
manual del S3C44B0X). Resumida en [PRACTICA2.md](PRACTICA2.md) para no releer los PDFs.
