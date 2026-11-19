#ifndef MAXHEAP_H
#define MAXHEAP_H

#include "Reserva.h"

// Max heap para la lista de espera de cada vuelo.
// Prioridad: Premium > Frecuente > Regular, si empatan gana el que llego antes.
// padre = (i-1)/2, hijos = 2i+1 y 2i+2
class MaxHeap {
private:
    Reserva** datos;
    int tam;        // elementos usados
    int capacidad;  // tamano del arreglo

    static int padre(int i) { return (i - 1) / 2; }
    static int hijoIzq(int i) { return 2 * i + 1; }
    static int hijoDer(int i) { return 2 * i + 2; }
    void intercambiar(int i, int j);
    void subir(int i);
    void bajar(int i);
    void crecer();

public:
    MaxHeap(int capacidadInicial = 16);
    MaxHeap(const MaxHeap& otro);             // copia del arreglo (para listar)
    ~MaxHeap();

    // true si a tiene MAS prioridad que b
    static bool mayorPrioridad(const Reserva* a, const Reserva* b);

    void insertar(Reserva* r);
    Reserva* extraerMax();                    // nullptr si esta vacio
    Reserva* verMax() const;                  // raiz sin extraer
    bool eliminarEn(int pos);                 // elimina en posicion cualquiera
    int buscarPos(const string& id) const;    // busqueda secuencial; -1 si no esta

    int tamano() const { return tam; }
    bool vacio() const { return tam == 0; }
    Reserva* obtener(int i) const { return datos[i]; }

    // Copia los elementos en orden de prioridad sin alterar el heap
    // (trabaja sobre una copia). Devuelve cuantos copio.
    int listarEnOrden(Reserva** salida, int maximo) const;

    bool esValido() const;                    // verifica la propiedad de heap
};

#endif
