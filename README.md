# Fuentes de la Práctica 1 de Proyecto Hardware

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
 
5. Habrá que añadir el path tanto en el proyecto (como en la práctica 1)
   como en la configuración del Jtag (openOCD).
   En el segundo caso hay que ponerlo en environment.

6. Al configurar el modo de depuración en el punto 4 del guión haremos dos cambios. 
	- Para evitar problemas con el path en lugar de poner arm-none-eabi-gdb pondremos la dirección absoluta: 
		C:\Program Files\EclipseARM\sourcery-g++-lite-arm-2011.03\bin\arm-none-eabi-gdb.exe
	- En la pestaña startup no activaremos la opción 'Set Breakpoint at' ni 'resume'.
	  No pasa nada por hacerlo, sencillamente como en el código no hay ningún start, se ignora
