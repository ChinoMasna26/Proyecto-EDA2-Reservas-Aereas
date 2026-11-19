#ifndef VUELO_H
#define VUELO_H

#include "MaxHeap.h"

// Vuelo con su propia lista de espera (max-heap).
// Los vuelos son pocos (40 a 60), por eso se guardan en un arreglo fijo.
struct Vuelo {
    string codigo;
    string origen;
    string destino;
    string fecha;
    string hora;
    int capacidad;
    int ocupados;       // reservas Confirmada + Pendiente + CheckIn
    MaxHeap espera;     // reservas EnEspera de este vuelo

    Vuelo() : capacidad(0), ocupados(0) {}
    bool lleno() const { return ocupados >= capacidad; }
    int libres() const { return capacidad - ocupados; }
};

#endif
