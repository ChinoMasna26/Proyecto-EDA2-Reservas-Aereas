# Proyecto EDA II – Sistema de Reservas Aéreas con Lista de Espera

Sistema en C++ que registra, busca, actualiza y cancela reservas de pasajeros y gestiona
una lista de espera por vuelo. Usa tres estructuras implementadas por el grupo, sin STL:

| Estructura | Clave | Uso |
|---|---|---|
| Tabla hash con encadenamiento (`TablaHash`) | `idReserva` | buscar, actualizar, cancelar, validar ID |
| Árbol AVL (`ArbolAVL`) | fecha + hora + idReserva | listado cronológico y consultas por rango |
| Max-heap en arreglo (`MaxHeap`) | categoría, luego orden de llegada | lista de espera de cada vuelo |

## Estructura de carpetas

```
src/
  Reserva.h/.cpp     registro de reserva, validaciones, clave compuesta, CSV
  TablaHash.h/.cpp   tabla hash con listas enlazadas (dueña de las reservas)
  ArbolAVL.h/.cpp    árbol AVL: rotaciones, recorrido en orden, rango con poda
  MaxHeap.h/.cpp     max-heap: insertar, extraerMax, eliminarEn, listar sobre copia
  Vuelo.h            vuelo con su lista de espera
  Sistema.h/.cpp     coordina las 3 estructuras (consistencia) y las 10 funcionalidades
  Menu.h/.cpp        interfaz de consola
  main.cpp           programa principal (menú)
  Pruebas.cpp        8 escenarios de prueba, tiempos con <chrono> y métricas
generador/
  Generador.cpp      genera los archivos de prueba (semilla fija)
datos/
  vuelos.csv, reservas_pequeno.csv, reservas_masivo.csv
compilar.bat         compila los tres programas
evidencias.bat       mide la escalabilidad y graba una sesion del menu
entrada_menu.txt     teclas que usa evidencias.bat para la sesion del menu
```

## Compilar y ejecutar (Windows)

Se necesita g++ (MinGW-w64 / WinLibs) en el PATH.

1. Ejecutar `compilar.bat` (genera `reservas.exe`, `pruebas.exe` y `generar.exe`).
2. Los datos de prueba ya estan en `datos/` (`vuelos.csv`, `reservas_pequeno.csv` y `reservas_masivo.csv`).
   Para volver a generarlos: `generar.exe datos 10000` (la semilla es fija, salen identicos).
3. `reservas.exe` abre el menu interactivo (ejecutar desde la carpeta del proyecto).
4. `pruebas.exe` ejecuta los escenarios 1-8, las metricas y la tabla de tiempos.
5. `evidencias.bat` mide la escalabilidad (2 500 a 10 000 reservas) y graba una sesion del menu usando `entrada_menu.txt`.

## Restricciones respetadas

Solo se incluyen `<iostream>`, `<fstream>`, `<sstream>`, `<string>` y `<chrono>`.
No se usa `vector`, `list`, `map`, `set`, `unordered_map`, `priority_queue`, `sort`,
`<algorithm>` ni `<cstdlib>`. Toda la memoria se libera en los destructores.


## Formato de los archivos

`vuelos.csv`: `codigoVuelo,origen,destino,fecha,hora,capacidad`

`reservas_*.csv`: `idReserva,dni,pasajero,codigoVuelo,origen,destino,fecha,hora,clase,categoria,precio,estado`
