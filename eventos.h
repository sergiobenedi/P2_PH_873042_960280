/*********************************************************************************************
* Fichero:		eventos.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		identificadores de los eventos que se guardan en la pila de depuracion
* Version:	2.0
*********************************************************************************************/

#ifndef _EVENTOS_H_
#define _EVENTOS_H_

/* Un ID unico por evento. Se guardan como uint8_t: como mucho 256 */
enum {
	EV_NINGUNO = 0,			// hueco vacio de la pila (tras lista_inicializar)
	EV_BOTON_IZQ,			// EINT del pulsador izquierdo
	EV_BOTON_DER,			// EINT del pulsador derecho
	EV_BOTON_SOLTADO,		// la encuesta ve el boton a 1
	EV_TIMER0,				// interrupcion del timer0 (antirrebotes)
	EV_FIN_TRD,				// fin del retardo de depresion: se reactivan los pulsadores
	EV_TIMER1,				// interrupcion del timer1 (autoincremento / parpadeo)
	EV_JUGADA_CONFIRMADA,
	EV_JUGADA_CANCELADA,
	EV_TECLADO,				// opcional: teclado matricial
	EV_PRUEBA				// solo para probar la pila
};

#endif /* _EVENTOS_H_ */
