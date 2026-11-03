/*********************************************************************************************
* Fichero:		timer.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		timer0 (retardos del antirrebotes) y temporizador de la entrada de jugada
* Version:	3.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "timer.h"
#include "button.h"
#include "entrada.h"
#include "timer2.h"
#include "44b.h"
#include "44blib.h"

/*--- constantes ---*/
/* Timers 0 y 1: preescalado 255 y divisor 1/2 -> 64 MHz / 256 / 2 = 125 kHz = 125 cuentas por ms */
#define CUENTAS_POR_MS	125

/* Bits del timer0 en TCON (bits 0-3) */
#define T0_START		(1 << 0)
#define T0_MANUAL_UPD	(1 << 1)
#define T0_INVERTER		(1 << 2)
#define T0_AUTO_RELOAD	(1 << 3)
#define T0_BITS			(T0_START | T0_MANUAL_UPD | T0_INVERTER | T0_AUTO_RELOAD)

/* Bits del timer1 en TCON (bits 8-11) */
#define T1_START		(1 << 8)
#define T1_MANUAL_UPD	(1 << 9)
#define T1_INVERTER		(1 << 10)
#define T1_AUTO_RELOAD	(1 << 11)
#define T1_BITS			(T1_START | T1_MANUAL_UPD | T1_INVERTER | T1_AUTO_RELOAD)

/* declaracion de funcion que es rutina de servicio de interrupcion
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void timer_ISR(void) __attribute__((interrupt("IRQ")));
#if TEMPORIZADOR_ENTRADA == 1
static void timer1_ISR(void) __attribute__((interrupt("IRQ")));
#endif

/*--- codigo de las funciones ---*/
void timer_ISR(void)
{
	boton_timer0();		// maquina antirrebotes (button.c)

	/* borrar bit en I_ISPC: es de solo escritura, se asigna (no |=) */
	rI_ISPC = BIT_TIMER0;
}

void timer_init(void)
{
	/* INTMOD/INTCON ya los configura sys_init: aqui solo desenmascaramos los timers */

	/* Establece la rutina de servicio para TIMER0 */
	pISR_TIMER0 = (unsigned) timer_ISR;
	rINTMSK &= ~(BIT_TIMER0);

	/* Configura el Timer0 tocando solo sus bits (no machacar timer1-5, timer2 da las marcas de tiempo) */
	rTCFG0 = (rTCFG0 & ~0xFF) | 255;	// preescalado timers 0-1 = 255 (bits 7:0)
	rTCFG1 = rTCFG1 & ~0xF;				// divisor timer0 = 1/2 (bits 3:0 = 0000)
	rTCMPB0 = 0;
	rTCON &= ~T0_BITS;					// timer0 parado: lo arranca timer0_programar cuando hace falta

#if TEMPORIZADOR_ENTRADA == 1
	/* Timer1 con el mismo preescalado que el timer0 (ya puesto arriba) */
	pISR_TIMER1 = (unsigned) timer1_ISR;
	rINTMSK &= ~(BIT_TIMER1);
	rTCFG1 &= ~(0xF << 4);				// divisor timer1 = 1/2 (bits 7:4 = 0000)
	rTCMPB1 = 0;
	rTCON &= ~T1_BITS;					// parado hasta que la entrada lo programe
#endif
}

void timer0_programar(unsigned int ms)
{
	rTCNTB0 = ms * CUENTAS_POR_MS;
	rTCON = (rTCON & ~T0_BITS) | T0_MANUAL_UPD;		// carga TCNTB0 en el contador
	rTCON = (rTCON & ~T0_BITS) | T0_START;			// arranca sin auto-reload: un solo disparo
}

void temporizador_entrada_vencido(void)
{
#if !TEST_REBOTES
	entrada_temporizador();
#endif
}

#if TEMPORIZADOR_ENTRADA == 1
/*--- temporizador de la entrada con el timer1 ---*/
static void timer1_ISR(void)
{
	temporizador_entrada_vencido();

	rI_ISPC = BIT_TIMER1;
}

void temporizador_entrada_programar(unsigned int ms, int periodico)
{
	rTCNTB1 = ms * CUENTAS_POR_MS;
	rTCON = (rTCON & ~T1_BITS) | T1_MANUAL_UPD;		// carga TCNTB1 en el contador
	rTCON = (rTCON & ~T1_BITS) | T1_START | (periodico ? T1_AUTO_RELOAD : 0);
}

void temporizador_entrada_parar(void)
{
	rTCON &= ~T1_START;
	rI_ISPC = BIT_TIMER1;		// descarta un aviso que ya estuviera pendiente
}

#else
/*--- temporizador de la entrada con la interrupcion del timer2 (ver timer2.c) ---*/
void temporizador_entrada_programar(unsigned int ms, int periodico)
{
	timer2_aviso_programar(ms, periodico);
}

void temporizador_entrada_parar(void)
{
	timer2_aviso_parar();
}
#endif
