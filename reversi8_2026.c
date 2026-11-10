/*********************************************************************************************
* Fichero:		reversi8_2026.c
* Autor:		Sergio Benedi 873042, Pablo Gimeno 960280
* Descrip:		logica del juego Reversi de 8 casillas
* Version:	2.0
*********************************************************************************************/

#include "reversi8_2026.h"
#include "entrada.h"

// Tamano del tablero
enum { DIM=8 };

// Valores que puede devolver la funcion patron_volteo())
enum {
	NO_HAY_MODELO = 0,
  	MODELO_ENCONTRADO = 1
};

// Estados de las casillas del tablero
// deberia ser enum, lo dejamos como const para usar char...
const char CASILLA_VACIA = 0;
const char FICHA_BLANCA  = 1;
const char FICHA_NEGRA   = 2;

// candidatas: indica las posiciones a explorar
// Se usa para no explorar todo el tablero innecesariamente
// Sus posibles valores son NO, SI, CASILLA_OCUPADA
const char  NO              = 0;
const char  SI              = 1;
const char  CASILLA_OCUPADA = 2;

/////////////////////////////////////////////////////////////////////////////
// TABLAS AUXILIARES
// declaramos las siguientes tablas como globales para que sean mas faciles visualizarlas en el simulador
// __attribute__ ((aligned (8))): specifies a minimum alignment for the variable or structure field, measured in bytes, in this case 8 bytes

static const char __attribute__ ((aligned (8))) tabla_valor[DIM][DIM] =
{
    {8,2,7,3,3,7,2,8},
    {2,1,4,4,4,4,1,2},
    {7,4,6,5,5,6,4,7},
    {3,4,5,0,0,5,4,3},
    {3,4,5,0,0,5,4,3},
    {7,4,6,5,5,6,4,7},
    {2,1,4,4,4,4,1,2},
    {8,2,7,3,3,7,2,8}
};


// Tabla de direcciones. Contiene los desplazamientos de las 8 direcciones posibles
// signed char: en ARM "char" es sin signo y -1 valdria 255 (voltear escribiria fuera del tablero)
const signed char vSF[DIM] = {-1,-1, 0, 1, 1, 1, 0,-1};
const signed char vSC[DIM] = { 0, 1, 1, 1, 0,-1,-1,-1};

//////////////////////////////////////////////////////////////////////////////////////
// Variables globales que no deberian serlo
// tablero es una variable que se deberia definir como local dentro de reversi8.
// Sin embargo, la hemos definido como global para que sea mas facil visualizar el tablero en la memoria
//////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// Tablero sin inicializar
////////////////////////////////////////////////////////////////////
// En C un "const char" no es constante de compilacion; init_table() lo pone a CASILLA_VACIA
static char __attribute__ ((aligned (8))) tablero[DIM][DIM];



// Versiones en ensamblador (reversi8_arm.asm)
extern int patron_volteo_arm(char tablero[][8], int *longitud, char f, char c, char SF, char SC, char color);

// Version de patron_volteo que usa el juego: la ARM, que llama a color_ficha_thumb (la mas eficiente en la P1)
#define PATRON_VOLTEO patron_volteo_arm

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 0 indica CASILLA_VACIA, 1 indica FICHA_BLANCA y 2 indica FICHA_NEGRA
// pone el tablero a cero y luego coloca las fichas centrales.

static void init_table(char tablero[][DIM], char candidatas[][DIM])
{
    int i, j;

    for (i = 0; i < DIM; i++)
    {
        for (j = 0; j < DIM; j++)
            tablero[i][j] = CASILLA_VACIA;
    }
    tablero[3][3] = FICHA_BLANCA;
    tablero[4][4] = FICHA_BLANCA;
    tablero[3][4] = FICHA_NEGRA;
    tablero[4][3] = FICHA_NEGRA;

    candidatas[3][3] = CASILLA_OCUPADA;
    candidatas[4][4] = CASILLA_OCUPADA;
    candidatas[3][4] = CASILLA_OCUPADA;
    candidatas[4][3] = CASILLA_OCUPADA;

    // casillas a explorar:
    candidatas[2][2] = SI;
    candidatas[2][3] = SI;
    candidatas[2][4] = SI;
    candidatas[2][5] = SI;
    candidatas[3][2] = SI;
    candidatas[3][5] = SI;
    candidatas[4][2] = SI;
    candidatas[4][5] = SI;
    candidatas[5][2] = SI;
    candidatas[5][3] = SI;
    candidatas[5][4] = SI;
    candidatas[5][5] = SI;
}


