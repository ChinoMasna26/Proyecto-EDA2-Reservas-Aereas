#include "ArbolAVL.h"

ArbolAVL::ArbolAVL() : raiz(nullptr), cantidad(0) {}

ArbolAVL::~ArbolAVL() { liberar(raiz); }

void ArbolAVL::liberar(NodoAVL* n) {
    if (n == nullptr) return;
    liberar(n->izq);
    liberar(n->der);
    delete n;  // solo el nodo; la reserva la libera la hash
}

void ArbolAVL::actualizarAltura(NodoAVL* n) {
    int hi = alturaDe(n->izq), hd = alturaDe(n->der);
    n->altura = 1 + (hi > hd ? hi : hd);
}

// Rotacion simple a la derecha: el hijo izquierdo x sube y y pasa a ser
// su hijo derecho; el subarbol derecho de x (B) pasa a ser hijo izquierdo de y.
NodoAVL* ArbolAVL::rotarDerecha(NodoAVL* y) {
    NodoAVL* x = y->izq;
    NodoAVL* B = x->der;
    x->der = y;
    y->izq = B;
    actualizarAltura(y);
    actualizarAltura(x);
    return x;
}

// Rotacion simple a la izquierda: el hijo derecho y sube y x pasa a ser
// su hijo izquierdo; el subarbol izquierdo de y (B) pasa a ser hijo derecho de x.
NodoAVL* ArbolAVL::rotarIzquierda(NodoAVL* x) {
    NodoAVL* y = x->der;
    NodoAVL* B = y->izq;
    y->izq = x;
    x->der = B;
    actualizarAltura(x);
    actualizarAltura(y);
    return y;
}

// Restaura la propiedad AVL en n (casos LL, LR, RR, RL).
NodoAVL* ArbolAVL::balancear(NodoAVL* n) {
    actualizarAltura(n);
    int fb = factorBalance(n);
    if (fb > 1) {  // cargado a la izquierda
        if (factorBalance(n->izq) < 0)  // caso LR
            n->izq = rotarIzquierda(n->izq);
        return rotarDerecha(n);  // caso LL
    }
    if (fb < -1) {  // cargado a la derecha
        if (factorBalance(n->der) > 0)  // caso RL
            n->der = rotarDerecha(n->der);
        return rotarIzquierda(n);  // caso RR
    }
    return n;
}

NodoAVL* ArbolAVL::insertarRec(NodoAVL* n, Reserva* r, bool& insertado) {
    if (n == nullptr) {
        insertado = true;
        return new NodoAVL(r);
    }
    int c = compararClave(r, n->dato);
    if (c < 0)      n->izq = insertarRec(n->izq, r, insertado);
    else if (c > 0) n->der = insertarRec(n->der, r, insertado);
    else            return n;  // clave repetida
    return balancear(n);
}

bool ArbolAVL::insertar(Reserva* r) {
    bool insertado = false;
    raiz = insertarRec(raiz, r, insertado);
    if (insertado) cantidad++;
    return insertado;
}

NodoAVL* ArbolAVL::minimo(NodoAVL* n) const {
    while (n->izq != nullptr) n = n->izq;
    return n;
}

NodoAVL* ArbolAVL::eliminarRec(NodoAVL* n, const Reserva* clave, bool& eliminado) {
    if (n == nullptr) return nullptr;
    int c = compararClave(clave, n->dato);
    if (c < 0)      n->izq = eliminarRec(n->izq, clave, eliminado);
    else if (c > 0) n->der = eliminarRec(n->der, clave, eliminado);
    else {
        eliminado = true;
        if (n->izq == nullptr || n->der == nullptr) {  // 0 o 1 hijo
            NodoAVL* hijo = n->izq ? n->izq : n->der;
            delete n;
            return hijo;
        }
        // 2 hijos: se reemplaza por el sucesor en orden y se borra el sucesor
        NodoAVL* suc = minimo(n->der);
        n->dato = suc->dato;
        bool aux = false;
        n->der = eliminarRec(n->der, suc->dato, aux);
    }
    return balancear(n);
}

bool ArbolAVL::eliminar(const Reserva* clave) {
    bool eliminado = false;
    raiz = eliminarRec(raiz, clave, eliminado);
    if (eliminado) cantidad--;
    return eliminado;
}

bool ArbolAVL::contiene(const Reserva* clave) const {
    NodoAVL* n = raiz;
    while (n != nullptr) {
        int c = compararClave(clave, n->dato);
        if (c == 0) return true;
        n = (c < 0) ? n->izq : n->der;
    }
    return false;
}

void ArbolAVL::inordenRec(NodoAVL* n, Reserva** salida, int maximo, int& k) const {
    if (n == nullptr || k >= maximo) return;
    inordenRec(n->izq, salida, maximo, k);
    if (k < maximo) salida[k++] = n->dato;
    inordenRec(n->der, salida, maximo, k);
}

int ArbolAVL::inorden(Reserva** salida, int maximo) const {
    int k = 0;
    inordenRec(raiz, salida, maximo, k);
    return k;
}

// Como la clave empieza con la fecha, a la izquierda todo tiene fecha <= y a la
// derecha >=. Si la fecha del nodo es menor que ini no tiene sentido bajar a la
// izquierda, y si es mayor que fin no bajamos a la derecha.
// OJO: tiene que ser >= y <= porque hay fechas repetidas (con > y < se perdian reservas)
void ArbolAVL::rangoRec(NodoAVL* n, const string& ini, const string& fin,
                        Reserva** muestra, int maxMuestra, int& total, long& visitados) const {
    if (n == nullptr) return;
    visitados++;
    const string& f = n->dato->fecha;
    if (f >= ini) rangoRec(n->izq, ini, fin, muestra, maxMuestra, total, visitados);
    if (f >= ini && f <= fin) {
        if (total < maxMuestra) muestra[total] = n->dato;
        total++;
    }
    if (f <= fin) rangoRec(n->der, ini, fin, muestra, maxMuestra, total, visitados);
}

int ArbolAVL::consultaRango(const string& ini, const string& fin,
                            Reserva** muestra, int maxMuestra, long& visitados) const {
    int total = 0;
    visitados = 0;
    rangoRec(raiz, ini, fin, muestra, maxMuestra, total, visitados);
    return total;
}

void ArbolAVL::recorrerRec(NodoAVL* n, void (*f)(Reserva*, void*), void* contexto) const {
    if (n == nullptr) return;
    recorrerRec(n->izq, f, contexto);
    f(n->dato, contexto);
    recorrerRec(n->der, f, contexto);
}

void ArbolAVL::recorrer(void (*f)(Reserva*, void*), void* contexto) const {
    recorrerRec(raiz, f, contexto);
}

bool ArbolAVL::esAVLRec(NodoAVL* n, int& altura) const {
    if (n == nullptr) { altura = 0; return true; }
    int hi, hd;
    if (!esAVLRec(n->izq, hi) || !esAVLRec(n->der, hd)) return false;
    if (n->izq && compararClave(n->izq->dato, n->dato) >= 0) return false;
    if (n->der && compararClave(n->der->dato, n->dato) <= 0) return false;
    int fb = hi - hd;
    if (fb > 1 || fb < -1) return false;
    altura = 1 + (hi > hd ? hi : hd);
    return altura == n->altura;
}

bool ArbolAVL::esAVLValido() const {
    int h;
    return esAVLRec(raiz, h);
}
