/*********************************************************************************************
* Fichero:		reversi8_thumb.asm
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		version en ensamblador Thumb de color_ficha() (misma estructura que el C
*				de reversi8_2026.c y que color_ficha_arm, cumpliendo ATPCS)
* Version:	1.0
*********************************************************************************************/

.text
.thumb

/* Mismos valores que en reversi8_2026.c */
.equ DIM,				8
.equ CASILLA_VACIA,		0

/*******************************************************************************************
* Codigo C original:
*
* char color_ficha(char tablero[][DIM], char f, char c, int *es_posicion_correcta)
* {
*     char ficha;
*
*     if ((f < DIM) && (f >= 0) && (c < DIM) && (c >= 0) && (tablero[f][c] != CASILLA_VACIA))
*     {
*         *es_posicion_correcta = 1;
*         ficha = tablero[f][c];
*     }
*     else
*     {
*         *es_posicion_correcta = 0;
*         ficha = CASILLA_VACIA;
*     }
*     return ficha;
* }
*
* Entrada:	r0 = tablero
* 			r1 = f
* 			r2 = c
* 			r3 = es_posicion_correcta (puntero)
* Salida:	r0 = ficha
* Variables locales: ficha -> r0 (no necesita pila)
*
* Marco de pila: ninguno. Es una funcion hoja que solo usa r0-r3 (caller-saved) y no llama
* a nadie, asi que lr no se pisa y no hay nada que guardar (el guion permite reducir el
* marco en Thumb). Se retorna con "bx lr": como el llamador es ARM (bit 0 de lr = 0),
* bx vuelve a estado ARM. "pop {pc}" NO serviria: en ARMv4T no cambia de estado.
*
* Llamada desde ARM/C: basta un "bl color_ficha_thumb". Al ser .thumb_func, el enlazador
* inserta un veneer ARM->Thumb (carga la direccion con bit 0 = 1 y hace bx) porque ARMv4T
* no tiene blx. Se ve en el .map / desensamblado junto al simbolo color_ficha_thumb.
*
* Char sin signo (ver reversi8_arm.asm): una sola comparacion sin signo cubre
* f < DIM y f >= 0.
*******************************************************************************************/
.global color_ficha_thumb
.type	color_ficha_thumb, %function
.thumb_func
color_ficha_thumb:
		cmp		r1, #DIM				/* sin signo: cubre f < DIM y f >= 0 */
		bhs		cft_else
		cmp		r2, #DIM				/* sin signo: cubre c < DIM y c >= 0 */
		bhs		cft_else
		lsl		r1, r1, #3				/* Thumb no tiene desplazamiento integrado en add */
		add		r0, r0, r1				/* r0 = &tablero[f][0] */
		ldrb	r0, [r0, r2]			/* ficha = tablero[f][c] */
		cmp		r0, #CASILLA_VACIA
		beq		cft_else

		mov		r1, #1
		str		r1, [r3]				/* *es_posicion_correcta = 1 */
		bx		lr

cft_else:
		mov		r0, #CASILLA_VACIA		/* ficha = CASILLA_VACIA (= 0) */
		str		r0, [r3]				/* *es_posicion_correcta = 0 (reutiliza el 0 de r0) */
		bx		lr

.end
