/*********************************************************************************************
* Fichero:		teclado.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		teclado matricial 4x4 (apartado opcional): escaneo y teclas habilitadas
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "teclado.h"

/*--- constantes ---*/
/* El teclado esta en el banco 3 (nGCS3, 0x0600_0000). Leer una de estas direcciones pone a 0
 * una fila (lineas A4-A1) y devuelve las 4 columnas en D3-D0 (0 = tecla pulsada en esa columna).
 * Orden de arriba a abajo segun la tabla 6.1 de Entradasalida.pdf (SB1, SB2, SB3, SB4) */
#define TECLADO_BASE	0x06000000
static const unsigned int direccion_fila[4] = { 0xFD, 0xFB, 0xF7, 0xEF };

/* Dato leido (D3-D0) con la tecla pulsada en cada columna, de izquierda a derecha */
static const unsigned char dato_columna[4] = { 0x7, 0xB, 0xD, 0xE };

/* Tecla de cada posicion [fila][columna]. Solo 10 habilitadas (figura 2 del guion):
 *   1 2 3 4
 *   5 6 7 8
 *   . . . .
 *   OK . . NO      (OK y NO en la ultima fila, como en la figura 2 del guion)
 * Fila 0 = arriba (0xFD, SB1) y columna 0 = izquierda (dato 0x7), segun la tabla 6.1 de
 * Entradasalida.pdf. Probado en placa el 2026-11-16 */
static const unsigned char tecla[4][4] = {
	{ 1,          2,             3,             4             },
	{ 5,          6,             7,             8             },
	{ TECLA_NINGUNA, TECLA_NINGUNA, TECLA_NINGUNA, TECLA_NINGUNA },
	{ TECLA_OK,   TECLA_NINGUNA, TECLA_NINGUNA, TECLA_NO      }
};

/*--- codigo de las funciones ---*/
int teclado_leer(void)
{
	int f, c;
	unsigned char dato;

	for (f = 0; f < 4; f++)
	{
		dato = *(volatile unsigned char *)(TECLADO_BASE + direccion_fila[f]) & 0x0F;
		for (c = 0; c < 4; c++)
			if (dato == dato_columna[c])
				return tecla[f][c];
	}
	return TECLA_NINGUNA;		// 0xF en las 4 filas, o dos teclas a la vez en una fila
}
