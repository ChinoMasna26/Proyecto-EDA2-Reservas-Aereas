#include "MaxHeap.h"

MaxHeap::MaxHeap(int capacidadInicial) : tam(0), capacidad(capacidadInicial) {
    if (capacidad < 1) capacidad = 1;
    datos = new Reserva*[capacidad];
}

MaxHeap::MaxHeap(const MaxHeap& otro) : tam(otro.tam), capacidad(otro.capacidad) {
    datos = new Reserva*[capacidad];
    for (int i = 0; i < tam; i++) datos[i] = otro.datos[i];
}

MaxHeap::~MaxHeap() { delete[] datos; }

bool MaxHeap::mayorPrioridad(const Reserva* a, const Reserva* b) {
    int ca = rangoCategoria(a->categoria), cb = rangoCategoria(b->categoria);
    if (ca != cb) return ca > cb;
    return a->ordenLlegada < b->ordenLlegada;  // llego antes => mas prioridad
}

void MaxHeap::intercambiar(int i, int j) {
    Reserva* t = datos[i];
    datos[i] = datos[j];
    datos[j] = t;
}

void MaxHeap::crecer() {
    int nueva = capacidad * 2;
    Reserva** arr = new Reserva*[nueva];
    for (int i = 0; i < tam; i++) arr[i] = datos[i];
    delete[] datos;
    datos = arr;
    capacidad = nueva;
}

void MaxHeap::subir(int i) {
    while (i > 0 && mayorPrioridad(datos[i], datos[padre(i)])) {
        intercambiar(i, padre(i));
        i = padre(i);
    }
}

void MaxHeap::bajar(int i) {
    while (true) {
        int mayor = i, l = hijoIzq(i), r = hijoDer(i);
        if (l < tam && mayorPrioridad(datos[l], datos[mayor])) mayor = l;
        if (r < tam && mayorPrioridad(datos[r], datos[mayor])) mayor = r;
        if (mayor == i) return;
        intercambiar(i, mayor);
        i = mayor;
    }
}

void MaxHeap::insertar(Reserva* r) {
    if (tam == capacidad) crecer();
    datos[tam] = r;
    subir(tam);
    tam++;
}

Reserva* MaxHeap::verMax() const {
    return tam > 0 ? datos[0] : nullptr;
}

Reserva* MaxHeap::extraerMax() {
    if (tam == 0) return nullptr;
    Reserva* max = datos[0];
    datos[0] = datos[tam - 1];
    tam--;
    if (tam > 0) bajar(0);
    return max;
}

// ponemos el ultimo en la posicion que se borra y despues vemos si sube o baja
bool MaxHeap::eliminarEn(int pos) {
    if (pos < 0 || pos >= tam) return false;
    tam--;
    if (pos == tam) return true;  // era el ultimo
    datos[pos] = datos[tam];
    if (pos > 0 && mayorPrioridad(datos[pos], datos[padre(pos)])) subir(pos);
    else bajar(pos);
    return true;
}

int MaxHeap::buscarPos(const string& id) const {
    for (int i = 0; i < tam; i++)
        if (datos[i]->idReserva == id) return i;
    return -1;
}

int MaxHeap::listarEnOrden(Reserva** salida, int maximo) const {
    MaxHeap copia(*this);  // no se altera el heap original
    int k = 0;
    while (!copia.vacio() && k < maximo) salida[k++] = copia.extraerMax();
    return k;
}

bool MaxHeap::esValido() const {
    for (int i = 1; i < tam; i++)
        if (mayorPrioridad(datos[i], datos[padre(i)])) return false;
    return true;
}
