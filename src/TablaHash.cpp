#include "TablaHash.h"

TablaHash::TablaHash(int tamanio) : tam(tamanio), cantidad(0) {
    tabla = new NodoHash*[tam];
    for (int i = 0; i < tam; i++) tabla[i] = nullptr;
}

TablaHash::~TablaHash() {
    for (int i = 0; i < tam; i++) {
        NodoHash* actual = tabla[i];
        while (actual != nullptr) {
            NodoHash* siguiente = actual->sig;
            delete actual->dato;  // la hash es duena de la reserva
            delete actual;
            actual = siguiente;
        }
    }
    delete[] tabla;
}

int TablaHash::funcionHash(const string& clave) const {
    unsigned long h = 0;
    for (int i = 0; i < (int)clave.size(); i++)
        h = (h * 31 + (unsigned char)clave[i]) % tam;
    return (int)h;
}

bool TablaHash::insertar(Reserva* r) {
    int pos = funcionHash(r->idReserva);
    for (NodoHash* p = tabla[pos]; p != nullptr; p = p->sig)
        if (p->dato->idReserva == r->idReserva) return false;  // id repetido
    tabla[pos] = new NodoHash(r, tabla[pos]); // lo metemos al inicio
    cantidad++;
    return true;
}

Reserva* TablaHash::buscar(const string& id) const {
    int pos = funcionHash(id);
    for (NodoHash* p = tabla[pos]; p != nullptr; p = p->sig)
        if (p->dato->idReserva == id) return p->dato;
    return nullptr;
}

bool TablaHash::eliminar(const string& id) {
    int pos = funcionHash(id);
    NodoHash* anterior = nullptr;
    NodoHash* actual = tabla[pos];
    while (actual != nullptr && actual->dato->idReserva != id) {
        anterior = actual;
        actual = actual->sig;
    }
    if (actual == nullptr) return false;
    if (anterior == nullptr) tabla[pos] = actual->sig;
    else anterior->sig = actual->sig;
    delete actual->dato;
    delete actual;
    cantidad--;
    return true;
}

int TablaHash::longitudCadena(int pos) const {
    int n = 0;
    for (NodoHash* p = tabla[pos]; p != nullptr; p = p->sig) n++;
    return n;
}

int TablaHash::cadenaMasLarga() const {
    int maximo = 0;
    for (int i = 0; i < tam; i++) {
        int n = longitudCadena(i);
        if (n > maximo) maximo = n;
    }
    return maximo;
}

int TablaHash::posicionesOcupadas() const {
    int n = 0;
    for (int i = 0; i < tam; i++)
        if (tabla[i] != nullptr) n++;
    return n;
}

void TablaHash::recorrer(void (*f)(Reserva*, void*), void* contexto) const {
    for (int i = 0; i < tam; i++)
        for (NodoHash* p = tabla[i]; p != nullptr; p = p->sig)
            f(p->dato, contexto);
}
