/*********************************************************************************************
* Fichero:		timer2.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		librería de medición de tiempos con el timer2 del s3c44b0x
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "timer2.h"
#include "44b.h"
#include "44blib.h"

/*--- constantes ---*/
/* Máxima precisión: preescalado 0 y divisor 1/2 -> F = 64 MHz / (1 * 2) = 32 MHz, 32 ticks por us */
#define TICKS_POR_US	32
#define CUENTA_MAX		65535	// TCNTB2: la cuenta va de 65535 a 0, periodo de 65536 ticks = 2048 us

/* Bits del timer2 en TCON (bits 12-15) */
#define T2_START		(1 << 12)
#define T2_MANUAL_UPD	(1 << 13)
#define T2_INVERTER		(1 << 14)
#define T2_AUTO_RELOAD	(1 << 15)
#define T2_BITS			(T2_START | T2_MANUAL_UPD | T2_INVERTER | T2_AUTO_RELOAD)

/*--- variables globales del módulo ---*/
/* nº de periodos completos del timer2 desde timer2_empezar; la modifica la ISR */
static volatile unsigned int timer2_num_int = 0;

/* declaración de función que es rutina de servicio de interrupción
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
static void timer2_ISR(void) __attribute__((interrupt("IRQ")));

/*--- codigo de las funciones ---*/
static void timer2_ISR(void)
{
	timer2_num_int++;

	/* borrar bit en I_ISPC para desactivar la solicitud de interrupción */
	rI_ISPC |= BIT_TIMER2;
}

void timer2_inicializar(void)
{
	/* sys_init() ya deja INTMOD en IRQ, INTCON vectorizado y BIT_GLOBAL desenmascarado */
	pISR_TIMER2 = (unsigned) timer2_ISR;
	rINTMSK &= ~(BIT_TIMER2);

	/* preescalado 1 (timers 2 y 3, bits 8-15 de TCFG0) = 0, sin tocar el del timer0 */
	rTCFG0 &= ~(0xFF << 8);
	/* mux del timer2 (bits 8-11 de TCFG1) = 0000 -> divisor 1/2 */
	rTCFG1 &= ~(0xF << 8);

	rTCNTB2 = CUENTA_MAX;
	rTCMPB2 = 0;

	/* timer2 parado; solo se tocan sus bits para no afectar al resto de timers */
	rTCON &= ~T2_BITS;
}

void timer2_empezar(void)
{
	timer2_num_int = 0;

	/* manual update carga TCNTB2 en el contador; después se arranca con auto-reload
	 * y a la vez se quita el manual update */
	rTCON = (rTCON & ~T2_BITS) | T2_MANUAL_UPD;
	rTCON = (rTCON & ~T2_BITS) | T2_START | T2_AUTO_RELOAD;
}

unsigned int timer2_leer(void)
{
	unsigned int n, cuenta;

	/* si la ISR salta entre las dos lecturas, n y cuenta no casan: se repite */
	do {
		n = timer2_num_int;
		cuenta = rTCNTO2;
	} while (n != timer2_num_int);

	/* cada periodo completo son 65536 / 32 = 2048 us; así no hay desbordamiento
	 * hasta 2^32 us (~71 min) */
	return n * ((CUENTA_MAX + 1) / TICKS_POR_US) + (CUENTA_MAX - cuenta) / TICKS_POR_US;
}

unsigned int timer2_parar(void)
{
	unsigned int t = timer2_leer();

	rTCON &= ~T2_START;
	return t;
}
