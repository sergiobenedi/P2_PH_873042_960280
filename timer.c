/*********************************************************************************************
* Fichero:		timer.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		funciones de control del timer0 del s3c44b0x
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "timer.h"
#include "44b.h"
#include "44blib.h"

/* declaración de función que es rutina de servicio de interrupción
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void timer_ISR(void) __attribute__((interrupt("IRQ")));

/*--- codigo de las funciones ---*/
void timer_ISR(void)
{
	/* (la máquina antirrebotes del Paso 5 irá aquí) */

	/* borrar bit en I_ISPC: es de solo escritura, se asigna (no |=) */
	rI_ISPC = BIT_TIMER0;
}

void timer_init(void)
{
	/* INTMOD/INTCON ya los configura sys_init: aquí solo desenmascaramos el timer0 */
	rINTMSK &= ~(BIT_TIMER0);

	/* Establece la rutina de servicio para TIMER0 */
	pISR_TIMER0 = (unsigned) timer_ISR;

	/* Configura el Timer0 tocando solo sus bits (no machacar timer1-5, timer2 da las marcas de tiempo) */
	rTCFG0 = (rTCFG0 & ~0xFF) | 255;	// preescalado timers 0-1 = 255 (bits 7:0)
	rTCFG1 = rTCFG1 & ~0xF;				// divisor timer0 = 1/2 (bits 3:0 = 0000)
	rTCNTB0 = 65535;					// valor inicial de cuenta (la cuenta es descendente)
	rTCMPB0 = 12800;					// valor de comparación
	rTCON = (rTCON & ~0xF) | 0x2;		// timer0: update manual (bit 1)
	rTCON = (rTCON & ~0xF) | 0x9;		// timer0: start (bit 0) + auto-reload (bit 3)
}
