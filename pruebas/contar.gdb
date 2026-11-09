# Cuenta fichas del tablero (reversi8_2026.c). Uso en la consola de GDB de Eclipse:
#   source C:/Users/sergi/Desktop/UNIZAR/3_INGENIERIA_INFORMATICA/1ER_CUATRIMESTRE/3_PROYECTO_HARDWARE/PRACTICAS/P2_873042_960280/pruebas/contar.gdb
# Con el programa parado (suspend). tablero es static, por eso el ambito 'reversi8_2026.c'::
set $t = (unsigned char *) &'reversi8_2026.c'::tablero[0][0]
set $b = 0
set $n = 0
set $i = 0
while $i < 64
  set $b = $b + ($t[$i] == 1)
  set $n = $n + ($t[$i] == 2)
  set $i = $i + 1
end
printf "blancas (maquina) = %d, negras (humano) = %d\n", $b, $n
