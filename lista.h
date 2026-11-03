/*********************************************************************************************
* Fichero:		lista.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		pila de depuracion: lista circular de los ultimos 150 eventos
* Version:	2.0
*********************************************************************************************/

#ifndef _LISTA_H_
#define _LISTA_H_

#include <stdint.h>

/*--- constantes ---*/
#define NUM_EVENTOS		150
/* Al final de la RAM, por debajo de las pilas de modo (SVC tiene su cima en 0x0C7FF100 y crece hacia abajo).
 * Ocupa 150 * 8 B = 1200 B (0x4B0): de 0x0C7FE000 a 0x0C7FE4AF, deja ~3 KB a la pila SVC */
#define PILA_DEP_BASE	0x0C7FE000

/* Una entrada: 8 B alineados a palabra. Se ve en la vista Memory como 2 palabras: ID | instante */
typedef struct {
	uint32_t id;			// uint8_t guardado en una palabra para que sea facil de leer en memoria
	uint32_t instante;		// us, de timer2_leer()
} evento_t;

/*--- declaracion de funciones visibles del modulo lista.c/lista.h ---*/
void lista_inicializar(void);								// vacia la pila (todo a 0) y vuelve al principio
void guardar_eventos(uint8_t ID_evento, uint32_t instante);	// apila un evento; al llegar a 150 da la vuelta

#endif /* _LISTA_H_ */
