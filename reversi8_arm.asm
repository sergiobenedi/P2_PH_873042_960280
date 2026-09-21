/*********************************************************************************************
* Fichero:		reversi8_arm.asm
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		versiones en ensamblador ARM de color_ficha() y patron_volteo()
*				(misma estructura que el C de reversi8_2026.c, cumpliendo ATPCS)
* Version:	1.0
*********************************************************************************************/

.text
.arm

# Constantes (mismos valores que en reversi8_2026.c)
.equ DIM,				8
.equ CASILLA_VACIA,		0
.equ NO_HAY_MODELO,		0
.equ MODELO_ENCONTRADO,	1

#############################################################################################
# char color_ficha_arm(char tablero[][DIM], char f, char c, int *es_posicion_correcta)
#
# Entrada:	r0 = tablero
#			r1 = f
#			r2 = c
#			r3 = es_posicion_correcta (puntero)
# Salida:	r0 = ficha
#
# Marco de pila (como el del compilador):
#	[fp]		pc
#	[fp, #-4]	lr
#	[fp, #-8]	sp (ip)
#	[fp, #-12]	fp anterior
#############################################################################################
.global color_ficha_arm
color_ficha_arm:
		mov		ip, sp
		stmdb	sp!, {fp, ip, lr, pc}
		sub		fp, ip, #4

		# TODO: if ((f < DIM) && (f >= 0) && (c < DIM) && (c >= 0) && (tablero[f][c] != CASILLA_VACIA))
		#			*es_posicion_correcta = 1; ficha = tablero[f][c];
		#		else
		#			*es_posicion_correcta = 0; ficha = CASILLA_VACIA;
		#		return ficha;

		ldmdb	fp, {fp, sp, pc}

#############################################################################################
# int patron_volteo_arm(char tablero[][DIM], int *longitud, char FA, char CA,
#						char SF, char SC, char color)
#
# Entrada:	r0 = tablero
#			r1 = longitud (puntero)
#			r2 = FA
#			r3 = CA
#			[fp, #4]	SF		(5º parámetro, en pila del llamador)
#			[fp, #8]	SC		(6º parámetro)
#			[fp, #12]	color	(7º parámetro)
# Salida:	r0 = MODELO_ENCONTRADO / NO_HAY_MODELO
#
# Marco de pila:
#	[fp]			pc
#	[fp, #-4]		lr
#	[fp, #-8]		sp (ip)
#	[fp, #-12]		fp anterior
#	[fp, #-16..]	r4-r10 (ajustar a los que realmente uséis)
#	[sp]			es_posicion_correcta: va en pila porque se pasa su dirección a color_ficha
#############################################################################################
.global patron_volteo_arm
.extern color_ficha
patron_volteo_arm:
		mov		ip, sp
		stmdb	sp!, {r4-r10, fp, ip, lr, pc}
		sub		fp, ip, #4
		sub		sp, sp, #4				/* int es_posicion_correcta */

		# TODO:
		#	FA = FA + SF; CA = CA + SC;
		#	casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
		#	while ((es_posicion_correcta == 1) && (casilla != color)) {
		#		FA = FA + SF; CA = CA + SC;
		#		*longitud = *longitud + 1;
		#		casilla = color_ficha(tablero, FA, CA, &es_posicion_correcta);
		#	}
		#	if ((es_posicion_correcta == 1) && (casilla == color)) return MODELO_ENCONTRADO;
		#	else return NO_HAY_MODELO;
		#
		# Llamada: r0 = tablero, r1 = FA, r2 = CA, r3 = sp (&es_posicion_correcta)
		#	bl	color_ficha		/* o bl color_ficha_arm */
		# Para color_ficha_thumb (ARMv4T no tiene blx): ldr ip, =color_ficha_thumb+1 ; mov lr, pc ; bx ip
		# OJO: FA/CA son char (con signo) -> tras sumar hay que extender el signo de 8 bits

		ldmdb	fp, {r4-r10, fp, sp, pc}

.end
