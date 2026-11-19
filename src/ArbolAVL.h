#ifndef ARBOLAVL_H
#define ARBOLAVL_H

#include "Reserva.h"

struct NodoAVL {
    Reserva* dato;
    NodoAVL* izq;
    NodoAVL* der;
    int altura;          // altura del subarbol (hoja = 1)
    NodoAVL(Reserva* r) : dato(r), izq(nullptr), der(nullptr), altura(1) {}
};

// AVL ordenado por fecha, hora e id.
// Solo libera sus nodos, las reservas las borra la hash.
class ArbolAVL {
private:
    NodoAVL* raiz;
    int cantidad;

    // utilidades de balanceo
    int alturaDe(NodoAVL* n) const { return n ? n->altura : 0; }
    int factorBalance(NodoAVL* n) const { return n ? alturaDe(n->izq) - alturaDe(n->der) : 0; }
    void actualizarAltura(NodoAVL* n);
    NodoAVL* rotarDerecha(NodoAVL* y);
    NodoAVL* rotarIzquierda(NodoAVL* x);
    NodoAVL* balancear(NodoAVL* n);

    // operaciones recursivas
    NodoAVL* insertarRec(NodoAVL* n, Reserva* r, bool& insertado);
    NodoAVL* eliminarRec(NodoAVL* n, const Reserva* clave, bool& eliminado);
    NodoAVL* minimo(NodoAVL* n) const;
    void inordenRec(NodoAVL* n, Reserva** salida, int maximo, int& k) const;
    void rangoRec(NodoAVL* n, const string& ini, const string& fin,
                  Reserva** muestra, int maxMuestra, int& total, long& visitados) const;
    void recorrerRec(NodoAVL* n, void (*f)(Reserva*, void*), void* contexto) const;
    bool esAVLRec(NodoAVL* n, int& altura) const;
    void liberar(NodoAVL* n);

public:
    ArbolAVL();
    ~ArbolAVL();

    bool insertar(Reserva* r);               // false si la clave ya existe
    bool eliminar(const Reserva* clave);     // busca por (fecha, hora, id)
    bool contiene(const Reserva* clave) const;

    int getCantidad() const { return cantidad; }
    int altura() const { return alturaDe(raiz); }

    // Primeras 'maximo' reservas en orden cronologico (recorrido en orden
    // que se detiene al completar). Devuelve cuantas se copiaron.
    int inorden(Reserva** salida, int maximo) const;

    // Reservas con fecha en [ini, fin] (inclusive). Poda las ramas fuera de
    // rango. Devuelve el total encontrado; copia hasta maxMuestra en 'muestra'
    // y cuenta en 'visitados' los nodos que realmente se revisaron.
    int consultaRango(const string& ini, const string& fin,
                      Reserva** muestra, int maxMuestra, long& visitados) const;

    // Recorrido en orden completo con callback.
    void recorrer(void (*f)(Reserva*, void*), void* contexto) const;

    // Verifica orden y balance (|fb| <= 1) de todo el arbol (para pruebas).
    bool esAVLValido() const;
};

#endif
