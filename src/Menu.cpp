#include "Menu.h"
#include <iostream>
#include <sstream>
#include <chrono>

using namespace std::chrono;

static const char* CLASES[3]     = {"Economica", "Premium", "Ejecutiva"};
static const char* CATEGORIAS[3] = {"Premium", "Frecuente", "Regular"};
static const char* INICIALES[2]  = {"Confirmada", "Pendiente"};
static const char* NUEVOS[2]     = {"Confirmada", "CheckIn"};

// pasa a mayusculas (para que res-000001 y RES-000001 sean lo mismo)
static string mayus(string s) {
    for (int i = 0; i < (int)s.size(); i++)
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = s[i] - 'a' + 'A';
    return s;
}

string Menu::leerLinea(const string& mensaje) {
    cout << mensaje;
    string s;
    if (!getline(cin, s)) return "";
    while (!s.empty() && (s[s.size() - 1] == '\r' || s[s.size() - 1] == ' ')) s.erase(s.size() - 1);
    while (!s.empty() && s[0] == ' ') s.erase(0, 1);
    return s;
}

double Menu::leerDouble(const string& mensaje) {
    while (true) {
        string s = leerLinea(mensaje);
        stringstream ss(s);
        double v;
        if (ss >> v) return v;
        if (!cin) return -1;
        cout << "  Valor no valido, intente de nuevo.\n";
    }
}

int Menu::leerEntero(const string& mensaje) {
    while (true) {
        string s = leerLinea(mensaje);
        stringstream ss(s);
        int v;
        if (ss >> v) return v;
        if (!cin) return -1;
        cout << "  Valor no valido, intente de nuevo.\n";
    }
}

string Menu::elegir(const string& mensaje, const char* const opciones[], int n) {
    cout << mensaje << "\n";
    for (int i = 0; i < n; i++) cout << "   " << (i + 1) << ") " << opciones[i] << "\n";
    while (true) {
        int k = leerEntero("  Opcion: ");
        if (k >= 1 && k <= n) return opciones[k - 1];
        if (!cin) return opciones[0];
        cout << "  Opcion fuera de rango.\n";
    }
}

void Menu::cabeceraReservas() {
    cout << "  ID          Fecha      Hora   Vuelo   Ruta                 Pasajero"
            "                 Clase      Categoria  Precio   Estado\n";
}

void Menu::opCargaMasiva() {
    if (!vuelosCargados) {
        string av = leerLinea("Archivo de vuelos [datos/vuelos.csv]: ");
        if (av.empty()) av = "datos/vuelos.csv";
        int nv = sis.cargarVuelos(av);
        if (nv < 0) { cout << "No se pudo abrir " << av << "\n"; return; }
        cout << "Vuelos cargados: " << nv << "\n";
        vuelosCargados = nv > 0;
        if (!vuelosCargados) return;
    }
    string ar = leerLinea("Archivo de reservas [datos/reservas_masivo.csv]: ");
    if (ar.empty()) ar = "datos/reservas_masivo.csv";

    bool abierto;
    steady_clock::time_point t0 = steady_clock::now();
    ResultadoCarga res = sis.cargarReservas(ar, abierto);
    steady_clock::time_point t1 = steady_clock::now();
    if (!abierto) { cout << "No se pudo abrir " << ar << "\n"; return; }

    double ms = duration_cast<microseconds>(t1 - t0).count() / 1000.0;
    cout << "\nLineas leidas:          " << res.leidas
         << "\nReservas insertadas:    " << res.insertadas
         << "\nRechazadas:             " << res.rechazadas
         << "\nPasadas a EnEspera por vuelo lleno: " << res.ajustadas
         << "\n\nCantidad en la tabla hash: " << sis.getHash().getCantidad()
         << "\nCantidad en el arbol AVL:  " << sis.getArbol().getCantidad()
         << "\nCantidad en heaps:         " << sis.totalEnEspera() << "\n";
    for (int i = 0; i < sis.getNumVuelos(); i++) {
        Vuelo* v = sis.getVuelo(i);
        if (!v->espera.vacio())
            cout << "   Heap " << v->codigo << ": " << v->espera.tamano() << " en espera"
                 << " (ocupados " << v->ocupados << "/" << v->capacidad << ")\n";
    }
    cout << "Tiempo de carga: " << ms << " ms\n";
}

