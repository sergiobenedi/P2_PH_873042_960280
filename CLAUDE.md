# P2_873042_960280 — Proyecto Hardware, Práctica 2 (UNIZAR)

Autores: Sergio Benedi (NIP 873042), Pablo Gimeno (NIP 960280).

> ⚠️ Este CLAUDE.md y [README.md](README.md) son gemelos: cualquier cambio relevante en uno debe reflejarse en el otro.

## Qué es esto

Práctica 2 de la asignatura Proyecto Hardware (3º Ingeniería Informática, UNIZAR):
**sistema de E/S y dispositivos básicos**. Juego **Reversi de 8 casillas** sobre una placa ARM7
con SoC **Samsung S3C44B0X**, jugado con los dos pulsadores y el display de 8 segmentos,
con rutinas de interrupción en C, antirrebotes por timer y una pila de depuración de eventos.
Parte del código de la Práctica 1 (lógica del juego con `patron_volteo` en ARM y `color_ficha` en Thumb).
Toolchain: Eclipse + ARM Sourcery GCC + openOCD (JTAG).

## Instrucciones para Eclipse

1. Colocar los fuentes en vuestro workspace

2. En la configuración del proyecto hay que incluir el directorio common para que encuentre los archivos.
   Esto se hace en la configuración del proyecto. Lo haréis en tres sitios:
	- ARM Sourcery Windows GCC Assembler >> Directories
	- ARM Sourcery Windows GCC C compiler >> Directories
   En cada sitio se selecciona add y se añade la carpeta common

3. Seguir las indicaciones de GuiaEntorno.pdf para crear el proyecto,
   utilizar una herramienta externa (openOCD) y
   configurar la depuración (ver apartado 1.1.2. Depuración sobre la placa ARM)

4. En el apartado 1.1.2.3 piden introducir la ubicación del ejecutable openocd.
   Este ejecutable se encuentra en:
        C:\Program Files\EclipseARM\openocd-0.7.0\bin\openocd-0.7.0.exe
 
5. Habrá que añadir el path tanto en el proyecto
   como en la configuración del Jtag (openOCD).
   En el segundo caso hay que ponerlo en environment.

6. Al configurar el modo de depuración en el punto 4 del guión haremos dos cambios. 
	- Para evitar problemas con el path en lugar de poner arm-none-eabi-gdb pondremos la dirección absoluta: 
		C:\Program Files\EclipseARM\sourcery-g++-lite-arm-2011.03\bin\arm-none-eabi-gdb.exe
	- En la pestaña startup no activaremos la opción 'Set Breakpoint at' ni 'resume'.
	  No pasa nada por hacerlo, sencillamente como en el código no hay ningún start, se ignora

## Estructura

- [main.c](main.c) — punto de entrada (`Main`), llama a `sys_init()` y `reversi8()`.
- [reversi8_2026.c](reversi8_2026.c) / [.h](reversi8_2026.h) — lógica del juego (de la P1).
- [reversi8_arm.asm](reversi8_arm.asm) — `patron_volteo_arm` (ARM), llama a `color_ficha_thumb`.
- [reversi8_thumb.asm](reversi8_thumb.asm) — `color_ficha_thumb` (Thumb).
- [led.c](led.c) / [.h](led.h) — control de los 2 LEDs de la placa.
- [8led.c](8led.c) / [.h](8led.h) — control del display de 8 segmentos (tabla de símbolos por bitmap).
- [button.c](button.c) / [.h](button.h) — manejo de pulsadores por interrupción externa (EINT4-7).
- [timer.c](timer.c) / [.h](timer.h) — timer0 del S3C44B0X; expone `switch_leds`.
- [timer2.c](timer2.c) / [.h](timer2.h) — librería de medición de tiempos con timer2 (de la P1): `timer2_inicializar/empezar/leer/parar`, resolución 1/32 µs. Da la marca de tiempo de la pila de depuración.
- [common/](common/) — soporte de placa suministrado (`44b.h`, `44blib.*`, `44binit.asm`,
  script de arranque, linker script). **No modificar salvo indicación explícita.**
- [emulador/](emulador/) — arranque para el modo emulador (`init_b_2026.asm`, linker script propio).
- [Debug/](Debug/) — salida de compilación de Eclipse (objetos, .elf, .hex, .map). Generado, no editar a mano.

## Convenciones del código base

- Cabecera de fichero estándar (Fichero/Autor/Descrip/Version) en cada .c/.h — mantenerla al crear ficheros nuevos.
- Comentarios y nombres en español, coherente con el resto del proyecto.
- Las funciones marcadas `(*)` en los .h ya vienen invocadas desde código suministrado; el resto son las que hay que completar/usar en la práctica.
- No tocar `common/` ni los scripts de arranque/linker salvo que el guión lo pida explícitamente: son la base que da el profesorado.

## Estado

- Práctica 2 (E/S con botones, 8led, timers y pila de depuración) partiendo del código de la P1.
- Plan, lo que sobra de la P1 y paso a paso: [PRACTICA2.md](PRACTICA2.md).
- Cuaderno de sesiones: [MEMORIA_NOTAS.md](MEMORIA_NOTAS.md). Cuando se pida "apúntalo", añadir
  una entrada con la plantilla de su §3 y una fila en el registro de horas.

## Documentación de la práctica

Fuera del repo, en `../P2_DOCUMENTACION/` (guion `PH_practica2_2026.pdf`, cuadernos de la UCM,
manual del S3C44B0X). Resumida en [PRACTICA2.md](PRACTICA2.md) para no releer los PDFs.
