#include "Sistema.h"
#include <fstream>
#include <sstream>

const char* Sistema::NOMBRES_CLASE[3]  = {"Economica", "Premium", "Ejecutiva"};
const char* Sistema::NOMBRES_ESTADO[5] = {"Confirmada", "Pendiente", "EnEspera", "CheckIn", "Cancelada"};

string mensajeCodigo(CodigoOp c) {
    switch (c) {
        case OP_OK:                  return "Operacion realizada correctamente.";
        case OP_ID_REPETIDO:         return "Error: ya existe una reserva con ese ID.";
        case OP_ID_NO_EXISTE:        return "Error: no existe una reserva con ese ID.";
        case OP_VUELO_NO_EXISTE:     return "Error: el vuelo no existe.";
        case OP_DATOS_INVALIDOS:     return "Error: datos invalidos.";
        case OP_TRANSICION_INVALIDA: return "Error: transicion de estado no permitida "
                                            "(solo Pendiente->Confirmada y Confirmada->CheckIn).";
        case OP_YA_CANCELADA:        return "Error: la reserva ya esta Cancelada y no se puede modificar.";
        case OP_NO_CANCELADA:        return "Error: solo se pueden eliminar fisicamente reservas Canceladas.";
        case OP_INCONSISTENCIA:      return "Error interno: inconsistencia entre estructuras.";
    }
    return "";
}

Sistema::Sistema(int tamHash) : numVuelos(0), hash(tamHash), contadorLlegada(0) {}

static string recortar(const string& s) {
    int ini = 0, fin = (int)s.size() - 1;
    while (ini <= fin && (s[ini] == ' ' || s[ini] == '\t' || s[ini] == '\r')) ini++;
    while (fin >= ini && (s[fin] == ' ' || s[fin] == '\t' || s[fin] == '\r')) fin--;
    return s.substr(ini, fin - ini + 1);
}

// 1. Carga masiva
int Sistema::cargarVuelos(const string& archivo) {
    ifstream in(archivo.c_str());
    if (!in.is_open()) return -1;
    string linea;
    int cargados = 0;
    while (getline(in, linea)) {
        linea = recortar(linea);
        if (linea.empty() || linea.compare(0, 11, "codigoVuelo") == 0) continue;  // cabecera
        if (numVuelos >= MAX_VUELOS) break;
        string c[6];
        int n = 0;
        stringstream ss(linea);
        string campo;
        while (n < 6 && getline(ss, campo, ',')) c[n++] = recortar(campo);
        if (n != 6 || !fechaValida(c[3]) || !horaValida(c[4])) continue;
        stringstream sc(c[5]);
        int cap;
        if (!(sc >> cap) || cap <= 0) continue;
        if (buscarVuelo(c[0]) != nullptr) continue;  // codigo repetido
        Vuelo& v = vuelos[numVuelos++];
        v.codigo = c[0]; v.origen = c[1]; v.destino = c[2];
        v.fecha = c[3];  v.hora = c[4];   v.capacidad = cap; v.ocupados = 0;
        cargados++;
    }
    return cargados;
}

Vuelo* Sistema::buscarVuelo(const string& codigo) {
    for (int i = 0; i < numVuelos; i++)
        if (vuelos[i].codigo == codigo) return &vuelos[i];
    return nullptr;
}

CodigoOp Sistema::altaInterna(Reserva* r, Vuelo* v, bool& fueAEspera, bool& ajustada) {
    fueAEspera = false;
    ajustada = false;
    if (hash.buscar(r->idReserva) != nullptr) return OP_ID_REPETIDO;

    // Si ocuparia asiento pero el vuelo esta lleno, va a lista de espera.
    if (ocupaAsiento(r) && v->lleno()) {
        r->estado = "EnEspera";
        ajustada = true;
    }

    hash.insertar(r);
    if (!arbol.insertar(r)) {  // no deberia pasar: la clave incluye el id
        hash.eliminar(r->idReserva);  // revierte (y libera r)
        return OP_INCONSISTENCIA;
    }
    if (ocupaAsiento(r)) {
        v->ocupados++;
    } else if (r->estado == "EnEspera") {
        r->ordenLlegada = ++contadorLlegada;
        v->espera.insertar(r);
        fueAEspera = true;
    }
    return OP_OK;
}

