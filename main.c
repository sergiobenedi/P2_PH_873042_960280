/*********************************************************************************************
* Fichero:	main.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	punto de entrada de C
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "reversi8_2026.h"
#include "timer2.h"
#include "timer.h"
#include "button.h"
#include "led.h"
#include "8led.h"
#include "44blib.h"
#include "44b.h"

/*--- codigo de funciones ---*/

/* Inicializa en un único sitio todos los periféricos que usa la práctica.
 * Requiere sys_init() antes (INTMOD, INTCON, puertos y BIT_GLOBAL) */
static void inicializar_perifericos(void)
{
	timer2_inicializar();	// timer2: marca de tiempo en us (primero: la usa la pila de depuración)
	leds_off();				// LEDs apagados
	D8Led_init();			// display de 8 segmentos
	timer_init();			// timer0: antirrebotes de los pulsadores
	Eint4567_init();		// pulsadores EINT6/EINT7 (lo último: a partir de aquí ya pueden llegar IRQ)
}

void Main(void)
{
	sys_init();					// Inicializacion de la placa, interrupciones y puertos
	inicializar_perifericos();

	reversi8();
}
