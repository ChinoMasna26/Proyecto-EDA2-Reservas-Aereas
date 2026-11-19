// Generador de los archivos de prueba (vuelos, reservas pequeno y masivo)
// uso: generar [carpeta] [cantidad]
// No usamos rand() porque cstdlib no esta permitido, asi que hicimos un LCG simple.
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

// generador aleatorio propio (LCG de 64 bits)
class Aleatorio {
    unsigned long long estado;
public:
    Aleatorio(unsigned long long semilla) : estado(semilla) {}
    unsigned long long siguiente() {
        estado = estado * 6364136223846793005ULL + 1442695040888963407ULL;
        return estado >> 33;
    }
    int entre(int a, int b) { return a + (int)(siguiente() % (unsigned long long)(b - a + 1)); }
};

// datos base
const int NUM_DEST = 14;
const char* DESTINOS[NUM_DEST] = {"Cusco", "Arequipa", "Trujillo", "Piura", "Iquitos", "Tacna",
                                  "Chiclayo", "Juliaca", "Tarapoto", "Ayacucho", "Cajamarca",
                                  "Pucallpa", "Puerto Maldonado", "Tumbes"};
const char* AEROLINEAS[4] = {"LA", "AV", "JA", "H2"};
const int NUM_NOM = 40, NUM_APE = 40;
const char* NOMBRES[NUM_NOM] = {"Ana", "Luis", "Maria", "Jose", "Carmen", "Jorge", "Rosa", "Carlos",
    "Lucia", "Miguel", "Sofia", "Pedro", "Valeria", "Diego", "Camila", "Andres", "Daniela", "Javier",
    "Fernanda", "Ricardo", "Gabriela", "Manuel", "Paola", "Raul", "Elena", "Victor", "Patricia",
    "Hugo", "Claudia", "Oscar", "Andrea", "Martin", "Natalia", "Sergio", "Mariana", "Pablo",
    "Isabel", "Rodrigo", "Veronica", "Alonso"};
const char* APELLIDOS[NUM_APE] = {"Torres", "Ramos", "Soto", "Quispe", "Flores", "Garcia", "Rojas",
    "Mendoza", "Huaman", "Chavez", "Vargas", "Castillo", "Espinoza", "Diaz", "Lopez", "Ramirez",
    "Sanchez", "Perez", "Gutierrez", "Mamani", "Cruz", "Romero", "Salazar", "Vasquez", "Reyes",
    "Herrera", "Medina", "Aguilar", "Paredes", "Cordova", "Rivera", "Silva", "Morales", "Castro",
    "Ortiz", "Navarro", "Cabrera", "Delgado", "Carrasco", "Guerrero"};

// estructuras simples
struct VueloGen {
    string codigo, origen, destino, fecha, hora;
    int capacidad;
    bool lleno;
};

struct ReservaGen {
    int vuelo;  // indice del vuelo
    string clase, categoria, estado;
    double precio;
    string pasajero, dni;
};

const int MAX_VUELOS = 60;
const int MAX_RES = 30000;

// utilidades
string dosDigitos(int n) { return (n < 10 ? "0" : "") + to_string(n); }

string conCeros(int n, int ancho) {
    string s = to_string(n);
    while ((int)s.size() < ancho) s = "0" + s;
    return s;
}

