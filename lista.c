/*********************************************************************************************
* Fichero:		lista.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		pila de depuración: lista circular de los últimos 150 eventos
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "lista.h"

/*--- variables globales del módulo ---*/
/* La pila vive en una dirección fija (no en .bss) para encontrarla siempre en la vista Memory */
static volatile evento_t * const pila = (volatile evento_t *) PILA_DEP_BASE;

/* Siguiente hueco a escribir (el último evento está en indice - 1).
 * ponytail: sin exclusión mutua; vale mientras solo la llamen RSI (no se anidan).
 * Si la llama también el programa principal, deshabilitar IRQ alrededor de la escritura */
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
		indice = 0;		// circular: el evento 151 pisa al más antiguo
}
