/*********************************************************************************************
* Fichero:		lista.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		pila de depuracion: lista circular de los ultimos 150 eventos
* Version:	2.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "lista.h"

/*--- variables globales del modulo ---*/
/* La pila vive en una direccion fija (no en .bss) para encontrarla siempre en la vista Memory */
static volatile evento_t * const pila = (volatile evento_t *) PILA_DEP_BASE;

/* Siguiente hueco a escribir (el ultimo evento esta en indice - 1).
 * ponytail: sin exclusion mutua; vale mientras solo la llamen RSI (no se anidan).
 * Si la llama tambien el programa principal, deshabilitar IRQ alrededor de la escritura */
static volatile unsigned int indice = 0;

/*--- codigo de las funciones ---*/
void lista_inicializar(void)
{
	unsigned int i;

	for (i = 0; i < NUM_EVENTOS; i++)
	{
		pila[i].id = 0;
		pila[i].instante = 0;
	}
	indice = 0;
}

void guardar_eventos(uint8_t ID_evento, uint32_t instante)
{
	pila[indice].id = ID_evento;
	pila[indice].instante = instante;

	indice++;
	if (indice == NUM_EVENTOS)
		indice = 0;		// circular: el evento 151 pisa al mas antiguo
}
