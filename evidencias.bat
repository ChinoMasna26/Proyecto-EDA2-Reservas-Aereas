@echo off
REM Genera las evidencias para el informe: escalabilidad y sesion del menu.
if not exist pruebas.exe goto sin_exe
if not exist reservas.exe goto sin_exe
if not exist generar.exe goto sin_exe
echo Generando datos de 2500, 5000 y 7500 reservas...
mkdir e2500 e5000 e7500 2>nul
generar.exe e2500 2500 >nul
generar.exe e5000 5000 >nul
generar.exe e7500 7500 >nul
if exist escala_completa.txt del escala_completa.txt
echo Midiendo escalabilidad (3 rondas, tarda menos de un minuto)...
for /L %%r in (1,1,3) do (
  pruebas.exe --escala e2500/reservas_masivo.csv >> escala_completa.txt
  pruebas.exe --escala e5000/reservas_masivo.csv >> escala_completa.txt
  pruebas.exe --escala e7500/reservas_masivo.csv >> escala_completa.txt
  pruebas.exe --escala datos/reservas_masivo.csv >> escala_completa.txt
)
echo Ejecutando la sesion del menu...
reservas.exe < entrada_menu.txt > sesion_menu.txt
rmdir /s /q e2500 e5000 e7500
echo.
echo Listo. Envie estos dos archivos: escala_completa.txt y sesion_menu.txt
goto fin
:sin_exe
echo Primero ejecute compilar.bat
:fin
pause
