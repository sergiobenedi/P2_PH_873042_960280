/*********************************************************************************************
* Fichero:		entrada.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		maquina de estados de la entrada de jugada (INICIO, FILA, COLUMNA, CONFIRMAR)
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "entrada.h"
#include "timer.h"
#include "timer2.h"
#include "lista.h"
#include "eventos.h"
#include "8led.h"
#include "led.h"
#include "teclado.h"

/*--- constantes ---*/
/* Indices de los simbolos en la tabla Symbol de 8led.c: D8Led_symbol recibe el indice, no el bitmap.
 * Los digitos 1-8 se muestran con su propio valor como indice */
#define SIMBOLO_C		12
#define SIMBOLO_F		15
#define SIMBOLO_BLANK	16

/* Led izquierdo de la placa = led1 (bit 9 de PDATB), verificado en la placa */
#define LED_ON()		led1_on()
#define LED_OFF()		led1_off()

#define MANTENIDO_MS	500		// pulsacion larga: pasado este tiempo empieza el autoincremento
#define AUTOINC_MS		300		// periodo del autoincremento
#define PARPADEO_MS		167		// medio periodo del parpadeo: 3 veces por segundo
#define CONMUTACIONES	18		// 3 s de parpadeo / 167 ms

/*--- variables globales del modulo ---*/
/* Solo las tocan las funciones de este modulo, que llaman las RSI (no se anidan) y Main
 * (solo entrada_jugada_lista) */
enum { INICIO, FILA, COLUMNA, CONFIRMAR };
enum { IZQ_LIBRE, IZQ_ESPERA, IZQ_AUTO };	// boton izquierdo: suelto, esperando los 0,5 s, autoincrementando

static volatile int estado = INICIO;
static volatile int subestado = IZQ_LIBRE;
static volatile int n = 0;					// valor elegido (1-8); 0 = aun no se ha pulsado el izquierdo
static volatile int fila_elegida = 0;		// 1-8
static volatile int columna_elegida = 0;	// 1-8
static volatile int parpadeos = 0;			// conmutaciones del led que quedan en CONFIRMAR
static volatile int led_encendido = 0;

/* La jugada la escribe la RSI (indices 0-7) y la lee Main. El aviso se escribe el ultimo */
static volatile int jugada_lista = 0;
static volatile int jugada_fila = 0;
static volatile int jugada_columna = 0;

/*--- codigo de las funciones ---*/
static void apilar(uint8_t id)
{
	guardar_eventos(id, timer2_leer());
}

static void siguiente_valor(void)
{
	n = n % 8 + 1;
	D8Led_symbol(n);
}

static void led_conmutar(void)
{
	led_encendido = !led_encendido;
	if (led_encendido)
		LED_ON();
	else
		LED_OFF();
}

/* Vuelve a pedir una fila nueva: 8led = F, sin valor elegido */
static void volver_a_fila(void)
{
	estado = FILA;
	subestado = IZQ_LIBRE;
	n = 0;
	D8Led_symbol(SIMBOLO_F);
}

/* Boton derecho con un valor elegido */
static void confirmar(void)
{
	if (estado == FILA)				// E2
	{
		fila_elegida = n;
		estado = COLUMNA;
		n = 0;
		D8Led_symbol(SIMBOLO_C);
	}
	else							// E3: el 8led sigue mostrando la columna
	{
		columna_elegida = n;
		estado = CONFIRMAR;
		parpadeos = CONMUTACIONES;
		led_encendido = 1;
		LED_ON();
		temporizador_entrada_programar(PARPADEO_MS, 1);
	}
}

/* E4a: se cancela la jugada mientras parpadea el led */
static void cancelar(void)
{
	temporizador_entrada_parar();
	led_encendido = 0;
	LED_OFF();
	apilar(EV_JUGADA_CANCELADA);
	volver_a_fila();
}

void entrada_inicializar(void)
{
	estado = INICIO;
	subestado = IZQ_LIBRE;
	n = 0;
	jugada_lista = 0;
	D8Led_symbol(SIMBOLO_BLANK);	// el guion pide el 8led apagado al principio
}

void entrada_boton_pulsado(int boton)
{
	switch (estado)
	{
	case INICIO:					// E1: cualquier boton empieza la partida
		volver_a_fila();
		break;

	case FILA:
	case COLUMNA:
		if (boton == BOTON_IZQ)
		{
			siguiente_valor();
			subestado = IZQ_ESPERA;
			temporizador_entrada_programar(MANTENIDO_MS, 0);
		}
		else if (n != 0)			// derecho: sin valor elegido no hay nada que confirmar
			confirmar();
		break;

	case CONFIRMAR:					// E4a: cualquier boton cancela la jugada
		cancelar();
		break;

	default:
		break;
	}
}

/* Teclado matricial (opcional): el numero se elige con una sola pulsacion. OK hace lo mismo que el
 * boton derecho, NO vuelve a pedir la fila. En CONFIRMAR solo NO cancela: asi un OK de mas no
 * deshace la jugada que se acaba de confirmar */
void entrada_tecla(int tecla)
{
	switch (estado)
	{
	case INICIO:					// E1: cualquier tecla empieza la partida
		volver_a_fila();
		break;

	case FILA:
	case COLUMNA:
		if (tecla == TECLA_OK)
		{
			if (n != 0)
				confirmar();
		}
		else if (tecla == TECLA_NO)
			volver_a_fila();
		else						// 1-8
		{
			n = tecla;
			D8Led_symbol(n);
		}
		break;

	case CONFIRMAR:
		if (tecla == TECLA_NO)
			cancelar();
		break;

	default:
		break;
	}
}

void entrada_boton_soltado(int boton)
{
	/* subestado solo deja de ser IZQ_LIBRE en FILA y COLUMNA */
	if (boton == BOTON_IZQ && subestado != IZQ_LIBRE)
	{
		temporizador_entrada_parar();
		subestado = IZQ_LIBRE;
	}
}

void entrada_temporizador(void)
{
	apilar(EV_TIMER1);

	switch (estado)
	{
	case FILA:
	case COLUMNA:
		if (subestado == IZQ_LIBRE)		// aviso que ya no esperabamos (llego tras parar el temporizador)
			break;
		if (subestado == IZQ_ESPERA)	// pasaron los 0,5 s: a partir de aqui cada 300 ms
		{
			subestado = IZQ_AUTO;
			temporizador_entrada_programar(AUTOINC_MS, 1);
		}
		siguiente_valor();
		break;

	case CONFIRMAR:
		led_conmutar();
		parpadeos--;
		if (parpadeos == 0)				// E4b: pasaron los 3 s sin cancelar
		{
			temporizador_entrada_parar();
			led_encendido = 0;
			LED_OFF();
			jugada_fila = fila_elegida - 1;			// el juego usa indices 0-7
			jugada_columna = columna_elegida - 1;
			jugada_lista = 1;
			apilar(EV_JUGADA_CONFIRMADA);
			volver_a_fila();
		}
		break;

	default:							// INICIO: aviso que no esperabamos
		break;
	}
}

int entrada_jugada_lista(int *fila, int *columna)
{
	if (!jugada_lista)
		return 0;

	/* Sin exclusion mutua: otra jugada necesita varios segundos de botones, no puede
	 * pisar estos valores entre la lectura y el borrado del aviso */
	*fila = jugada_fila;
	*columna = jugada_columna;
	jugada_lista = 0;
	return 1;
}
