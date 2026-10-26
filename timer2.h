/*********************************************************************************************
* Fichero:		timer2.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		librería de medición de tiempos con el timer2 del s3c44b0x
* Version:	1.0
*********************************************************************************************/

#ifndef _TIMER2_H_
#define _TIMER2_H_

/*--- declaración de funciones visibles del módulo timer2.c/timer2.h ---*/
void timer2_inicializar(void);		// configura el timer2 a máxima precisión (1/32 us) con IRQ en cada cuenta completa
void timer2_empezar(void);			// reinicia la cuenta y el nº de interrupciones y arranca el timer2
unsigned int timer2_leer(void);		// devuelve los us transcurridos desde timer2_empezar
unsigned int timer2_parar(void);	// para el timer2 y devuelve los us transcurridos

#endif /* _TIMER2_H_ */
