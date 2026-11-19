#ifndef TABLAHASH_H
#define TABLAHASH_H

#include "Reserva.h"

// nodo de la lista de cada casilla
struct NodoHash {
    Reserva* dato;
    NodoHash* sig;
    NodoHash(Reserva* r, NodoHash* s) : dato(r), sig(s) {}
};

// Tabla hash con encadenamiento (clave = idReserva)
// ojo: la hash es la duena de las reservas, ella hace el delete
class TablaHash {
private:
    NodoHash** tabla;   // arreglo de cabezas de lista
    int tam;            // numero de posiciones (primo)
    int cantidad;       // reservas almacenadas

public:
    static const int TAM_DEFECTO = 13007;   // primo; factor de carga ~0.77 con 10 000

    TablaHash(int tamanio = TAM_DEFECTO);
    ~TablaHash();

    // Hash polinomial: h = (h * 31 + c) mod tam, sobre todos los caracteres.
    int funcionHash(const string& clave) const;

    bool insertar(Reserva* r);                    // false si el id ya existe
    Reserva* buscar(const string& id) const;      // nullptr si no existe
    bool eliminar(const string& id);              // libera nodo y reserva

    int getCantidad() const { return cantidad; }
    int getTam() const { return tam; }
    double factorCarga() const { return (double)cantidad / tam; }
    int longitudCadena(int pos) const;
    int cadenaMasLarga() const;
    int posicionesOcupadas() const;

    // Recorre todas las reservas llamando a f(reserva, contexto).
    void recorrer(void (*f)(Reserva*, void*), void* contexto) const;
};

#endif
