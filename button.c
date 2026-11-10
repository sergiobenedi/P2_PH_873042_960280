/*********************************************************************************************
* Fichero:	button.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	Pulsadores (EINT6-7) y maquina de estados antirrebotes
* Version:	3.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "button.h"
#include "entrada.h"
#include "teclado.h"
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
 * trd debe cubrir la rafaga al soltar; trp no se sube para no perder toques cortos.
 * Placa 7 (2026-11-10, 5 pulsaciones de 1 s por boton): rafagas de hasta 67,7 ms (izq) y un caso
 * de 119,6 ms (der); se suben trp y trd a 100 ms. Toques de menos de 100 ms se descartan */
#define TRP_MS		100		// retardo de rebote de pulsacion
#define TRD_MS		100		// retardo de rebote de depresion
#define ENCUESTA_MS	50		// periodo de muestreo del boton mientras esta pulsado

/* El teclado (opcional) usa la misma maquina antirrebotes que los pulsadores. Solo se atiende una
 * entrada a la vez: mientras dura la maquina se enmascaran todas, asi el timer0 nunca se comparte.
 * En el modo de test de rebotes el teclado no se habilita */
#define USAR_TECLADO	(TECLADO && !TEST_REBOTES)
#if USAR_TECLADO
#define LINEAS_ENTRADA	(BIT_EINT4567 | BIT_EINT1)
#else
#define LINEAS_ENTRADA	BIT_EINT4567
#endif

/* declaracion de funcion que es rutina de servicio de interrupcion
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void Eint4567_ISR(void) __attribute__((interrupt("IRQ")));
#if USAR_TECLADO
static void Eint1_ISR(void) __attribute__((interrupt("IRQ")));
#endif

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

/* Distinto de 0 mientras la entrada activa esta pulsada. Pulsadores: estan a 0 en PDATG (pull-up).
 * Teclado: devuelve la tecla escaneada (las no habilitadas cuentan como no pulsadas) */
static int boton_pulsado(void)
{
#if USAR_TECLADO
	if (boton_activo == BOTON_TECLADO)
		return teclado_leer();
#endif
	return !(rPDATG & ((boton_activo == BOTON_IZQ) ? PIN_IZQ : PIN_DER));
}

/* T1 (comun a pulsadores y teclado): ESPERANDO -> RETARDO_TRP */
static void empezar_retardo_trp(int boton, uint8_t evento)
{
	boton_activo = boton;
	rINTMSK |= LINEAS_ENTRADA;		// los rebotes (y las demas entradas) ya no interrumpen
	apilar(evento);
	estado = RETARDO_TRP;
	timer0_programar(TRP_MS);
}

void Eint4567_ISR(void)
{
	/* Identificar la interrupcion mirando cada bit (pueden llegar los dos a la vez) */
	unsigned int pnd = rEXTINTPND & (PND_IZQ | PND_DER);

	/* Solo hay IRQ en ESPERANDO: en los demas estados la linea esta enmascarada */
	if (pnd && estado == ESPERANDO)
	{
		if (pnd & PND_IZQ)			// con los dos a la vez se atiende el izquierdo
			empezar_retardo_trp(BOTON_IZQ, EV_BOTON_IZQ);
		else
			empezar_retardo_trp(BOTON_DER, EV_BOTON_DER);
	}

	/* Finalizar ISR: borrar solo lo atendido */
	rEXTINTPND = pnd;				// borra los bits atendidos en EXTINTPND (escribiendo 1)
	rI_ISPC    = BIT_EINT4567;		// I_ISPC es de solo escritura: se asigna (no |=)
}

#if USAR_TECLADO
/* Una tecla del teclado matricial pone a 0 su columna -> flanco de bajada en EINT1. No se escanea
 * aqui: la tecla se lee al acabar trp, ya sin rebotes */
static void Eint1_ISR(void)
{
	if (estado == ESPERANDO)
		empezar_retardo_trp(BOTON_TECLADO, EV_TECLADO);

	rI_ISPC = BIT_EINT1;
}
#endif

/* T2a, T2b, T3, T4 y T5: la llama la RSI del timer0 cada vez que vence un retardo */
void boton_timer0(void)
{
	int valor;

	switch (estado)
	{
	case RETARDO_TRP:
		apilar(EV_TIMER0);
		valor = boton_pulsado();
		if (valor)						// T2a: pulsacion confirmada
		{
			estado = ENCUESTA;
			timer0_programar(ENCUESTA_MS);
			if (boton_activo == BOTON_TECLADO)
				entrada_tecla(valor);
			else
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

	case RETARDO_TRD:					// T5: reactivar los pulsadores (y el teclado)
		apilar(EV_FIN_TRD);
		estado = ESPERANDO;
		rEXTINTPND = PND_IZQ | PND_DER;		// primero borrar los rebotes de la suelta (y los EINT1
		rI_ISPC    = LINEAS_ENTRADA;		// que provoca el propio escaneo del teclado)...
		rINTMSK   &= ~(LINEAS_ENTRADA);		// ...y solo despues desenmascarar
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

#if USAR_TECLADO
	/* Teclado matricial: pin 1 del puerto G = EINT1, por flanco de bajada. El banco 3 (nGCS3)
	 * ya lo configura el arranque. Port_Init ya deja PCONG en modo EINT, se fuerza por si acaso */
	rPCONG  |= 0xC;										// pin 1 (bits 3:2) = 11 -> EINT1
	rEXTINT  = (rEXTINT & ~0x70) | 0x20;				// EINT1 (bits 6:4) por flanco de bajada (010)
	pISR_EINT1 = (int) Eint1_ISR;
	rI_ISPC  = BIT_EINT1;
	rINTMSK &= ~(BIT_EINT1);
#endif
}