ResultadoCarga Sistema::cargarReservas(const string& archivo, bool& abierto) {
    ResultadoCarga res;
    ifstream in(archivo.c_str());
    abierto = in.is_open();
    if (!abierto) return res;
    string linea;
    while (getline(in, linea)) {
        linea = recortar(linea);
        if (linea.empty() || linea.compare(0, 9, "idReserva") == 0) continue;  // cabecera
        res.leidas++;
        Reserva* r = new Reserva();
        Vuelo* v = nullptr;
        bool valida = parsearReserva(linea, *r);
        if (valida) {
            v = buscarVuelo(r->codigoVuelo);
            valida = v != nullptr && v->fecha == r->fecha && v->hora == r->hora &&
                     v->origen == r->origen && v->destino == r->destino;
        }
        if (!valida) {
            delete r;
            res.rechazadas++;
            continue;
        }
        bool aEspera, ajustada;
        CodigoOp c = altaInterna(r, v, aEspera, ajustada);
        if (c == OP_OK) {
            res.insertadas++;
            if (aEspera) res.enHeaps++;
            if (ajustada) res.ajustadas++;
        } else {
            if (c == OP_ID_REPETIDO) delete r;  // en OP_INCONSISTENCIA ya se libero
            res.rechazadas++;
        }
    }
    return res;
}

// 2. Registro de nueva reserva
static bool sinComas(const string& s) {
    for (int i = 0; i < (int)s.size(); i++)
        if (s[i] == ',') return false;
    return true;
}

CodigoOp Sistema::registrar(const string& id, const string& dni, const string& pasajero,
                            const string& codigoVuelo, const string& clase,
                            const string& categoria, double precio,
                            const string& estadoDeseado, Reserva*& creada) {
    creada = nullptr;
    if (id.empty() || dni.empty() || pasajero.empty() || precio < 0 ||
        !sinComas(id) || !sinComas(dni) || !sinComas(pasajero) ||
        !claseValida(clase) || !categoriaValida(categoria) ||
        (estadoDeseado != "Confirmada" && estadoDeseado != "Pendiente"))
        return OP_DATOS_INVALIDOS;
    if (hash.buscar(id) != nullptr) return OP_ID_REPETIDO;
    Vuelo* v = buscarVuelo(codigoVuelo);
    if (v == nullptr) return OP_VUELO_NO_EXISTE;

    Reserva* r = new Reserva();
    r->idReserva = id;   r->dni = dni;           r->pasajero = pasajero;
    r->codigoVuelo = v->codigo; r->origen = v->origen; r->destino = v->destino;
    r->fecha = v->fecha; r->hora = v->hora;
    r->clase = clase;    r->categoria = categoria; r->precio = precio;
    r->estado = v->lleno() ? "EnEspera" : estadoDeseado;

    bool aEspera, ajustada;
    CodigoOp c = altaInterna(r, v, aEspera, ajustada);
    if (c != OP_OK) {
        if (c == OP_ID_REPETIDO) delete r;
        return c;
    }
    creada = r;
    return OP_OK;
}

// 6. Actualizacion de estado
CodigoOp Sistema::actualizarEstado(const string& id, const string& nuevoEstado) {
    Reserva* r = hash.buscar(id);
    if (r == nullptr) return OP_ID_NO_EXISTE;
    if (r->estado == "Cancelada") return OP_YA_CANCELADA;
    bool permitida = (r->estado == "Pendiente"  && nuevoEstado == "Confirmada") ||
                     (r->estado == "Confirmada" && nuevoEstado == "CheckIn");
    if (!permitida) return OP_TRANSICION_INVALIDA;
    // Ambas transiciones mantienen la ocupacion del asiento y no cambian la
    // clave del arbol; como el arbol apunta al mismo objeto, queda actualizado.
    r->estado = nuevoEstado;
    return OP_OK;
}

