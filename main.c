/*********************************************************************************************
* Fichero:	main.c
* Autor:    
* Descrip:	punto de entrada de C
* Version:  <P4-ARM.timer-leds>
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "reversi8_2026.h"
#include "44blib.h"
#include "44b.h"

/*--- variables globales ---*/


/*--- codigo de funciones ---*/
void Main(void)
{
	/* Inicializa controladores */
	sys_init();         // Inicializacion de la placa, interrupciones y puertos

	reversi8();
}
