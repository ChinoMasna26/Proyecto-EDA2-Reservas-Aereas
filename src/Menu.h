#ifndef MENU_H
#define MENU_H

#include "Sistema.h"

// Interfaz de consola: cada opcion del menu llama a una funcionalidad del Sistema.
class Menu {
private:
    Sistema& sis;
    bool vuelosCargados;

    string leerLinea(const string& mensaje);
    double leerDouble(const string& mensaje);
    int leerEntero(const string& mensaje);
    string elegir(const string& mensaje, const char* const opciones[], int n);
    void cabeceraReservas();

    void opCargaMasiva();
    void opRegistrar();
    void opBuscar();
    void opListado();
    void opRango();
    void opActualizar();
    void opCancelar();
    void opEliminar();
    void opListaEspera();
    void opEstadisticas();
    void opConsistencia();
    void opVuelos();

public:
    Menu(Sistema& s) : sis(s), vuelosCargados(false) {}
    void ejecutar();
};

#endif
