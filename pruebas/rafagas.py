# Fichero:  rafagas.py
# Autor:    Sergio Benedi 873042, Pablo Gimeno 960280
# Descrip:  agrupa en rafagas un volcado hex de la pila de depuracion (TEST_REBOTES = 1)
#           Uso: python pruebas/rafagas.py volcado.txt [indice]
#           indice = valor de la variable indice de lista.c si la pila dio la vuelta
# Version:  1.0
import sys

HUECO_US = 30000  # ponytail: eventos separados menos de 30 ms = misma rafaga; subir si una rafaga sale partida

# acepta el volcado limpio o la salida de "x/300xw" de GDB (se quita la direccion antes de ':')
lineas = open(sys.argv[1], encoding="utf-8-sig").read().splitlines()
palabras = [int(p, 16) for l in lineas for p in l.split(":")[-1].split() if p.startswith("0x")]
ev = [(palabras[i], palabras[i + 1]) for i in range(0, len(palabras) - 1, 2) if palabras[i] != 0]
if len(sys.argv) > 2:  # pila circular: el mas antiguo esta en indice
    k = int(sys.argv[2])
    ev = ev[k:] + ev[:k]

NOMBRE = {1: "izq", 2: "der", 3: "soltado"}  # id del primer evento de la rafaga
rafagas = []
for id_, t in ev:
    if rafagas and t - rafagas[-1][-1][1] < HUECO_US:
        rafagas[-1].append((id_, t))
    else:
        rafagas.append([(id_, t)])

# ponytail: no se distingue pulsar de soltar (el id del primer evento no lo dice); se ve a ojo por
# el instante: con pulsaciones de ~1 s, la rafaga de soltar llega ~1 s despues de la de pulsar
for r in rafagas:
    dur = (r[-1][1] - r[0][1]) / 1000
    print(f"{r[0][1] / 1e6:9.3f} s  primer id {NOMBRE.get(r[0][0], r[0][0]):8} {len(r):3} eventos  {dur:6.1f} ms")
print(f"\n{len(rafagas)} rafagas. Peor: {max((r[-1][1] - r[0][1]) for r in rafagas) / 1000:.1f} ms")