// Fecha a partir de 2026-12-01 sumando 'dias' (dic 31, ene 31, feb 28, mar 31).
string fechaDesde(int dias) {
    int anio = 2026, mes = 12, dia = 1 + dias;
    const int largo[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    while (dia > largo[mes]) {
        dia -= largo[mes];
        mes++;
        if (mes > 12) { mes = 1; anio++; }
    }
    return to_string(anio) + "-" + dosDigitos(mes) + "-" + dosDigitos(dia);
}

string precioTexto(double p) {
    ostringstream os;
    os.setf(ios::fixed);
    os.precision(2);
    os << p;
    return os.str();
}

string elegirCategoria(Aleatorio& rnd) {  // 15% Premium, 30% Frecuente, 55% Regular
    int x = rnd.entre(1, 100);
    if (x <= 15) return "Premium";
    if (x <= 45) return "Frecuente";
    return "Regular";
}

string elegirClase(Aleatorio& rnd) {  // 70% Economica, 20% Premium, 10% Ejecutiva
    int x = rnd.entre(1, 100);
    if (x <= 70) return "Economica";
    if (x <= 90) return "Premium";
    return "Ejecutiva";
}

string elegirEstadoOcupa(Aleatorio& rnd) {  // 60% Confirmada, 25% Pendiente, 15% CheckIn
    int x = rnd.entre(1, 100);
    if (x <= 60) return "Confirmada";
    if (x <= 85) return "Pendiente";
    return "CheckIn";
}

double precioPara(Aleatorio& rnd, const string& clase) {
    int centimos;
    if (clase == "Economica")    centimos = rnd.entre(12000, 35000);
    else if (clase == "Premium") centimos = rnd.entre(30000, 60000);
    else                         centimos = rnd.entre(50000, 120000);
    return centimos / 100.0;
}

void completarPasajero(Aleatorio& rnd, ReservaGen& r) {
    r.pasajero = string(NOMBRES[rnd.entre(0, NUM_NOM - 1)]) + " " + APELLIDOS[rnd.entre(0, NUM_APE - 1)];
    r.dni = to_string(rnd.entre(10000000, 79999999));
    r.clase = elegirClase(rnd);
    r.precio = precioPara(rnd, r.clase);
}

string lineaReserva(int id, const ReservaGen& r, const VueloGen* vuelos) {
    const VueloGen& v = vuelos[r.vuelo];
    return "RES-" + conCeros(id, 6) + "," + r.dni + "," + r.pasajero + "," + v.codigo + "," +
           v.origen + "," + v.destino + "," + v.fecha + "," + v.hora + "," + r.clase + "," +
           r.categoria + "," + precioTexto(r.precio) + "," + r.estado;
}

// Mezcla Fisher-Yates sobre el arreglo de reservas.
void mezclar(Aleatorio& rnd, ReservaGen* arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rnd.entre(0, i);
        ReservaGen t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

int main(int argc, char* argv[]) {
    string carpeta = argc > 1 ? argv[1] : "datos";
    int totalMasivo = argc > 2 ? stoi(argv[2]) : 10000;
    if (totalMasivo > MAX_RES - 500) totalMasivo = MAX_RES - 500;

    Aleatorio rnd(20261201ULL);  // semilla fija: los archivos se pueden regenerar iguales

    // vuelos
    const int NUM_VUELOS = 56;
    VueloGen* vuelos = new VueloGen[MAX_VUELOS];
    const int LLENOS[5] = {3, 11, 19, 30, 44};  // vuelos que quedaran llenos
    for (int i = 0; i < NUM_VUELOS; i++) {
        VueloGen& v = vuelos[i];
        v.codigo = string(AEROLINEAS[i % 4]) + to_string(1000 + i * 37);
        const char* dest = DESTINOS[(i / 2) % NUM_DEST];
        if (i % 2 == 0) { v.origen = "Lima"; v.destino = dest; }
        else            { v.origen = dest;   v.destino = "Lima"; }
        v.lleno = false;
        for (int k = 0; k < 5; k++) if (LLENOS[k] == i) v.lleno = true;
        v.capacidad = v.lleno ? rnd.entre(150, 170) : rnd.entre(180, 300);
        if (i % 6 == 5) {  // comparte fecha y hora con el vuelo anterior
            v.fecha = vuelos[i - 1].fecha;
            v.hora = vuelos[i - 1].hora;
        } else {
            v.fecha = fechaDesde(rnd.entre(0, 89));  // dic 2026 - feb 2027
            v.hora = dosDigitos(rnd.entre(5, 22)) + ":" + dosDigitos(rnd.entre(0, 11) * 5);
        }
    }
    ofstream fv((carpeta + "/vuelos.csv").c_str());
    if (!fv.is_open()) { cout << "No se pudo escribir en " << carpeta << "\n"; delete[] vuelos; return 1; }
    for (int i = 0; i < NUM_VUELOS; i++)
        fv << vuelos[i].codigo << "," << vuelos[i].origen << "," << vuelos[i].destino << ","
           << vuelos[i].fecha << "," << vuelos[i].hora << "," << vuelos[i].capacidad << "\n";
    fv.close();

    // reservas masivas
    ReservaGen* res = new ReservaGen[MAX_RES];
    int n = 0;
    int ocupados[MAX_VUELOS];
    for (int i = 0; i < NUM_VUELOS; i++) ocupados[i] = 0;

    // a) vuelos llenos: capacidad completa + 8 a 14 en espera (categorias mezcladas)
    for (int k = 0; k < 5; k++) {
        int iv = LLENOS[k];
        for (int s = 0; s < vuelos[iv].capacidad; s++) {
            ReservaGen& r = res[n++];
            r.vuelo = iv; completarPasajero(rnd, r);
            r.categoria = elegirCategoria(rnd); r.estado = elegirEstadoOcupa(rnd);
        }
        ocupados[iv] = vuelos[iv].capacidad;
        int espera = rnd.entre(8, 14);
        for (int s = 0; s < espera; s++) {
            ReservaGen& r = res[n++];
            r.vuelo = iv; completarPasajero(rnd, r);
            // al menos 2 de cada categoria; el resto al azar
            r.categoria = s < 2 ? "Regular" : s < 4 ? "Frecuente" : s < 6 ? "Premium" : elegirCategoria(rnd);
            r.estado = "EnEspera";
        }
        for (int c = 0; c < 3; c++) {  // algunas canceladas
            ReservaGen& r = res[n++];
            r.vuelo = iv; completarPasajero(rnd, r);
            r.categoria = elegirCategoria(rnd); r.estado = "Cancelada";
        }
    }
    // b) resto de vuelos: canceladas de base
    for (int i = 0; i < NUM_VUELOS; i++) {
        if (vuelos[i].lleno) continue;
        int canc = rnd.entre(3, 10);
        for (int c = 0; c < canc; c++) {
            ReservaGen& r = res[n++];
            r.vuelo = i; completarPasajero(rnd, r);
            r.categoria = elegirCategoria(rnd); r.estado = "Cancelada";
        }
    }
    // c) se completa hasta totalMasivo con reservas que ocupan asiento, dejando
    //    al menos 5 asientos libres por vuelo (para poder registrar nuevas)
    int intentos = 0;
    while (n < totalMasivo && intentos < 10000000) {
        intentos++;
        int i = rnd.entre(0, NUM_VUELOS - 1);
        if (vuelos[i].lleno || ocupados[i] >= vuelos[i].capacidad - 5) continue;
        ReservaGen& r = res[n++];
        r.vuelo = i; completarPasajero(rnd, r);
        r.categoria = elegirCategoria(rnd); r.estado = elegirEstadoOcupa(rnd);
        ocupados[i]++;
    }
    mezclar(rnd, res, n);
    // Las reservas EnEspera de cada vuelo lleno se dejan en el orden del archivo
    // (el orden de llegada); la mezcla hace que las categorias queden desordenadas.

    ofstream fm((carpeta + "/reservas_masivo.csv").c_str());
    for (int i = 0; i < n; i++) fm << lineaReserva(i + 1, res[i], vuelos) << "\n";
    fm.close();

    // reservas pequeno (30)
    // 6 vuelos (dos de ellos con la misma fecha y hora), estados variados.
    const int VP[6] = {0, 4, 5, 8, 12, 21};
    const int NP = 30;
    ReservaGen* peq = new ReservaGen[NP];
    const char* estadosPeq[5] = {"Confirmada", "Pendiente", "CheckIn", "Confirmada", "Cancelada"};
    for (int i = 0; i < NP; i++) {
        ReservaGen& r = peq[i];
        r.vuelo = VP[rnd.entre(0, 5)];
        completarPasajero(rnd, r);
        r.categoria = elegirCategoria(rnd);
        r.estado = estadosPeq[rnd.entre(0, 4)];
    }
    ofstream fp((carpeta + "/reservas_pequeno.csv").c_str());
    for (int i = 0; i < NP; i++) fp << lineaReserva(i + 1, peq[i], vuelos) << "\n";
    fp.close();

    // resumen
    int esperaTotal = 0, cancTotal = 0;
    for (int i = 0; i < n; i++) {
        if (res[i].estado == "EnEspera") esperaTotal++;
        if (res[i].estado == "Cancelada") cancTotal++;
    }
    cout << "Vuelos generados:          " << NUM_VUELOS << "\n"
         << "Reservas (masivo):         " << n << "  (EnEspera: " << esperaTotal
         << ", Canceladas: " << cancTotal << ")\n"
         << "Reservas (pequeno):        " << NP << "\n"
         << "Vuelos llenos con espera:  ";
    for (int k = 0; k < 5; k++) cout << vuelos[LLENOS[k]].codigo << (k < 4 ? ", " : "\n");

    delete[] vuelos;
    delete[] res;
    delete[] peq;
    return 0;
}
