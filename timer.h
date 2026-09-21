/*********************************************************************************************
* Fichero:		timer.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		funciones de control del timer0 del s3c44b0x
* Version:	1.0
*********************************************************************************************/

#ifndef _TIMER_H_
#define _TIMER_H_

/*--- variables visibles del módulo timer.c/timer.h ---*/
int switch_leds;

/*--- declaracion de funciones visibles del módulo timer.c/timer.h ---*/
void timer_init(void);

#endif /* _TIMER_H_ */
