# Prueba de la pila de depuracion en el simulador (Paso 6).
# Uso, desde la carpeta del proyecto y tras compilar en Eclipse (configuracion Debug):
#   arm-none-eabi-gdb -batch -x pruebas/lista_sim.gdb Debug/P2_873042_960280.elf
# El simulador arranca en reversi8 (no pasa por Main); se para en entrada_jugada_lista y desde ahi
# se llama a mano a lista_inicializar y guardar_eventos.
set pagination off
set confirm off
target sim
load
break entrada_jugada_lista
run
delete
# Ensuciar la zona antes, para ver que lista_inicializar la limpia
set $a = 0x0C7FE000
while $a < 0x0C7FE4B8
  set *(unsigned int *)$a = 0xDEADBEEF
  set $a = $a + 4
end
call lista_inicializar()
echo \n=== Tras lista_inicializar: 2 primeras y 2 ultimas entradas, y la palabra siguiente (no se toca) ===\n
x/4wx 0x0C7FE000
x/4wx 0x0C7FE4A0
x/2wx 0x0C7FE4B0
# 150 eventos: id = n, instante = n * 1000
set $n = 1
while $n <= 150
  call guardar_eventos(10, $n * 1000)
  set $n = $n + 1
end
echo \n=== Tras 150 eventos: indice debe ser 0 (ha dado la vuelta) ===\n
print indice
x/4wx 0x0C7FE000
x/4wx 0x0C7FE4A0
call guardar_eventos(11, 151000)
echo \n=== Evento 151: pisa la entrada 0, indice = 1, el resto intacto ===\n
print indice
x/4wx 0x0C7FE000
x/2wx 0x0C7FE4B0