void Menu::opRegistrar() {
    if (!vuelosCargados) { cout << "Primero cargue los vuelos (opcion 1).\n"; return; }
    string id = mayus(leerLinea("ID de reserva (ej. RES-020001): "));
    if (sis.buscar(id) != nullptr) { cout << mensajeCodigo(OP_ID_REPETIDO) << "\n"; return; }
    string vuelo = mayus(leerLinea("Codigo de vuelo: "));
    Vuelo* v = sis.buscarVuelo(vuelo);
    if (v == nullptr) { cout << mensajeCodigo(OP_VUELO_NO_EXISTE) << "\n"; return; }
    cout << "  Vuelo " << v->codigo << " " << v->origen << "-" << v->destino << " "
         << v->fecha << " " << v->hora << "  asientos libres: " << v->libres() << "\n";
    string dni = leerLinea("DNI: ");
    string nombre = leerLinea("Nombre del pasajero: ");
    string clase = elegir("Clase:", CLASES, 3);
    string categoria = elegir("Categoria del cliente:", CATEGORIAS, 3);
    double precio = leerDouble("Precio: ");
    string estado = v->lleno() ? "Confirmada" : elegir("Estado inicial:", INICIALES, 2);

    Reserva* r;
    CodigoOp c = sis.registrar(id, dni, nombre, vuelo, clase, categoria, precio, estado, r);
    cout << mensajeCodigo(c) << "\n";
    if (c == OP_OK) {
        if (r->estado == "EnEspera")
            cout << "El vuelo esta lleno: la reserva queda EnEspera (posicion en el heap asignada).\n";
        cabeceraReservas();
        cout << "  " << reservaATexto(r) << "\n";
    }
}

void Menu::opBuscar() {
    string id = mayus(leerLinea("ID a buscar: "));
    steady_clock::time_point t0 = steady_clock::now();
    Reserva* r = sis.buscar(id);
    steady_clock::time_point t1 = steady_clock::now();
    if (r == nullptr) cout << mensajeCodigo(OP_ID_NO_EXISTE) << "\n";
    else {
        cout << "\n  ID:        " << r->idReserva  << "\n  DNI:       " << r->dni
             << "\n  Pasajero:  " << r->pasajero   << "\n  Vuelo:     " << r->codigoVuelo
             << "\n  Ruta:      " << r->origen << " - " << r->destino
             << "\n  Salida:    " << r->fecha << " " << r->hora
             << "\n  Clase:     " << r->clase      << "\n  Categoria: " << r->categoria
             << "\n  Precio:    " << formatoPrecio(r->precio) << "\n  Estado:    " << r->estado << "\n";
    }
    cout << "  (busqueda en hash: " << duration_cast<nanoseconds>(t1 - t0).count() << " ns)\n";
}

void Menu::opListado() {
    int n = leerEntero("Cuantas reservas mostrar (20 o 50): ");
    if (n <= 0) n = 20;
    if (n > 500) n = 500;
    Reserva** lista = new Reserva*[n];
    int k = sis.listarCronologico(lista, n);
    cabeceraReservas();
    for (int i = 0; i < k; i++) cout << "  " << reservaATexto(lista[i]) << "\n";
    cout << "Mostradas " << k << " de " << sis.getArbol().getCantidad() << " reservas.\n";
    delete[] lista;
}

// Acepta la fecha como 07/12/2026, 7/12/2026 o 2026-12-07 y la devuelve
// como AAAA-MM-DD (el formato en que se guarda). Devuelve "" si no es valida.
static string normalizarFecha(const string& texto) {
    if (fechaValida(texto)) return texto;            // ya viene como AAAA-MM-DD
    int partes[3] = {0, 0, 0}, n = 0, digitos = 0;
    for (int i = 0; i < (int)texto.size(); i++) {
        char c = texto[i];
        if (c >= '0' && c <= '9') {
            if (n > 2) return "";
            partes[n] = partes[n] * 10 + (c - '0');
            digitos++;
        } else if (c == '/' || c == '-') {
            if (digitos == 0) return "";
            n++; digitos = 0;
        } else return "";
    }
    if (n != 2 || digitos == 0) return "";
    int dia = partes[0], mes = partes[1], anio = partes[2];
    if (anio < 100) anio += 2000;                    // 07/12/26 -> 2026
    string f = to_string(anio) + "-" + (mes < 10 ? "0" : "") + to_string(mes) + "-" +
               (dia < 10 ? "0" : "") + to_string(dia);
    return fechaValida(f) ? f : "";
}

// AAAA-MM-DD -> DD/MM/AAAA (solo para mostrar)
static string fechaBonita(const string& f) {
    return f.substr(8, 2) + "/" + f.substr(5, 2) + "/" + f.substr(0, 4);
}

