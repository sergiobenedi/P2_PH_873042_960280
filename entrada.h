/*********************************************************************************************
* Fichero:		entrada.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		maquina de estados de la entrada de jugada (INICIO, FILA, COLUMNA, CONFIRMAR)
* Version:	1.0
*********************************************************************************************/

#ifndef _ENTRADA_H_
#define _ENTRADA_H_

#include "button.h"

/*--- funciones que llaman las RSI (siempre desde una IRQ, nunca desde Main) ---*/
void entrada_boton_pulsado(int boton);	// pulsacion confirmada (BOTON_IZQ / BOTON_DER)
void entrada_boton_soltado(int boton);	// el boton se ha soltado
void entrada_temporizador(void);		// ha vencido el temporizador de la entrada (ver timer.h)

/*--- funciones que llama el programa principal ---*/
void entrada_inicializar(void);			// 8led apagado y maquina en INICIO (despues de D8Led_init)

/* Si hay una jugada confirmada, la devuelve en fila y columna (indices 0-7), la consume y
 * devuelve 1. Si no, devuelve 0. Se llama en un bucle desde el programa principal */
int entrada_jugada_lista(int *fila, int *columna);

#endif /* _ENTRADA_H_ */
