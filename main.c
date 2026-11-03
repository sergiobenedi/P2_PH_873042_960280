/*********************************************************************************************
* Fichero:	main.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	punto de entrada de C
* Version:	2.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "reversi8_2026.h"
#include "timer2.h"
#include "timer.h"
#include "button.h"
#include "entrada.h"
#include "led.h"
#include "8led.h"
#include "lista.h"
#include "44blib.h"
#include "44b.h"

/*--- codigo de funciones ---*/

/* Inicializa en un unico sitio todos los perifericos que usa la practica.
 * Requiere sys_init() antes (INTMOD, INTCON, puertos y BIT_GLOBAL) */
static void inicializar_perifericos(void)
{
	lista_inicializar();	// pila de depuracion vacia
	timer2_inicializar();	// timer2: marca de tiempo en us (primero: la usa la pila de depuracion)
	timer2_empezar();		// en marcha desde el arranque: instante 0 = arranque
	leds_off();				// LEDs apagados
	D8Led_init();			// display de 8 segmentos
	entrada_inicializar();	// maquina de la entrada de jugada: 8led apagado (despues de D8Led_init)
	timer_init();			// timer0 (antirrebotes) y temporizador de la entrada
	Eint4567_init();		// pulsadores EINT6/EINT7 (lo ultimo: a partir de aqui ya pueden llegar IRQ)
}

void Main(void)
{
	sys_init();					// Inicializacion de la placa, interrupciones y puertos
	inicializar_perifericos();

	reversi8();
}
