/*********************************************************************************************
* Fichero:		reversi8_arm.asm
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		versiones en ensamblador ARM de color_ficha() y patron_volteo()
*				(misma estructura que el C de reversi8_2026.c, cumpliendo ATPCS)
* Version:	1.0
*********************************************************************************************/

.text
.arm

/*******************************************************************************************
* Constantes (mismos valores que en reversi8_2026.c)
*******************************************************************************************/
.equ DIM,				8
.equ CASILLA_VACIA,		0
.equ NO_HAY_MODELO,		0
.equ MODELO_ENCONTRADO,	1

/*******************************************************************************************
* NOTA: en arm-none-eabi-gcc "char" es SIN signo. Un indice negativo (p.ej. FA = 0 + (-1))
* queda como 0xFF, por eso basta una comparacion sin signo (< DIM) para cubrir
* tambien la condicion ">= 0", y tras cada suma hay que truncar a 8 bits (and #0xFF).
*******************************************************************************************/

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
* Marco de pila (como el del compilador):
* 	[fp]		pc
* 	[fp, #-4]	lr
* 	[fp, #-8]	sp (ip)
* 	[fp, #-12]	fp anterior
*******************************************************************************************/
.global color_ficha_arm
color_ficha_arm:
		mov		ip, sp
		stmdb	sp!, {fp, ip, lr, pc}
		sub		fp, ip, #4

		cmp		r1, #DIM				/* sin signo: cubre f < DIM y f >= 0 */
		bhs		cf_else
		cmp		r2, #DIM				/* sin signo: cubre c < DIM y c >= 0 */
		bhs		cf_else
		add		r0, r0, r1, lsl #3		/* r0 = &tablero[f][0] (fila de DIM = 8 = 2^3 bytes) */
		ldrb	r0, [r0, r2]			/* ficha = tablero[f][c] */
		cmp		r0, #CASILLA_VACIA
		beq		cf_else

		mov		r1, #1
		str		r1, [r3]
		ldmdb	fp, {fp, sp, pc}

cf_else:
		mov		r1, #0
		str		r1, [r3]
		mov		r0, #CASILLA_VACIA
		ldmdb	fp, {fp, sp, pc}

/*******************************************************************************************
* Codigo C original:
*
* int patron_volteo(char tablero[][DIM], int *longitud, char FA, char CA, char SF, char SC, char color)
* {
*     int es_posicion_correcta;
*     char casilla;
*
*     FA = FA + SF;
*     CA = CA + SC;
*     casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
*     while ((es_posicion_correcta == 1) && (casilla != color))
*     {
*         FA = FA + SF;
*         CA = CA + SC;
*         *longitud = *longitud + 1;
*         casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
*     }
*     if ((es_posicion_correcta == 1) && (casilla == color))
*         return MODELO_ENCONTRADO;
*     else
*         return NO_HAY_MODELO;
* }
*
* Entrada:	r0 = tablero
* 			r1 = longitud (puntero)
* 			r2 = FA
* 			r3 = CA
* 			[fp, #4]	SF		(5º parametro, en pila del llamador)
* 			[fp, #8]	SC		(6º parametro)
* 			[fp, #12]	color	(7º parametro)
* Salida:	r0 = MODELO_ENCONTRADO / NO_HAY_MODELO
*
* Uso de registros (r4-r10 porque sobreviven a la llamada a color_ficha):
* 	r4 = tablero	r5 = longitud	r6 = FA		r7 = CA
* 	r8 = SF			r9 = SC			r10 = color	r0 = casilla (valor devuelto)
*
* Marco de pila:
* 	[fp]			pc
* 	[fp, #-4]		lr
* 	[fp, #-8]		sp (ip)
* 	[fp, #-12]		fp anterior
* 	[fp, #-16]		r10
* 	...
* 	[fp, #-40]		r4
* 	[fp, #-44] = [sp]	es_posicion_correcta: va en pila porque se pasa su direccion a color_ficha
*******************************************************************************************/
.global patron_volteo_arm
.extern color_ficha
patron_volteo_arm:
		mov		ip, sp
		stmdb	sp!, {r4-r10, fp, ip, lr, pc}
		sub		fp, ip, #4
		sub		sp, sp, #4				/* int es_posicion_correcta -> [sp] */

		mov		r4, r0					/* r4 = tablero */
		mov		r5, r1					/* r5 = longitud */
		ldrb	r8, [fp, #4]			/* r8 = SF */
		ldrb	r9, [fp, #8]			/* r9 = SC */
		ldrb	r10, [fp, #12]			/* r10 = color */

		add		r6, r2, r8
		and		r6, r6, #0xFF			/* FA es char: truncar a 8 bits */
		add		r7, r3, r9
		and		r7, r7, #0xFF
		mov		r0, r4
		mov		r1, r6
		mov		r2, r7
		mov		r3, sp
		bl		color_ficha				/* cambiar por color_ficha_arm para probar la version ARM */

pv_while:
		ldr		r1, [sp]
		cmp		r1, #1
		bne		pv_fin_while
		cmp		r0, r10
		beq		pv_fin_while

		add		r6, r6, r8
		and		r6, r6, #0xFF
		add		r7, r7, r9
		and		r7, r7, #0xFF
		ldr		r1, [r5]
		add		r1, r1, #1
		str		r1, [r5]
		mov		r0, r4
		mov		r1, r6
		mov		r2, r7
		mov		r3, sp
		bl		color_ficha
		b		pv_while

pv_fin_while:
		ldr		r1, [sp]
		cmp		r1, #1
		cmpeq	r0, r10
		moveq	r0, #MODELO_ENCONTRADO
		movne	r0, #NO_HAY_MODELO
		ldmdb	fp, {r4-r10, fp, sp, pc}

.end
