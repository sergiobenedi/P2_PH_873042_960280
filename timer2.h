/*********************************************************************************************
* Fichero:		timer2.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		libreria de medicion de tiempos con el timer2 del s3c44b0x
* Version:	2.0
*********************************************************************************************/

#ifndef _TIMER2_H_
#define _TIMER2_H_

/*--- declaracion de funciones visibles del modulo timer2.c/timer2.h ---*/
void timer2_inicializar(void);		// configura el timer2 a maxima precision (1/32 us) con IRQ en cada cuenta completa
void timer2_empezar(void);			// reinicia la cuenta y el num. de interrupciones y arranca el timer2
unsigned int timer2_leer(void);		// devuelve los us transcurridos desde timer2_empezar
unsigned int timer2_parar(void);	// para el timer2 y devuelve los us transcurridos

/* Aviso a temporizador_entrada_vencido() (timer.h) tras ms, contado en periodos del timer2 (2,048 ms).
 * Solo para TEMPORIZADOR_ENTRADA == 2. Error: el primer aviso puede adelantarse hasta un periodo */
void timer2_aviso_programar(unsigned int ms, int periodico);	// periodico = 0: un solo disparo
void timer2_aviso_parar(void);

#endif /* _TIMER2_H_ */
