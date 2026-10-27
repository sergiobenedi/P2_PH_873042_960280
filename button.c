/*********************************************************************************************
* Fichero:	button.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:	Funciones de manejo de los pulsadores (EINT6-7)
* Version:	1.0
*********************************************************************************************/

/*--- ficheros de cabecera ---*/
#include "button.h"
#include "44blib.h"
#include "44b.h"
#include "def.h"

/* bits de los pulsadores en EXTINTPND */
#define PND_EINT6	0x4
#define PND_EINT7	0x8

/* declaración de función que es rutina de servicio de interrupción
 * https://gcc.gnu.org/onlinedocs/gcc/ARM-Function-Attributes.html */
void Eint4567_ISR(void) __attribute__((interrupt("IRQ")));

/*--- codigo de funciones ---*/
void Eint4567_ISR(void)
{
	/* Identificar la interrupcion mirando cada bit (pueden llegar los dos a la vez) */
	unsigned int pnd = rEXTINTPND & (PND_EINT6 | PND_EINT7);

	if (pnd & PND_EINT6)
	{
		/* (botón EINT6: tratamiento en el Paso 5) */
	}
	if (pnd & PND_EINT7)
	{
		/* (botón EINT7: tratamiento en el Paso 5) */
	}

	/* Finalizar ISR: borrar solo lo atendido */
	rEXTINTPND = pnd;				// borra los bits atendidos en EXTINTPND (escribiendo 1)
	rI_ISPC    = BIT_EINT4567;		// I_ISPC es de solo escritura: se asigna (no |=)
}

void Eint4567_init(void)
{
	/* INTMOD/INTCON y el borrado general de INTPND ya los hace sys_init */

	/* Configuracion del puerto G: solo pines 6 y 7 */
	rPCONG  |= 0xF000;							// pines 6-7 (bits 15:12) = 11 -> EINT6/EINT7
	rPUPG   &= ~0xC0;							// pull-up activado en pines 6-7 (0 = activo)
	rEXTINT = (rEXTINT & ~0xFF000000) | 0x22000000;	// EINT6/7 por flanco de bajada (010)

	/* Establece la rutina de servicio para Eint4567 */
	pISR_EINT4567 = (int) Eint4567_ISR;

	/* Borrar pendientes antes de desenmascarar */
	rEXTINTPND = PND_EINT6 | PND_EINT7;
	rI_ISPC    = BIT_EINT4567;
	rINTMSK   &= ~(BIT_EINT4567);		// habilitamos la linea eint4567 en el vector de mascaras
}