void Menu::opRango() {
    if (sis.getNumVuelos() == 0) { cout << "Primero cargue los datos (opcion 1).\n"; return; }
    string primera = sis.getVuelo(0)->fecha, ultima = primera;
    for (int i = 1; i < sis.getNumVuelos(); i++) {
        if (sis.getVuelo(i)->fecha < primera) primera = sis.getVuelo(i)->fecha;
        if (sis.getVuelo(i)->fecha > ultima) ultima = sis.getVuelo(i)->fecha;
    }
    cout << "Hay vuelos del " << fechaBonita(primera) << " al " << fechaBonita(ultima) << ".\n"
         << "Escriba las fechas como DD/MM/AAAA (ejemplo: 07/12/2026).\n";
    string ini = normalizarFecha(leerLinea("Fecha inicial: "));
    if (ini.empty()) { cout << "Fecha no valida. Use DD/MM/AAAA, por ejemplo 07/12/2026.\n"; return; }
    string textoFin = leerLinea("Fecha final (Enter = solo ese dia): ");
    string fin = textoFin.empty() ? ini : normalizarFecha(textoFin);
    if (fin.empty()) { cout << "Fecha no valida. Use DD/MM/AAAA, por ejemplo 13/12/2026.\n"; return; }
    if (ini > fin) { cout << "La fecha inicial es posterior a la final.\n"; return; }

    const int MUESTRA = 10;
    Reserva* muestra[MUESTRA];
    long visitados;
    int total = sis.consultarRango(ini, fin, muestra, MUESTRA, visitados);
    cout << "Del " << fechaBonita(ini) << " al " << fechaBonita(fin) << ":\n"
         << "Reservas encontradas: " << total << "\n"
         << "Nodos visitados: " << visitados << " de " << sis.getArbol().getCantidad()
         << " (las ramas fuera del rango se descartan)\n";
    if (total > 0) {
        cout << "Muestra:\n";
        cabeceraReservas();
        for (int i = 0; i < total && i < MUESTRA; i++) cout << "  " << reservaATexto(muestra[i]) << "\n";
    }
}

void Menu::opActualizar() {
    string id = mayus(leerLinea("ID de la reserva: "));
    Reserva* r = sis.buscar(id);
    if (r == nullptr) { cout << mensajeCodigo(OP_ID_NO_EXISTE) << "\n"; return; }
    cout << "Estado actual: " << r->estado << "\n";
    string nuevo = elegir("Nuevo estado:", NUEVOS, 2);
    CodigoOp c = sis.actualizarEstado(id, nuevo);
    cout << mensajeCodigo(c) << "\n";
    if (c == OP_OK) cout << "Estado actual: " << r->estado << "\n";
}

void Menu::opCancelar() {
    string id = mayus(leerLinea("ID de la reserva a cancelar: "));
    Reserva* r = sis.buscar(id);
    string estadoPrevio = r ? r->estado : "";
    Reserva* promovido;
    CodigoOp c = sis.cancelar(id, promovido);
    cout << mensajeCodigo(c) << "\n";
    if (c != OP_OK) return;
    if (estadoPrevio == "EnEspera")
        cout << "Caso B: la reserva estaba EnEspera y se retiro del heap de su vuelo.\n";
    else if (promovido != nullptr) {
        cout << "Caso A: se libero un asiento. Pasajero promovido desde la lista de espera:\n";
        cabeceraReservas();
        cout << "  " << reservaATexto(promovido) << "\n";
    } else
        cout << "Caso A: se libero un asiento (no habia lista de espera).\n";
}

void Menu::opEliminar() {
    string id = mayus(leerLinea("ID de la reserva Cancelada a eliminar: "));
    CodigoOp c = sis.eliminarFisica(id);
    cout << mensajeCodigo(c) << "\n";
    if (c == OP_OK)
        cout << "Verificacion: busqueda por ID -> "
             << (sis.buscar(id) == nullptr ? "no existe (correcto)" : "TODAVIA EXISTE") << "\n";
}

void Menu::opListaEspera() {
    string codigo = mayus(leerLinea("Codigo de vuelo: "));
    Vuelo* v;
    Reserva* lista[1000];
    int k = sis.listaEspera(codigo, lista, 1000, v);
    if (k < 0) { cout << mensajeCodigo(OP_VUELO_NO_EXISTE) << "\n"; return; }
    cout << "Vuelo " << v->codigo << "  ocupados " << v->ocupados << "/" << v->capacidad
         << "  en espera: " << v->espera.tamano() << "\n";
    if (k == 0) { cout << "La lista de espera esta vacia.\n"; return; }
    cout << "Siguiente a atender (raiz del heap): " << v->espera.verMax()->idReserva << " - "
         << v->espera.verMax()->pasajero << " (" << v->espera.verMax()->categoria << ")\n";
    cout << "Lista completa en orden de prioridad:\n";
    for (int i = 0; i < k; i++)
        cout << "  " << (i + 1) << ". " << lista[i]->idReserva << "  " << lista[i]->categoria
             << "  llegada #" << lista[i]->ordenLlegada << "  " << lista[i]->pasajero << "\n";
}

