/*********************************************************************************************
* Fichero:		timer.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		timer0 (retardos del antirrebotes) y temporizador de la entrada de jugada
* Version:	3.0
*********************************************************************************************/

#ifndef _TIMER_H_
#define _TIMER_H_

/*--- constantes ---*/
/* Temporizador de la entrada de jugada (0,5 s, 300 ms y parpadeo). A decidir:
 *   1 = timer1 del S3C44B0X (el que pide el guion). Maximo 524 ms por disparo.
 *   2 = interrupcion del timer2 de timer2.c (la de las marcas de tiempo), con tic de 2,048 ms */
#define TEMPORIZADOR_ENTRADA	1

/*--- declaracion de funciones visibles del modulo timer.c/timer.h ---*/
void timer_init(void);							// timer0 y, si hace falta, el temporizador de la entrada

/* timer0 de un solo disparo: avisa a boton_timer0() al vencer. ms <= 524 (16 bits a 125 kHz) */
void timer0_programar(unsigned int ms);

/* Temporizador de la entrada: avisa a entrada_temporizador() al vencer.
 * periodico = 0: un solo disparo; 1: se repite cada ms. Reprogramar sustituye al anterior */
void temporizador_entrada_programar(unsigned int ms, int periodico);
void temporizador_entrada_parar(void);

/* Lo llama quien vence el temporizador (la RSI del timer1 o la del timer2) */
void temporizador_entrada_vencido(void);

#endif /* _TIMER_H_ */
