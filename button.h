/*********************************************************************************************
* Fichero:	button.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	Pulsadores (EINT6-7) y maquina de estados antirrebotes
* Version:	3.0
*********************************************************************************************/

#ifndef _BUTTON_H_
#define _BUTTON_H_

/*--- constantes ---*/
/* 1 = modo de test de rebotes (paso 9 del guion): la RSI solo apila eventos (flancos de bajada y
 * de subida), sin antirrebotes ni aviso a la entrada de jugada. 0 = funcionamiento normal */
#define TEST_REBOTES	0

/* Identificador del boton que se avisa a la entrada de jugada. BOTON_TECLADO = teclado matricial
 * (opcional), que pasa por la misma maquina antirrebotes */
enum { BOTON_IZQ, BOTON_DER, BOTON_TECLADO };

/*--- declaracion de funciones visibles del modulo button.c/button.h ---*/
void Eint4567_init(void);	// pulsadores y, si TECLADO (teclado.h), tambien el teclado por EINT1
void boton_timer0(void);	// la llama la RSI del timer0: avanza la maquina antirrebotes

#endif /* _BUTTON_H_ */