void Menu::opEstadisticas() {
    Estadisticas* e = new Estadisticas();
    sis.estadisticas(*e);
    cout << "\nTotal de reservas: " << e->total
         << "\nIngresos totales (Confirmada + CheckIn): " << formatoPrecio(e->ingresos)
         << "\nPrecio promedio: " << formatoPrecio(e->precioPromedio) << "\n\nPor destino:\n";
    for (int i = 0; i < e->numDestinos; i++)
        cout << "   " << e->destinos[i] << ": " << e->porDestino[i] << "\n";
    cout << "Por clase:\n";
    for (int i = 0; i < 3; i++) cout << "   " << Sistema::NOMBRES_CLASE[i] << ": " << e->porClase[i] << "\n";
    cout << "Por estado:\n";
    for (int i = 0; i < 5; i++) cout << "   " << Sistema::NOMBRES_ESTADO[i] << ": " << e->porEstado[i] << "\n";
    if (e->mayorPrecio) {
        cout << "Mayor precio: "; cout << reservaATexto(e->mayorPrecio) << "\n";
        cout << "Menor precio: "; cout << reservaATexto(e->menorPrecio) << "\n";
    }
    if (e->vueloMasEspera)
        cout << "Vuelo con mas pasajeros en espera: " << e->vueloMasEspera->codigo << " ("
             << e->vueloMasEspera->espera.tamano() << ")\n";
    else
        cout << "Ningun vuelo tiene pasajeros en espera.\n";
    delete e;
}

void Menu::opConsistencia() {
    string detalle;
    bool ok = sis.verificarConsistencia(detalle);
    cout << (ok ? "Estructuras consistentes: hash, arbol y heaps coinciden.\n"
                : "Se encontraron inconsistencias:\n" + detalle);
    cout << "Hash: " << sis.getHash().getCantidad() << " reservas, tamano " << sis.getHash().getTam()
         << ", factor de carga " << sis.getHash().factorCarga()
         << ", cadena mas larga " << sis.getHash().cadenaMasLarga() << "\n"
         << "Arbol AVL: " << sis.getArbol().getCantidad() << " nodos, altura " << sis.getArbol().altura() << "\n";
}

void Menu::opVuelos() {
    for (int i = 0; i < sis.getNumVuelos(); i++) {
        Vuelo* v = sis.getVuelo(i);
        cout << "  " << v->codigo << "  " << v->origen << "-" << v->destino << "  " << v->fecha
             << " " << v->hora << "  " << v->ocupados << "/" << v->capacidad
             << (v->lleno() ? "  LLENO" : "") << "  espera: " << v->espera.tamano() << "\n";
    }
}

void Menu::ejecutar() {
    while (true) {
        cout << "\n========= SISTEMA DE RESERVAS AEREAS =========\n"
                " 1. Carga masiva (vuelos y reservas)\n"
                " 2. Registrar nueva reserva\n"
                " 3. Buscar reserva por ID\n"
                " 4. Listado cronologico\n"
                " 5. Consulta por rango de fechas\n"
                " 6. Actualizar estado\n"
                " 7. Cancelar reserva\n"
                " 8. Eliminar fisicamente una reserva Cancelada\n"
                " 9. Lista de espera de un vuelo\n"
                "10. Estadisticas generales\n"
                "11. Verificar consistencia y metricas\n"
                "12. Ver vuelos\n"
                " 0. Salir\n";
        int op = leerEntero("Opcion: ");
        if (!cin || op == 0) break;
        cout << "\n";
        switch (op) {
            case 1:  opCargaMasiva(); break;
            case 2:  opRegistrar(); break;
            case 3:  opBuscar(); break;
            case 4:  opListado(); break;
            case 5:  opRango(); break;
            case 6:  opActualizar(); break;
            case 7:  opCancelar(); break;
            case 8:  opEliminar(); break;
            case 9:  opListaEspera(); break;
            case 10: opEstadisticas(); break;
            case 11: opConsistencia(); break;
            case 12: opVuelos(); break;
            default: cout << "Opcion no valida.\n";
        }
    }
    cout << "Hasta luego.\n";
}
