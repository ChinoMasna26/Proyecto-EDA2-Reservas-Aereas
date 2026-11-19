#include "Reserva.h"
#include <sstream>
#include <iostream>

bool ocupaAsiento(const Reserva* r) {
    return r->estado == "Confirmada" || r->estado == "Pendiente" || r->estado == "CheckIn";
}

int rangoCategoria(const string& categoria) {
    if (categoria == "Premium")   return 3;
    if (categoria == "Frecuente") return 2;
    if (categoria == "Regular")   return 1;
    return 0;
}

bool claseValida(const string& c) {
    return c == "Economica" || c == "Premium" || c == "Ejecutiva";
}

bool categoriaValida(const string& c) {
    return rangoCategoria(c) > 0;
}

bool estadoValido(const string& e) {
    return e == "Confirmada" || e == "Pendiente" || e == "EnEspera" ||
           e == "CheckIn" || e == "Cancelada";
}

static bool esDigito(char c) { return c >= '0' && c <= '9'; }

bool fechaValida(const string& f) {
    if (f.size() != 10 || f[4] != '-' || f[7] != '-') return false;
    for (int i = 0; i < 10; i++)
        if (i != 4 && i != 7 && !esDigito(f[i])) return false;
    int mes = (f[5] - '0') * 10 + (f[6] - '0');
    int dia = (f[8] - '0') * 10 + (f[9] - '0');
    return mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31;
}

bool horaValida(const string& h) {
    if (h.size() != 5 || h[2] != ':') return false;
    if (!esDigito(h[0]) || !esDigito(h[1]) || !esDigito(h[3]) || !esDigito(h[4])) return false;
    int hh = (h[0] - '0') * 10 + (h[1] - '0');
    int mm = (h[3] - '0') * 10 + (h[4] - '0');
    return hh <= 23 && mm <= 59;
}

int compararClave(const Reserva* a, const Reserva* b) {
    int c = a->fecha.compare(b->fecha);
    if (c != 0) return c;
    c = a->hora.compare(b->hora);
    if (c != 0) return c;
    return a->idReserva.compare(b->idReserva);
}

// saca espacios y el \r que dejan los archivos de windows
static string limpiar(const string& s) {
    int ini = 0, fin = (int)s.size() - 1;
    while (ini <= fin && (s[ini] == ' ' || s[ini] == '\t' || s[ini] == '\r')) ini++;
    while (fin >= ini && (s[fin] == ' ' || s[fin] == '\t' || s[fin] == '\r')) fin--;
    return s.substr(ini, fin - ini + 1);
}

bool parsearReserva(const string& linea, Reserva& r) {
    string campos[12];
    int n = 0;
    stringstream ss(linea);
    string campo;
    while (getline(ss, campo, ',')) {
        if (n >= 12) return false;  // sobran columnas
        campos[n++] = limpiar(campo);
    }
    if (n != 12) return false;

    r.idReserva   = campos[0];
    r.dni         = campos[1];
    r.pasajero    = campos[2];
    r.codigoVuelo = campos[3];
    r.origen      = campos[4];
    r.destino     = campos[5];
    r.fecha       = campos[6];
    r.hora        = campos[7];
    r.clase       = campos[8];
    r.categoria   = campos[9];
    r.estado      = campos[11];
    r.ordenLlegada = 0;

    if (r.idReserva.empty() || r.codigoVuelo.empty()) return false;
    if (!fechaValida(r.fecha) || !horaValida(r.hora)) return false;
    if (!claseValida(r.clase) || !categoriaValida(r.categoria) || !estadoValido(r.estado))
        return false;

    stringstream sp(campos[10]);
    double p;
    if (!(sp >> p) || p < 0) return false;
    r.precio = p;
    return true;
}

string formatoPrecio(double valor) {
    ostringstream os;
    os.setf(ios::fixed);
    os.precision(2);
    os << valor;
    return os.str();
}

string reservaACsv(const Reserva* r) {
    return r->idReserva + "," + r->dni + "," + r->pasajero + "," + r->codigoVuelo + "," +
           r->origen + "," + r->destino + "," + r->fecha + "," + r->hora + "," +
           r->clase + "," + r->categoria + "," + formatoPrecio(r->precio) + "," + r->estado;
}

// Rellena con espacios a la derecha hasta 'ancho' caracteres.
static string ajustar(const string& s, int ancho) {
    string t = s;
    while ((int)t.size() < ancho) t += ' ';
    return t;
}

string reservaATexto(const Reserva* r) {
    return ajustar(r->idReserva, 11) + " " + r->fecha + " " + r->hora + "  " +
           ajustar(r->codigoVuelo, 7) + " " + ajustar(r->origen + "-" + r->destino, 20) + " " +
           ajustar(r->pasajero, 24) + " " + ajustar(r->clase, 10) + " " +
           ajustar(r->categoria, 10) + " " + ajustar(formatoPrecio(r->precio), 8) + " " + r->estado;
}
