/*********************************************************************************************
* Fichero:	main.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	punto de entrada de C
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "reversi8_2026.h"
#include "timer2.h"
#include "44blib.h"
#include "44b.h"

/* 1: calibrar timer2 contra Delay() antes de jugar (Paso 7); 0: solo el juego */
#define CALIBRAR_TIMER2 1

/*--- variables globales ---*/
/* us medidos por timer2 para Delay() de 1 ms, 10 ms, 1 s y 10 s (ver en Expressions) */
unsigned int tiempos_calibracion[4];

/*--- codigo de funciones ---*/
#if CALIBRAR_TIMER2
static void calibrar_timer2(void)
{
	/* Delay(n) espera n * 100 us */
	static const int esperas[4] = { 10, 100, 10000, 100000 };
	int i;

	for (i = 0; i < 4; i++)
	{
		timer2_empezar();
		Delay(esperas[i]);
		tiempos_calibracion[i] = timer2_parar();
	}
}
#endif

void Main(void)
{
	/* Inicializa controladores */
	sys_init();         // Inicializacion de la placa, interrupciones y puertos
	timer2_inicializar();

#if CALIBRAR_TIMER2
	calibrar_timer2();
#endif

	reversi8();
}