////////////////////////////////////////////////////////////////////////////////
// voltea n fichas en la direccion que toque
// SF y SC son las cantidades a sumar para movernos en la direccion que toque
// color indica el color de la pieza que se acaba de colocar
// FA y CA son la fila y columna a analizar

static void voltear(char tablero[][DIM], int FA, int CA, signed char SF, signed char SC, int n, char color)
{
    int i;

    for (i = 0; i < n; i++)
    {
        FA = FA + SF;
        CA = CA + SC;
        tablero[FA][CA] = color;
    }
}

////////////////////////////////////////////////////////////////////////////////
// comprueba si hay que actualizar alguna ficha
// no comprueba que el movimiento realizado sea valido
// f y c son la fila y columna a analizar
// char vSF[DIM] = {-1,-1, 0, 1, 1, 1, 0,-1};
// char vSC[DIM] = { 0, 1, 1, 1, 0,-1,-1,-1};

static int actualizar_tablero(char tablero[][DIM], char f, char c, char color)
{
    signed char SF, SC; // cantidades a sumar para movernos en la direccion que toque
    int i, flip, modelo;

    for (i = 0; i < DIM; i++) // 0 es Norte, 1 NE, 2 E ...
    {
        SF = vSF[i];
        SC = vSC[i];
        // flip: numero de fichas a voltear
        flip = 0;
        modelo = PATRON_VOLTEO(tablero, &flip, f, c, SF, SC, color);
        //printf("Flip: %d \n", flip);
        if (modelo == MODELO_ENCONTRADO )
        {
            voltear(tablero, f, c, SF, SC, flip, color);
        }
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// Recorre todo el tablero comprobando en cada posicion si se puede mover
// En caso afirmativo, consulta la puntuacion de la posicion y si es la mejor
// que se ha encontrado la guarda
// Al acabar escribe el movimiento seleccionado en f y c

// Candidatas
// NO    0
// SI    1
// CASILLA_OCUPADA 2

static int elegir_mov(char candidatas[][DIM], char tablero[][DIM], char *f, char *c)
{
    int i, j, k, found;
    int mf = -1; // almacena la fila del mejor movimiento encontrado
    int mc;      // almacena la columna del mejor movimiento encontrado
    char mejor = 0; // almacena el mejor valor encontrado
    int modelo, longitud;
    char SF, SC; // cantidades a sumar para movernos en la direccion que toque

    // Recorremos todo el tablero comprobando donde podemos mover
    // Comparamos la puntuacion de los movimientos encontrados y nos quedamos con el mejor
    for (i=0; i<DIM; i++)
    {
        for (j=0; j<DIM; j++)
        {   // indica en que casillas quiza se pueda mover
            if (candidatas[i][j] == SI)
            {
                if (tablero[i][j] == CASILLA_VACIA)
                {
                    found = 0;
                    k = 0;

                    // en este bucle comprobamos si es un movimiento valido
                    // (es decir si implica voltear en alguna direccion)
                    while ((found == 0) && (k < DIM))
                    {
                        SF = vSF[k];    // k representa la direccion que miramos
                        SC = vSC[k];    // 1 es norte, 2 NE, 3 E ...

                        // nos dice que hay que voltear en cada direccion
                        longitud = 0;
                        modelo = PATRON_VOLTEO(tablero, &longitud, i, j, SF, SC, FICHA_BLANCA);
                        //  //printf("%d ", modelo);
                        if (modelo == MODELO_ENCONTRADO)
                        {
                            found = 1;
                            if (tabla_valor[i][j] > mejor)
                            {
                                mf = i;
                                mc = j;
                                mejor = tabla_valor[i][j];
                            }
                        }
                        k++;
                        // si no hemos encontrado nada probamos con la siguiente direccion
                    }
                }
            }
        }
    }
    *f = (char) mf;
    *c = (char) mc;
    // si no se ha encontrado una posicion valida devuelve -1
    return mf;
}

////////////////////////////////////////////////////////////////////////////////
// Cuenta el numero de fichas de cada color.
// Los guarda en la direccion b (blancas) y n (negras)

static void contar(char tablero[][DIM], int *b, int *n)
{
    int i,j;

    *b = 0;
    *n = 0;

    // recorremos todo el tablero contando las fichas de cada color
    for (i=0; i<DIM; i++)
    {
        for (j=0; j<DIM; j++)
        {
            if (tablero[i][j] == FICHA_BLANCA)
            {
                (*b)++;
            }
            else if (tablero[i][j] == FICHA_NEGRA)
            {
                (*n)++;
            }
        }
    }
}

static void actualizar_candidatas(char candidatas[][DIM], int f, int c)
{
    // donde ya se ha colocado no se puede volver a colocar
    // En las posiciones alrededor si
    candidatas[f][c] = CASILLA_OCUPADA;
    if (f > 0)
    {
        if (candidatas[f-1][c] != CASILLA_OCUPADA)
            candidatas[f-1][c] = SI;

        if ((c > 0) && (candidatas[f-1][c-1] != CASILLA_OCUPADA))
            candidatas[f-1][c-1] = SI;

        if ((c < 7) && (candidatas[f-1][c+1] != CASILLA_OCUPADA))
            candidatas[f-1][c+1] = SI;
    }
    if (f < 7)
    {
        if (candidatas[f+1][c] != CASILLA_OCUPADA)
            candidatas[f+1][c] = SI;

        if ((c > 0) && (candidatas[f+1][c-1] != CASILLA_OCUPADA))
            candidatas[f+1][c-1] = SI;

        if ((c < 7) && (candidatas[f+1][c+1] != CASILLA_OCUPADA))
            candidatas[f+1][c+1] = SI;
    }
    if ((c > 0) && (candidatas[f][c-1] != CASILLA_OCUPADA))
        candidatas[f][c-1] = SI;

    if ((c < 7) && (candidatas[f][c+1] != CASILLA_OCUPADA))
        candidatas[f][c+1] = SI;
}




////////////////////////////////////////////////////////////////////////////////
// Proceso principal del juego
// Utiliza el tablero global (deberia ser local de reversi8; es global para verlo junto en memoria)
// y la fila y columna que da la maquina de estados de entrada (entrada.c)
// en esta version el humano lleva negras y la maquina blancas
// no se comprueba que el humano mueva correctamente.
// Solo que la maquina realice un movimiento correcto.

void reversi8()
{

	 ////////////////////////////////////////////////////////////////////
	 // Tablero candidatas: se usa para no explorar todas las posiciones del tablero
	// solo se exploran las que estan alrededor de las fichas colocadas
	 ////////////////////////////////////////////////////////////////////
	char __attribute__ ((aligned (8))) candidatas[DIM][DIM] =
    {
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO},
        {NO,NO,NO,NO,NO,NO,NO,NO}
    };


    int done;     // la maquina ha conseguido mover o no
    int move = 0; // el humano ha conseguido mover o no
    int blancas, negras; // numero de fichas de cada color
    int fin = 0;  // fin vale 1 si el humano no ha podido mover
                  // (ha introducido un valor de movimiento con algun 8)
                  // y luego la maquina tampoco puede
    char f, c;    // fila y columna elegidas por la maquina para su movimiento
    int fila, columna;  // jugada del humano (0-7), de la maquina de estados de entrada
    int vacias = DIM*DIM - 4; // casillas en blanco: el tablero empieza con 4 fichas

    init_table(tablero, candidatas);

    while (fin == 0 && vacias > 0)
    {
        move = 0;
        // Espera la jugada: bucle en el programa principal, la RSI solo avanza la maquina de entrada
        while (!entrada_jugada_lista(&fila, &columna)) {}
        // si la fila o columna son 8 asumimos que el jugador no puede mover.
        // Sin "paso" por ahora (pendiente de preguntar al profesor): la entrada solo da 0-7,
        // asi que move vale siempre 1 y fin no llega a ser 1
        if (((fila) != DIM) && ((columna) != DIM))
        {
            if (tablero[(int)fila][(int)columna] == CASILLA_VACIA)
                vacias--;   // no se valida la jugada: solo cuenta si la casilla estaba libre
            tablero[(int)fila][(int)columna] = FICHA_NEGRA;
            actualizar_tablero(tablero, fila, columna, FICHA_NEGRA);
            actualizar_candidatas(candidatas, fila, columna);
            move = 1;
            if (vacias == 0)
                break;  // tablero lleno: la maquina ya no puede mover
        }

        // escribe el movimiento en las variables globales fila columna
        done = elegir_mov(candidatas, tablero, &f, &c);
        if (done == -1)
        {
            if (move == 0)
                fin = 1;
        }
        else
        {
            tablero[(int)f][(int)c] = FICHA_BLANCA;
            actualizar_tablero(tablero, f, c, FICHA_BLANCA);
            actualizar_candidatas(candidatas, f, c);
            vacias--;
        }
    }
    contar(tablero, &blancas, &negras);
}
