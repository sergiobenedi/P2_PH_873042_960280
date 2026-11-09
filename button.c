/*********************************************************************************************
* Fichero:	button.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	Pulsadores (EINT6-7) y maquina de estados antirrebotes
* Version:	3.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "button.h"
#include "entrada.h"
#include "timer.h"
#include "timer2.h"
#include "lista.h"
#include "eventos.h"
#include "44blib.h"
#include "44b.h"
#include "def.h"

/* PROVISIONAL: SB2 va a EINT6 y SB3 a EINT7, pero esta SIN VERIFICAR en la placa cual es el izquierdo */
#define EINT_IZQ	6
#define EINT_DER	7

/* bit de cada pulsador en EXTINTPND (EINT4 = bit 0) y pin en PDATG */
#define PND_IZQ		(1 << (EINT_IZQ - 4))
#define PND_DER		(1 << (EINT_DER - 4))
#define PIN_IZQ		(1 << EINT_IZQ)
#define PIN_DER		(1 << EINT_DER)

/* Medidos con TEST_REBOTES (paso 9) en dos placas. Al pulsar: peor caso 41,5 ms (placa 12) y 38,2 ms
 * (placa 4, salvo un caso suelto de 94 ms). Al soltar: 12,4 ms (placa 12) y 38,2 ms (placa 4).
 * trd debe cubrir la rafaga al soltar; trp no se sube para no perder toques cortos */
#define TRP_MS		60		// retardo de rebote de pulsacion
#define TRD_MS		50		// retardo de rebote de depresion
#define ENCUESTA_MS	50		// periodo de muestreo del boton mientras esta pulsado

/* declaracion de funcion que es rutina de servicio de interrupcion
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void Eint4567_ISR(void) __attribute__((interrupt("IRQ")));

#if TEST_REBOTES
/*--- modo de test: solo apila un evento por flanco, sin enmascarar ni esperar ---*/
void Eint4567_ISR(void)
{
	unsigned int pnd = rEXTINTPND & (PND_IZQ | PND_DER);
	unsigned int t = timer2_leer();

	/* nivel en PDATG: 0 = pulsado (flanco de bajada), 1 = suelto (flanco de subida) */
	if (pnd & PND_IZQ)
		guardar_eventos((rPDATG & PIN_IZQ) ? EV_BOTON_SOLTADO : EV_BOTON_IZQ, t);
	if (pnd & PND_DER)
		guardar_eventos((rPDATG & PIN_DER) ? EV_BOTON_SOLTADO : EV_BOTON_DER, t);

	rEXTINTPND = pnd;				// borra los bits atendidos en EXTINTPND (escribiendo 1)
	rI_ISPC    = BIT_EINT4567;		// I_ISPC es de solo escritura: se asigna (no |=)
}

void boton_timer0(void)
{
	/* en el modo de test el timer0 no se programa nunca */
}

#else
/*--- maquina de estados antirrebotes ---*/
enum { ESPERANDO, RETARDO_TRP, ENCUESTA, RETARDO_TRD };

/* Solo las tocan la RSI de EINT4567 y la del timer0 (que no se anidan): nadie mas */
static volatile int estado = ESPERANDO;
static volatile int boton_activo = BOTON_IZQ;

static void apilar(uint8_t id)
{
	guardar_eventos(id, timer2_leer());
}

/* El pulsador esta a 0 en PDATG mientras esta pulsado (pull-up) */
static int boton_pulsado(void)
{
	return !(rPDATG & ((boton_activo == BOTON_IZQ) ? PIN_IZQ : PIN_DER));
}

/* T1: ESPERANDO -> RETARDO_TRP */
void Eint4567_ISR(void)
{
	/* Identificar la interrupcion mirando cada bit (pueden llegar los dos a la vez) */
	unsigned int pnd = rEXTINTPND & (PND_IZQ | PND_DER);

	/* Solo hay IRQ en ESPERANDO: en los demas estados la linea esta enmascarada */
	if (pnd && estado == ESPERANDO)
	{
		boton_activo = (pnd & PND_IZQ) ? BOTON_IZQ : BOTON_DER;	// con los dos a la vez se atiende el izquierdo
		rINTMSK |= BIT_EINT4567;								// los rebotes ya no interrumpen
		apilar((boton_activo == BOTON_IZQ) ? EV_BOTON_IZQ : EV_BOTON_DER);
		estado = RETARDO_TRP;
		timer0_programar(TRP_MS);
	}

	/* Finalizar ISR: borrar solo lo atendido */
	rEXTINTPND = pnd;				// borra los bits atendidos en EXTINTPND (escribiendo 1)
	rI_ISPC    = BIT_EINT4567;		// I_ISPC es de solo escritura: se asigna (no |=)
}

/* T2a, T2b, T3, T4 y T5: la llama la RSI del timer0 cada vez que vence un retardo */
void boton_timer0(void)
{
	switch (estado)
	{
	case RETARDO_TRP:
		apilar(EV_TIMER0);
		if (boton_pulsado())			// T2a: pulsacion confirmada
		{
			estado = ENCUESTA;
			timer0_programar(ENCUESTA_MS);
			entrada_boton_pulsado(boton_activo);
		}
		else							// T2b: pulso espurio, sin aviso
		{
			estado = RETARDO_TRD;
			timer0_programar(TRD_MS);
		}
		break;

	case ENCUESTA:
		if (boton_pulsado())			// T3: sigue pulsado
		{
			apilar(EV_TIMER0);
			timer0_programar(ENCUESTA_MS);
		}
		else							// T4: soltado
		{
			apilar(EV_BOTON_SOLTADO);
			estado = RETARDO_TRD;
			timer0_programar(TRD_MS);
			entrada_boton_soltado(boton_activo);
		}
		break;

	case RETARDO_TRD:					// T5: reactivar los pulsadores
		apilar(EV_FIN_TRD);
		estado = ESPERANDO;
		rEXTINTPND = PND_IZQ | PND_DER;		// primero borrar los rebotes de la suelta...
		rI_ISPC    = BIT_EINT4567;
		rINTMSK   &= ~(BIT_EINT4567);		// ...y solo despues desenmascarar
		break;

	default:							// ESPERANDO: aviso del timer0 que no esperabamos
		break;
	}
}
#endif

void Eint4567_init(void)
{
	/* INTMOD/INTCON y el borrado general de INTPND ya los hace sys_init */

	/* Configuracion del puerto G: solo pines 6 y 7 */
	rPCONG  |= 0xF000;							// pines 6-7 (bits 15:12) = 11 -> EINT6/EINT7
	rPUPG   &= ~0xC0;							// pull-up activado en pines 6-7 (0 = activo)
#if TEST_REBOTES
	rEXTINT = (rEXTINT & ~0xFF000000) | 0x66000000;	// EINT6/7 por ambos flancos (110): se ven los rebotes al pulsar y al soltar
#else
	rEXTINT = (rEXTINT & ~0xFF000000) | 0x22000000;	// EINT6/7 por flanco de bajada (010)
#endif

	/* Establece la rutina de servicio para Eint4567 */
	pISR_EINT4567 = (int) Eint4567_ISR;

	/* Borrar pendientes antes de desenmascarar */
	rEXTINTPND = PND_IZQ | PND_DER;
	rI_ISPC    = BIT_EINT4567;
	rINTMSK   &= ~(BIT_EINT4567);		// habilitamos la linea eint4567 en el vector de mascaras
}