// 7. Cancelacion
CodigoOp Sistema::cancelar(const string& id, Reserva*& promovido) {
    promovido = nullptr;
    Reserva* r = hash.buscar(id);
    if (r == nullptr) return OP_ID_NO_EXISTE;
    if (r->estado == "Cancelada") return OP_YA_CANCELADA;
    Vuelo* v = buscarVuelo(r->codigoVuelo);
    if (v == nullptr) return OP_INCONSISTENCIA;

    if (ocupaAsiento(r)) {
        // Caso A: libera el asiento y promueve al de mayor prioridad.
        r->estado = "Cancelada";
        v->ocupados--;
        if (!v->espera.vacio()) {
            Reserva* p = v->espera.extraerMax();
            p->estado = "Confirmada";
            v->ocupados++;
            promovido = p;
        }
    } else {
        // Caso B: estaba EnEspera; se retira del heap desde su posicion.
        int pos = v->espera.buscarPos(id);
        if (pos < 0) return OP_INCONSISTENCIA;
        v->espera.eliminarEn(pos);
        r->estado = "Cancelada";
    }
    return OP_OK;
}

// 8. Eliminacion fisica
CodigoOp Sistema::eliminarFisica(const string& id) {
    Reserva* r = hash.buscar(id);
    if (r == nullptr) return OP_ID_NO_EXISTE;
    if (r->estado != "Cancelada") return OP_NO_CANCELADA;
    // Primero el arbol (necesita la clave de r), luego la hash, que libera r.
    if (!arbol.eliminar(r)) return OP_INCONSISTENCIA;
    hash.eliminar(id);
    return OP_OK;
}

// 9. Lista de espera
int Sistema::listaEspera(const string& codigoVuelo, Reserva** salida, int maximo, Vuelo*& vuelo) {
    vuelo = buscarVuelo(codigoVuelo);
    if (vuelo == nullptr) return -1;
    return vuelo->espera.listarEnOrden(salida, maximo);
}

int Sistema::totalEnEspera() const {
    int t = 0;
    for (int i = 0; i < numVuelos; i++) t += vuelos[i].espera.tamano();
    return t;
}

// 10. Estadisticas (recorrido en orden del arbol)
static int indiceDe(const char* const nombres[], int n, const string& valor) {
    for (int i = 0; i < n; i++)
        if (valor == nombres[i]) return i;
    return -1;
}

static void acumular(Reserva* r, void* ctx) {
    Estadisticas& e = *(Estadisticas*)ctx;
    e.total++;
    e.sumaPrecios += r->precio;
    if (r->estado == "Confirmada" || r->estado == "CheckIn") e.ingresos += r->precio;

    int d = -1;
    for (int i = 0; i < e.numDestinos; i++)
        if (e.destinos[i] == r->destino) { d = i; break; }
    if (d < 0 && e.numDestinos < Estadisticas::MAX_DEST) {
        d = e.numDestinos++;
        e.destinos[d] = r->destino;
        e.porDestino[d] = 0;
    }
    if (d >= 0) e.porDestino[d]++;

    int c = indiceDe(Sistema::NOMBRES_CLASE, 3, r->clase);
    if (c >= 0) e.porClase[c]++;
    int s = indiceDe(Sistema::NOMBRES_ESTADO, 5, r->estado);
    if (s >= 0) e.porEstado[s]++;

    if (e.mayorPrecio == nullptr || r->precio > e.mayorPrecio->precio) e.mayorPrecio = r;
    if (e.menorPrecio == nullptr || r->precio < e.menorPrecio->precio) e.menorPrecio = r;
}

