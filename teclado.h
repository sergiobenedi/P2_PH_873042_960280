/*********************************************************************************************
* Fichero:		teclado.h
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		teclado matricial 4x4 (apartado opcional): escaneo y teclas habilitadas
* Version:	1.0
*********************************************************************************************/

#ifndef _TECLADO_H_
#define _TECLADO_H_

/*--- constantes ---*/
/* 1 = teclado matricial habilitado (EINT1) ademas de los pulsadores. 0 = solo pulsadores */
#define TECLADO		1

/* Valor de las teclas habilitadas (figura 2 del guion). Los numeros valen su propio valor (1-8) */
enum {
	TECLA_NINGUNA = 0,		// ninguna tecla pulsada, o una de las 6 no habilitadas
	TECLA_OK = 9,			// tecla de confirmar
	TECLA_NO = 10			// tecla de cancelar
};

/*--- declaracion de funciones visibles del modulo teclado.c/teclado.h ---*/
/* Escanea las 4 filas y devuelve la tecla pulsada (1-8, TECLA_OK, TECLA_NO o TECLA_NINGUNA).
 * Efecto colateral: al poner a 0 la fila de la tecla pulsada se vuelve a activar EINT1 */
int teclado_leer(void);

#endif /* _TECLADO_H_ */
