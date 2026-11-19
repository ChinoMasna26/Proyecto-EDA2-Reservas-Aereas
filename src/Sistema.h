#ifndef SISTEMA_H
#define SISTEMA_H

#include "TablaHash.h"
#include "ArbolAVL.h"
#include "Vuelo.h"

// Resultado de la carga masiva (funcionalidad 1).
struct ResultadoCarga {
    int leidas;          // lineas de datos leidas
    int insertadas;      // reservas insertadas en hash y arbol
    int enHeaps;         // reservas EnEspera insertadas en algun heap
    int rechazadas;      // lineas invalidas, id repetido o vuelo inexistente
    int ajustadas;       // ocupaban asiento en un vuelo ya lleno -> pasaron a EnEspera
    ResultadoCarga() : leidas(0), insertadas(0), enHeaps(0), rechazadas(0), ajustadas(0) {}
};

// Codigos de resultado de las operaciones.
enum CodigoOp {
    OP_OK = 0,
    OP_ID_REPETIDO,
    OP_ID_NO_EXISTE,
    OP_VUELO_NO_EXISTE,
    OP_DATOS_INVALIDOS,
    OP_TRANSICION_INVALIDA,
    OP_YA_CANCELADA,
    OP_NO_CANCELADA,
    OP_INCONSISTENCIA
};
string mensajeCodigo(CodigoOp c);

// Estadisticas generales (funcionalidad 10).
struct Estadisticas {
    static const int MAX_DEST = 64;
    int total;
    double ingresos;           // Confirmada + CheckIn
    double sumaPrecios;
    double precioPromedio;     // sobre todas las reservas
    string destinos[MAX_DEST];
    int porDestino[MAX_DEST];
    int numDestinos;
    int porClase[3];           // Economica, Premium, Ejecutiva
    int porEstado[5];          // Confirmada, Pendiente, EnEspera, CheckIn, Cancelada
    Reserva* mayorPrecio;
    Reserva* menorPrecio;
    Vuelo* vueloMasEspera;
};

// Sistema junta las 3 estructuras. Todos los cambios pasan por aca para que
// la hash, el arbol y los heaps no se desincronicen.
class Sistema {
public:
    static const int MAX_VUELOS = 60;
    static const char* NOMBRES_CLASE[3];
    static const char* NOMBRES_ESTADO[5];

private:
    Vuelo vuelos[MAX_VUELOS];
    int numVuelos;
    TablaHash hash;
    ArbolAVL arbol;
    long contadorLlegada;      // orden global de llegada a listas de espera

    // Inserta una reserva ya validada en hash, arbol y (si aplica) heap.
    CodigoOp altaInterna(Reserva* r, Vuelo* v, bool& fueAEspera, bool& ajustada);

public:
    Sistema(int tamHash = TablaHash::TAM_DEFECTO);

    // 1. Carga masiva
    int cargarVuelos(const string& archivo);           // -1 si no se pudo abrir
    ResultadoCarga cargarReservas(const string& archivo, bool& abierto);

    Vuelo* buscarVuelo(const string& codigo);
    int getNumVuelos() const { return numVuelos; }
    Vuelo* getVuelo(int i) { return &vuelos[i]; }

    // 2. Registro: fecha, hora, origen y destino se toman del vuelo.
    //    estadoDeseado: "Confirmada" o "Pendiente" (si hay asiento).
    CodigoOp registrar(const string& id, const string& dni, const string& pasajero,
                       const string& codigoVuelo, const string& clase,
                       const string& categoria, double precio,
                       const string& estadoDeseado, Reserva*& creada);

    // 3. Busqueda por ID
    Reserva* buscar(const string& id) const { return hash.buscar(id); }

    // 4. Listado cronologico (primeras 'maximo')
    int listarCronologico(Reserva** salida, int maximo) const { return arbol.inorden(salida, maximo); }

    // 5. Consulta por rango de fechas
    int consultarRango(const string& ini, const string& fin, Reserva** muestra,
                       int maxMuestra, long& visitados) const {
        return arbol.consultaRango(ini, fin, muestra, maxMuestra, visitados);
    }

    // 6. Actualizacion de estado (Pendiente->Confirmada, Confirmada->CheckIn)
    CodigoOp actualizarEstado(const string& id, const string& nuevoEstado);

    // 7. Cancelacion. 'promovido' recibe la reserva que salio de la lista de espera.
    CodigoOp cancelar(const string& id, Reserva*& promovido);

    // 8. Eliminacion fisica de una reserva Cancelada
    CodigoOp eliminarFisica(const string& id);

    // 9. Lista de espera de un vuelo (sobre copia del heap)
    int listaEspera(const string& codigoVuelo, Reserva** salida, int maximo, Vuelo*& vuelo);

    // 10. Estadisticas
    void estadisticas(Estadisticas& e) const;

    // Verificacion completa de consistencia entre estructuras.
    bool verificarConsistencia(string& detalle) const;

    // Acceso de solo lectura para metricas y pruebas
    const TablaHash& getHash() const { return hash; }
    const ArbolAVL& getArbol() const { return arbol; }
    int totalEnEspera() const;
};

#endif