void Sistema::estadisticas(Estadisticas& e) const {
    e.total = 0; e.ingresos = 0; e.sumaPrecios = 0; e.precioPromedio = 0;
    e.numDestinos = 0;
    for (int i = 0; i < 3; i++) e.porClase[i] = 0;
    for (int i = 0; i < 5; i++) e.porEstado[i] = 0;
    e.mayorPrecio = e.menorPrecio = nullptr;
    e.vueloMasEspera = nullptr;

    arbol.recorrer(acumular, &e);
    if (e.total > 0) e.precioPromedio = e.sumaPrecios / e.total;

    int maxEspera = 0;
    for (int i = 0; i < numVuelos; i++)
        if (vuelos[i].espera.tamano() > maxEspera) {
            maxEspera = vuelos[i].espera.tamano();
            e.vueloMasEspera = const_cast<Vuelo*>(&vuelos[i]);
        }
}

// Verificacion de consistencia
struct CtxConsistencia {
    const Sistema* sis;
    const ArbolAVL* arbol;
    int ocupados[Sistema::MAX_VUELOS];
    int enEspera;
    int errores;
    string detalle;
};

static void revisar(Reserva* r, void* p) {
    CtxConsistencia& c = *(CtxConsistencia*)p;
    if (!c.arbol->contiene(r)) {
        c.errores++;
        c.detalle += "  " + r->idReserva + " esta en la hash pero no en el arbol\n";
    }
    Sistema* s = const_cast<Sistema*>(c.sis);
    Vuelo* v = s->buscarVuelo(r->codigoVuelo);
    if (v == nullptr) {
        c.errores++;
        c.detalle += "  " + r->idReserva + " referencia un vuelo inexistente\n";
        return;
    }
    int idx = (int)(v - s->getVuelo(0));
    if (ocupaAsiento(r)) c.ocupados[idx]++;
    if (r->estado == "EnEspera") {
        c.enEspera++;
        if (v->espera.buscarPos(r->idReserva) < 0) {
            c.errores++;
            c.detalle += "  " + r->idReserva + " esta EnEspera pero no en el heap\n";
        }
    }
}

bool Sistema::verificarConsistencia(string& detalle) const {
    CtxConsistencia c;
    c.sis = this; c.arbol = &arbol; c.enEspera = 0; c.errores = 0;
    for (int i = 0; i < MAX_VUELOS; i++) c.ocupados[i] = 0;

    if (hash.getCantidad() != arbol.getCantidad()) {
        c.errores++;
        ostringstream os;
        os << "  hash=" << hash.getCantidad() << " arbol=" << arbol.getCantidad() << "\n";
        c.detalle += os.str();
    }
    if (!arbol.esAVLValido()) { c.errores++; c.detalle += "  el arbol no cumple orden/balance AVL\n"; }

    hash.recorrer(revisar, &c);

    if (c.enEspera != totalEnEspera()) {
        c.errores++;
        c.detalle += "  el total en heaps no coincide con las reservas EnEspera\n";
    }
    for (int i = 0; i < numVuelos; i++) {
        const Vuelo& v = vuelos[i];
        if (!v.espera.esValido()) { c.errores++; c.detalle += "  heap invalido en " + v.codigo + "\n"; }
        for (int k = 0; k < v.espera.tamano(); k++)
            if (v.espera.obtener(k)->estado != "EnEspera") {
                c.errores++;
                c.detalle += "  heap de " + v.codigo + " contiene una reserva que no esta EnEspera\n";
            }
        if (c.ocupados[i] != v.ocupados || v.ocupados > v.capacidad) {
            c.errores++;
            c.detalle += "  ocupacion incorrecta en " + v.codigo + "\n";
        }
    }
    detalle = c.detalle;
    return c.errores == 0;
}
