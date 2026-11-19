#ifndef RESERVA_H
#define RESERVA_H

#include <string>
using namespace std;

// Datos de una reserva.
// Cada reserva se crea una sola vez con new. La hash es la que la borra,
// el arbol y los heaps solo guardan punteros a la misma reserva.
struct Reserva {
    string idReserva;
    string dni;
    string pasajero;
    string codigoVuelo;
    string origen;
    string destino;
    string fecha;      // AAAA-MM-DD
    string hora;       // HH:MM
    string clase;      // Economica | Premium | Ejecutiva
    string categoria;  // Premium | Frecuente | Regular
    double precio;
    string estado;     // Confirmada | Pendiente | EnEspera | CheckIn | Cancelada
    long ordenLlegada; // orden de ingreso a la lista de espera (desempate del heap)

    Reserva() : precio(0.0), ordenLlegada(0) {}
};

// Una reserva ocupa asiento si esta Confirmada, Pendiente o CheckIn.
bool ocupaAsiento(const Reserva* r);

// Premium = 3, Frecuente = 2, Regular = 1 (0 si no es valida).
int rangoCategoria(const string& categoria);

// Validaciones de dominio
bool claseValida(const string& c);
bool categoriaValida(const string& c);
bool estadoValido(const string& e);
bool fechaValida(const string& f);   // formato AAAA-MM-DD
bool horaValida(const string& h);    // formato HH:MM

// Compara por la clave compuesta del arbol: fecha, hora, idReserva.
// Devuelve <0, 0 o >0.
int compararClave(const Reserva* a, const Reserva* b);

// Convierte una linea CSV en una reserva. Devuelve false si es invalida.
bool parsearReserva(const string& linea, Reserva& r);

// Linea CSV (mismo formato del archivo) y texto legible para pantalla.
string reservaACsv(const Reserva* r);
string reservaATexto(const Reserva* r);

// Formatea un numero con 2 decimales.
string formatoPrecio(double valor);

#endif
