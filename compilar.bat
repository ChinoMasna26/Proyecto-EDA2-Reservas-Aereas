@echo off
REM Compilacion en Windows (necesita g++ de MinGW/WinLibs en el PATH)
where g++ >nul 2>nul
if errorlevel 1 goto sin_gpp
g++ -std=c++11 -O2 -o reservas.exe src\Reserva.cpp src\TablaHash.cpp src\ArbolAVL.cpp src\MaxHeap.cpp src\Sistema.cpp src\Menu.cpp src\main.cpp
if errorlevel 1 goto fallo
g++ -std=c++11 -O2 -o pruebas.exe src\Reserva.cpp src\TablaHash.cpp src\ArbolAVL.cpp src\MaxHeap.cpp src\Sistema.cpp src\Pruebas.cpp
if errorlevel 1 goto fallo
g++ -std=c++11 -O2 -o generar.exe generador\Generador.cpp
if errorlevel 1 goto fallo
echo Listo: reservas.exe (menu), pruebas.exe (escenarios y tiempos), generar.exe (datos)
goto fin
:sin_gpp
echo ERROR: no se encontro g++. Instale MinGW/WinLibs y abra una terminal nueva.
goto fin
:fallo
echo ERROR: la compilacion fallo, revise los mensajes de arriba.
:fin
pause
