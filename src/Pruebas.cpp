// Pruebas del proyecto: los 8 escenarios del enunciado + tiempos y metricas
// uso: pruebas [carpeta]   o   pruebas --escala archivo.csv (para la tabla de escalabilidad)
#include "Sistema.h"
#include <iostream>
#include <chrono>

using namespace std::chrono;

static int fallos = 0;

static void check(bool cond, const string& texto) {
    cout << (cond ? "  [OK]    " : "  [FALLA] ") << texto << "\n";
    if (!cond) fallos++;
}

static void titulo(const string& t) {
    cout << "\n==================================================================\n"
         << t << "\n==================================================================\n";
}

static string fmt(double v, int dec) {
    string s = formatoPrecio(v);  // 2 decimales
    if (dec == 3) {
        double x = v * 1000.0;
        long entero = (long)(x + 0.5);
        string d = to_string(entero % 1000);
        while (d.size() < 3) d = "0" + d;
        s = to_string(entero / 1000) + "." + d;
    }
    return s;
}

// Recolecta todas las reservas (en orden cronologico) en un arreglo.
struct Coleccion { Reserva** arr; int n; };
static void agregar(Reserva* r, void* c) { Coleccion* col = (Coleccion*)c; col->arr[col->n++] = r; }
static Reserva** todas(const Sistema& s, int& n) {
    Coleccion c;
    c.arr = new Reserva*[s.getArbol().getCantidad() + 1];
    c.n = 0;
    s.getArbol().recorrer(agregar, &c);
    n = c.n;
    return c.arr;
}

// Conteo por fuerza bruta (para validar la consulta por rango).
struct CtxRango { string ini, fin; int n; };
static void contarRango(Reserva* r, void* c) {
    CtxRango* x = (CtxRango*)c;
    if (r->fecha >= x->ini && r->fecha <= x->fin) x->n++;
}

static Sistema* cargar(const string& carpeta, const string& archivo, ResultadoCarga& res, double& ms) {
    Sistema* s = new Sistema();
    bool ab;
    steady_clock::time_point t0 = steady_clock::now();
    s->cargarVuelos(carpeta + "/vuelos.csv");
    res = s->cargarReservas(carpeta + "/" + archivo, ab);
    steady_clock::time_point t1 = steady_clock::now();
    ms = duration_cast<nanoseconds>(t1 - t0).count() / 1e6;
    if (!ab) cout << "No se pudo abrir " << archivo << "\n";
    return s;
}

static void imprimir(Reserva* r) { cout << "    " << reservaATexto(r) << "\n"; }

// Arbol binario de busqueda SIN balancear, solo para comparar la altura
// en el escenario de orden desfavorable.
struct NodoABB { Reserva* d; NodoABB* izq; NodoABB* der; };
static int alturaABB(NodoABB* n) {  // iterativa por niveles no hace falta: el ABB degenerado es una lista
    int h = 0;
    while (n) { h++; n = n->der ? n->der : n->izq; }
    return h;
}
static void liberarABB(NodoABB* n) {
    while (n) { NodoABB* s = n->der ? n->der : n->izq; delete n; n = s; }
}

static int modoEscala(const string& archivo) {
    // carpeta = directorio del archivo
    string carpeta = ".", nombre = archivo;
    int p = (int)archivo.rfind('/');
    if (p >= 0) { carpeta = archivo.substr(0, p); nombre = archivo.substr(p + 1); }
    ResultadoCarga res; double msCarga;
    Sistema* s = cargar(carpeta, nombre, res, msCarga);
    int n; Reserva** arr = todas(*s, n);

    // IDs a buscar (pseudoaleatorios entre los existentes)
    const int REP = 1000;
    string* ids = new string[REP];
    unsigned long long x = 12345;
    for (int i = 0; i < REP; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        ids[i] = arr[(x >> 33) % n]->idReserva;
    }
    // orden "de archivo" para la secuencial: el de la hash
    Coleccion c; c.arr = new Reserva*[n]; c.n = 0;
    s->getHash().recorrer(agregar, &c);

    long suma = 0;
    steady_clock::time_point t0 = steady_clock::now();
    for (int i = 0; i < REP; i++) suma += s->buscar(ids[i]) ? 1 : 0;
    steady_clock::time_point t1 = steady_clock::now();
    for (int i = 0; i < REP; i++)
        for (int k = 0; k < c.n; k++)
            if (c.arr[k]->idReserva == ids[i]) { suma++; break; }
    steady_clock::time_point t2 = steady_clock::now();
    Reserva* muestra[10]; long visit = 0;
    steady_clock::time_point t3 = steady_clock::now();
    for (int i = 0; i < 100; i++) s->consultarRango("2027-01-01", "2027-01-31", muestra, 10, visit);
    steady_clock::time_point t4 = steady_clock::now();

    cout << "ESCALA n=" << n
         << " carga_ms=" << fmt(msCarga, 3)
         << " hash_us=" << fmt(duration_cast<nanoseconds>(t1 - t0).count() / 1000.0 / REP, 3)
         << " secuencial_us=" << fmt(duration_cast<nanoseconds>(t2 - t1).count() / 1000.0 / REP, 3)
         << " rango_us=" << fmt(duration_cast<nanoseconds>(t4 - t3).count() / 1000.0 / 100, 3)
         << " altura=" << s->getArbol().altura()
         << " factor=" << fmt(s->getHash().factorCarga(), 3)
         << " cadena_max=" << s->getHash().cadenaMasLarga()
         << " (chk " << suma << ")\n";
    delete[] ids; delete[] arr; delete[] c.arr; delete s;
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc > 2 && string(argv[1]) == "--escala") return modoEscala(argv[2]);
    string carpeta = argc > 1 ? argv[1] : "datos";

    // escenario 1
    titulo("ESCENARIO 1. Carga masiva");
    ResultadoCarga res; double msCarga;
    Sistema* s = cargar(carpeta, "reservas_masivo.csv", res, msCarga);
    cout << "  Vuelos cargados:              " << s->getNumVuelos() << "\n"
         << "  Lineas leidas:                " << res.leidas << "\n"
         << "  Insertadas en la tabla hash:  " << s->getHash().getCantidad() << "\n"
         << "  Insertadas en el arbol AVL:   " << s->getArbol().getCantidad() << "\n"
         << "  Insertadas en heaps:          " << s->totalEnEspera() << "\n"
         << "  Rechazadas:                   " << res.rechazadas << "\n"
         << "  Tiempo de carga:              " << fmt(msCarga, 3) << " ms\n"
         << "  Heaps con pasajeros en espera:\n";
    for (int i = 0; i < s->getNumVuelos(); i++) {
        Vuelo* v = s->getVuelo(i);
        if (!v->espera.vacio())
            cout << "    " << v->codigo << " (" << v->origen << "-" << v->destino << ", "
                 << v->ocupados << "/" << v->capacidad << "): " << v->espera.tamano() << " en espera\n";
    }
    string det;
    check(s->getHash().getCantidad() >= 10000, "al menos 10 000 reservas cargadas");
    check(s->getHash().getCantidad() == s->getArbol().getCantidad(), "hash y arbol tienen la misma cantidad");
    check(s->verificarConsistencia(det), "consistencia hash-arbol-heaps");

    // escenario 2
    titulo("ESCENARIO 2. Busqueda por ID (tabla hash)");
    const char* ids[4] = {"RES-000001", "RES-005000", "RES-010000", "RES-999999"};
    const char* desc[4] = {"primera", "intermedia", "ultima", "inexistente"};
    for (int i = 0; i < 4; i++) {
        Reserva* r = s->buscar(ids[i]);
        cout << "  " << desc[i] << " (" << ids[i] << "): ";
        if (r) { cout << "encontrada\n"; imprimir(r); }
        else cout << "no existe una reserva con ese ID\n";
    }
    check(s->buscar("RES-000001") && s->buscar("RES-005000") && s->buscar("RES-010000"),
          "se encuentran la primera, una intermedia y la ultima");
    check(s->buscar("RES-999999") == nullptr, "el ID inexistente devuelve error");

    // escenario 3
    titulo("ESCENARIO 3. Ordenamiento cronologico (recorrido en orden del AVL)");
    {
        ResultadoCarga rp; double msp;
        Sistema* sp = cargar(carpeta, "reservas_pequeno.csv", rp, msp);
        Reserva* lista[50];
        int k = sp->listarCronologico(lista, 20);
        cout << "  Archivo pequeno (" << sp->getArbol().getCantidad() << " reservas), primeras " << k << ":\n";
        bool orden = true;
        for (int i = 0; i < k; i++) {
            imprimir(lista[i]);
            if (i > 0 && compararClave(lista[i - 1], lista[i]) >= 0) orden = false;
        }
        check(orden, "archivo pequeno: orden por fecha, hora e id");
        delete sp;
        k = s->listarCronologico(lista, 20);
        cout << "  Archivo masivo, primeras " << k << ":\n";
        orden = true;
        for (int i = 0; i < k; i++) {
            imprimir(lista[i]);
            if (i > 0 && compararClave(lista[i - 1], lista[i]) >= 0) orden = false;
        }
        check(orden, "archivo masivo: orden por fecha, hora e id");
        int n; Reserva** arr = todas(*s, n);
        bool todoOrden = true;
        for (int i = 1; i < n; i++) if (compararClave(arr[i - 1], arr[i]) >= 0) todoOrden = false;
        check(todoOrden, "el recorrido completo (" + to_string(n) + " reservas) esta ordenado");
        delete[] arr;
    }

    // escenario 4
    titulo("ESCENARIO 4. Consulta por rango de fechas");
    {
        const char* rIni[3] = {"2026-12-07", "2027-01-01", "2027-06-01"};
        const char* rFin[3] = {"2026-12-13", "2027-01-31", "2027-06-30"};
        const char* rDes[3] = {"una semana", "un mes", "sin resultados"};
        for (int q = 0; q < 3; q++) {
            Reserva* muestra[5]; long visit;
            int total = s->consultarRango(rIni[q], rFin[q], muestra, 5, visit);
            CtxRango cr; cr.ini = rIni[q]; cr.fin = rFin[q]; cr.n = 0;
            s->getHash().recorrer(contarRango, &cr);
            cout << "  Rango " << rDes[q] << " [" << rIni[q] << ", " << rFin[q] << "]: "
                 << total << " reservas; nodos visitados " << visit << " de "
                 << s->getArbol().getCantidad() << "\n";
            for (int i = 0; i < total && i < 5; i++) imprimir(muestra[i]);
            check(total == cr.n, string("el conteo coincide con la fuerza bruta (") + to_string(cr.n) + ")");
            if (q < 2) check(visit < s->getArbol().getCantidad(), "no se recorre todo el arbol");
            else check(total == 0, "el rango sin datos devuelve 0");
        }
    }

    // escenario 5
    titulo("ESCENARIO 5. Insercion individual");
    {
        Vuelo* libre = nullptr; Vuelo* lleno = nullptr;
        for (int i = 0; i < s->getNumVuelos(); i++) {
            Vuelo* v = s->getVuelo(i);
            if (!libre && !v->lleno()) libre = v;
            if (!lleno && v->lleno()) lleno = v;
        }
        Reserva* r;
        CodigoOp c = s->registrar("RES-020001", "71234567", "Rodrigo Ventocilla", libre->codigo,
                                  "Economica", "Frecuente", 199.90, "Confirmada", r);
        cout << "  Registro en vuelo con asientos (" << libre->codigo << "): " << mensajeCodigo(c) << "\n";
        imprimir(r);
        check(c == OP_OK && r->estado == "Confirmada", "queda Confirmada");
        check(s->buscar("RES-020001") == r, "aparece en la busqueda por ID");
        Reserva* muestra[2000]; long visit;
        int total = s->consultarRango(r->fecha, r->fecha, muestra, 2000, visit);
        bool esta = false;
        for (int i = 0; i < total && i < 2000; i++) if (muestra[i] == r) esta = true;
        check(esta, "aparece en la consulta por rango del arbol (" + r->fecha + ")");

        int antes = lleno->espera.tamano();
        c = s->registrar("RES-020002", "70011223", "Diego Salazar", lleno->codigo,
                         "Premium", "Regular", 350.00, "Confirmada", r);
        cout << "  Registro en vuelo lleno (" << lleno->codigo << "): " << mensajeCodigo(c) << "\n";
        imprimir(r);
        check(c == OP_OK && r->estado == "EnEspera", "queda EnEspera");
        check(lleno->espera.tamano() == antes + 1 && lleno->espera.buscarPos("RES-020002") >= 0,
              "entra al heap del vuelo (" + to_string(antes) + " -> " + to_string(lleno->espera.tamano()) + ")");
        c = s->registrar("RES-020001", "1", "X", libre->codigo, "Economica", "Regular", 1, "Confirmada", r);
        check(c == OP_ID_REPETIDO, "un ID repetido se rechaza");
        c = s->registrar("RES-020003", "1", "X", "XX9999", "Economica", "Regular", 1, "Confirmada", r);
        check(c == OP_VUELO_NO_EXISTE, "un vuelo inexistente se rechaza");
    }

    // escenario 6
    titulo("ESCENARIO 6. Actualizacion de estado, cancelacion y eliminacion fisica");
    {
        int n; Reserva** arr = todas(*s, n);
        int hechos = 0;
        cout << "  Actualizaciones:\n";
        for (int i = 0; i < n && hechos < 10; i++) {
            Reserva* r = arr[i];
            string antes = r->estado, nuevo;
            if (hechos < 5 && antes == "Pendiente") nuevo = "Confirmada";
            else if (hechos >= 5 && antes == "Confirmada") nuevo = "CheckIn";
            else continue;
            CodigoOp c = s->actualizarEstado(r->idReserva, nuevo);
            // el arbol apunta al mismo objeto: se comprueba que siga alli con el nuevo estado
            Reserva* enArbol = s->getArbol().contiene(r) ? r : nullptr;
            cout << "    " << r->idReserva << ": " << antes << " -> " << s->buscar(r->idReserva)->estado
                 << (c == OP_OK ? "" : "  (" + mensajeCodigo(c) + ")") << "\n";
            if (c == OP_OK && enArbol && enArbol->estado == nuevo) hechos++;
        }
        check(hechos == 10, "10 transiciones validas reflejadas en hash y arbol");
        // transiciones no permitidas
        string idCan, idCheck;
        for (int i = 0; i < n; i++) {
            if (idCan.empty() && arr[i]->estado == "Cancelada") idCan = arr[i]->idReserva;
            if (idCheck.empty() && arr[i]->estado == "CheckIn") idCheck = arr[i]->idReserva;
        }
        check(s->actualizarEstado(idCan, "Confirmada") == OP_YA_CANCELADA,
              "una reserva Cancelada no se puede reactivar (" + idCan + ")");
        check(s->actualizarEstado(idCheck, "Confirmada") == OP_TRANSICION_INVALIDA,
              "CheckIn -> Confirmada se rechaza (" + idCheck + ")");

        // cancelar y eliminar 10 reservas de vuelos sin lista de espera
        string borrar[10]; string fechas[10]; int nb = 0;
        int antesHash = s->getHash().getCantidad();
        cout << "  Cancelacion y eliminacion fisica:\n";
        for (int i = 0; i < n && nb < 10; i += 97) {
            Reserva* r = arr[i];
            Vuelo* v = s->buscarVuelo(r->codigoVuelo);
            if (!ocupaAsiento(r) || !v->espera.vacio()) continue;
            int ocup = v->ocupados;
            Reserva* prom;
            CodigoOp c1 = s->cancelar(r->idReserva, prom);
            bool asientoLiberado = v->ocupados == ocup - 1;
            borrar[nb] = r->idReserva; fechas[nb] = r->fecha;
            CodigoOp c2 = s->eliminarFisica(borrar[nb]);
            cout << "    " << borrar[nb] << " (" << v->codigo << "): cancelar -> " << (c1 == OP_OK ? "OK" : "error")
                 << ", asiento liberado " << (asientoLiberado ? "si" : "no")
                 << ", eliminar -> " << (c2 == OP_OK ? "OK" : "error") << "\n";
            nb++;
        }
        delete[] arr;  // contiene punteros ya liberados: no se vuelve a usar
        bool ninguno = true;
        for (int i = 0; i < nb; i++) {
            if (s->buscar(borrar[i]) != nullptr) ninguno = false;
            Reserva* muestra[3000]; long v;
            int t = s->consultarRango(fechas[i], fechas[i], muestra, 3000, v);
            for (int k = 0; k < t && k < 3000; k++) if (muestra[k]->idReserva == borrar[i]) ninguno = false;
        }
        check(nb == 10, "10 reservas canceladas y eliminadas");
        check(ninguno, "ya no aparecen en la busqueda por ID ni en las consultas del arbol");
        check(s->getHash().getCantidad() == antesHash - 10 && s->getArbol().getCantidad() == antesHash - 10,
              "hash y arbol bajaron en 10 (" + to_string(s->getHash().getCantidad()) + ")");
        check(s->eliminarFisica(idCheck) == OP_NO_CANCELADA, "no se elimina una reserva que no esta Cancelada");
        check(s->verificarConsistencia(det), "consistencia tras actualizaciones y eliminaciones");
    }

    // escenario 7
    titulo("ESCENARIO 7. Lista de espera (max-heap)");
    {
        Estadisticas* e = new Estadisticas();
        s->estadisticas(*e);
        Vuelo* v = e->vueloMasEspera;
        delete e;
        Reserva* lista[100];
        Vuelo* vv;
        int k = s->listaEspera(v->codigo, lista, 100, vv);
        cout << "  Vuelo " << v->codigo << " (" << v->ocupados << "/" << v->capacidad << "), "
             << k << " en espera. Raiz del heap: " << v->espera.verMax()->idReserva << "\n"
             << "  Lista en orden de prioridad:\n";
        bool ordenOk = true;
        for (int i = 0; i < k; i++) {
            cout << "    " << (i + 1) << ". " << lista[i]->idReserva << "  " << lista[i]->categoria
                 << "  llegada #" << lista[i]->ordenLlegada << "\n";
            if (i > 0 && MaxHeap::mayorPrioridad(lista[i], lista[i - 1])) ordenOk = false;
        }
        check(ordenOk, "orden: Premium > Frecuente > Regular y, en empate, llegada");
        check(v->espera.tamano() == k, "listar no altera el heap (trabaja sobre una copia)");

        // Caso A: cancelar una reserva con asiento
        Reserva* esperado = lista[0];
        int n; Reserva** arr = todas(*s, n);
        Reserva* conAsiento = nullptr;
        for (int i = 0; i < n; i++)
            if (arr[i]->codigoVuelo == v->codigo && ocupaAsiento(arr[i])) { conAsiento = arr[i]; break; }
        delete[] arr;
        Reserva* prom;
        CodigoOp c = s->cancelar(conAsiento->idReserva, prom);
        cout << "  Caso A: se cancela " << conAsiento->idReserva << " (" << mensajeCodigo(c) << ")\n"
             << "  Promovido: " << (prom ? prom->idReserva + " " + prom->categoria + " -> " + prom->estado : "ninguno") << "\n";
        check(prom == esperado && prom->estado == "Confirmada",
              "se promueve al de mayor prioridad (" + esperado->idReserva + ") y queda Confirmada");
        check(v->ocupados == v->capacidad, "el vuelo sigue lleno tras la promocion");

        // Caso B: cancelar una reserva EnEspera que no esta en la raiz
        int pos = v->espera.tamano() / 2;  // posicion intermedia del arreglo
        Reserva* objetivo = v->espera.obtener(pos);
        int tamAntes = v->espera.tamano();
        c = s->cancelar(objetivo->idReserva, prom);
        cout << "  Caso B: se cancela " << objetivo->idReserva << " (posicion " << pos
             << " del arreglo, no es la raiz): " << mensajeCodigo(c) << "\n";
        check(c == OP_OK && objetivo->estado == "Cancelada", "queda Cancelada");
        check(v->espera.tamano() == tamAntes - 1 && v->espera.buscarPos(objetivo->idReserva) < 0,
              "se retiro del heap");
        check(v->espera.esValido(), "el heap sigue siendo valido");
        k = s->listaEspera(v->codigo, lista, 100, vv);
        cout << "  Lista resultante:\n";
        for (int i = 0; i < k; i++)
            cout << "    " << (i + 1) << ". " << lista[i]->idReserva << "  " << lista[i]->categoria
                 << "  llegada #" << lista[i]->ordenLlegada << "\n";
        check(s->verificarConsistencia(det), "consistencia tras la lista de espera");
    }

    // escenario 8
    titulo("ESCENARIO 8. Casos extremos de las estructuras");
    {
        TablaHash th;
        int objetivo = th.funcionHash("RES-000001");
        string colisiones[50]; int nc = 0;
        for (int i = 0; nc < 50; i++) {
            string id = "RES-C" + to_string(i);
            if (th.funcionHash(id) == objetivo) colisiones[nc++] = id;
        }
        for (int i = 0; i < 50; i++) {
            Reserva* r = new Reserva();
            r->idReserva = colisiones[i];
            th.insertar(r);
        }
        int encontrados = 0;
        for (int i = 0; i < 50; i++) if (th.buscar(colisiones[i])) encontrados++;
        cout << "  50 IDs con hash = " << objetivo << ": " << colisiones[0] << ", " << colisiones[1]
             << ", " << colisiones[2] << ", ..., " << colisiones[49] << "\n"
             << "  Longitud de la cadena en la posicion " << objetivo << ": " << th.longitudCadena(objetivo) << "\n";
        check(th.longitudCadena(objetivo) == 50, "los 50 IDs caen en la misma posicion");
        check(encontrados == 50, "se encuentran los 50");

        // orden desfavorable
        ArbolAVL avl;
        Reserva* rs = new Reserva[1000];
        NodoABB* abb = nullptr; NodoABB* ultimo = nullptr;
        for (int i = 0; i < 1000; i++) {
            int dia = i / 96, minutos = (i % 96) * 15;  // 96 salidas por dia
            rs[i].fecha = "2027-03-" + string(dia + 1 < 10 ? "0" : "") + to_string(dia + 1);
            rs[i].hora = string(minutos / 60 < 10 ? "0" : "") + to_string(minutos / 60) + ":" +
                         string(minutos % 60 < 10 ? "0" : "") + to_string(minutos % 60);
            rs[i].idReserva = "ORD-" + to_string(100000 + i);
            avl.insertar(&rs[i]);
            NodoABB* nodo = new NodoABB(); nodo->d = &rs[i]; nodo->izq = nodo->der = nullptr;
            if (!abb) abb = nodo; else ultimo->der = nodo;  // clave creciente: siempre a la derecha
            ultimo = nodo;
        }
        bool crecientes = true;
        for (int i = 1; i < 1000; i++) if (compararClave(&rs[i - 1], &rs[i]) >= 0) crecientes = false;
        cout << "  1000 reservas con fecha/hora estrictamente crecientes ("
             << rs[0].fecha << " " << rs[0].hora << " ... " << rs[999].fecha << " " << rs[999].hora << ")\n"
             << "  Altura del AVL: " << avl.altura() << "   (log2(1000) = 9.97; ABB sin balancear: "
             << alturaABB(abb) << ")\n";
        check(crecientes, "las claves son estrictamente crecientes");
        check(avl.altura() <= 11 && avl.esAVLValido(), "la altura del AVL es logaritmica y el arbol es valido");
        liberarABB(abb);
        delete[] rs;  // el destructor del AVL solo libera sus nodos, no las reservas
    }

    // METRICAS
    titulo("METRICAS DE LAS ESTRUCTURAS (sistema con la carga masiva)");
    {
        const TablaHash& h = s->getHash();
        int ocup = h.posicionesOcupadas();
        int hist[8] = {0};
        for (int i = 0; i < h.getTam(); i++) { int L = h.longitudCadena(i); hist[L < 7 ? L : 7]++; }
        cout << "  Tabla hash: " << h.getCantidad() << " reservas, tamano " << h.getTam()
             << ", factor de carga " << fmt(h.factorCarga(), 3) << "\n"
             << "  Posiciones ocupadas: " << ocup << " (" << fmt(100.0 * ocup / h.getTam(), 2) << " %)"
             << ", longitud media de cadenas no vacias " << fmt((double)h.getCantidad() / ocup, 3) << "\n"
             << "  Cadena mas larga: " << h.cadenaMasLarga() << "\n  Distribucion de longitudes:";
        for (int L = 0; L < 8; L++) if (hist[L]) cout << "  [" << L << (L == 7 ? "+" : "") << "]=" << hist[L];
        cout << "\n  Arbol AVL: " << s->getArbol().getCantidad() << " nodos, altura " << s->getArbol().altura()
             << " (log2 n = 13.29; maximo teorico AVL ~ 1.44 log2 n = 19)\n";
    }

    // TIEMPOS
    delete s;
    titulo("TIEMPOS (sistema nuevo, carga masiva)");
    {
        ResultadoCarga r2; double ms;
        Sistema* t = cargar(carpeta, "reservas_masivo.csv", r2, ms);
        cout << "  Carga masiva (" << r2.insertadas << " reservas, 1 vez): " << fmt(ms, 3) << " ms\n";

        int n; Reserva** arr = todas(*t, n);
        const int REP = 1000;
        string* ids = new string[REP];
        unsigned long long x = 98765;
        for (int i = 0; i < REP; i++) {
            x = x * 6364136223846793005ULL + 1442695040888963407ULL;
            ids[i] = arr[(x >> 33) % n]->idReserva;
        }
        Coleccion c; c.arr = new Reserva*[n]; c.n = 0;
        t->getHash().recorrer(agregar, &c);  // arreglo para la busqueda secuencial

        long suma = 0;
        steady_clock::time_point a = steady_clock::now();
        for (int i = 0; i < REP; i++) if (t->buscar(ids[i])) suma++;
        steady_clock::time_point b = steady_clock::now();
        double usHash = duration_cast<nanoseconds>(b - a).count() / 1000.0 / REP;

        a = steady_clock::now();
        for (int i = 0; i < REP; i++)
            for (int k = 0; k < c.n; k++) if (c.arr[k]->idReserva == ids[i]) { suma++; break; }
        b = steady_clock::now();
        double usSec = duration_cast<nanoseconds>(b - a).count() / 1000.0 / REP;

        // insercion individual: 100 reservas nuevas
        long nsIns = 0;
        for (int i = 0; i < 100; i++) {
            Vuelo* v = t->getVuelo(i % t->getNumVuelos());
            Reserva* r;
            string id = "RES-03" + to_string(1000 + i);
            a = steady_clock::now();
            t->registrar(id, "70000000", "Prueba Insercion", v->codigo, "Economica", "Regular", 150, "Confirmada", r);
            b = steady_clock::now();
            nsIns += duration_cast<nanoseconds>(b - a).count();
        }
        double usIns = nsIns / 1000.0 / 100;

        Reserva* lista[50];
        a = steady_clock::now();
        for (int i = 0; i < 100; i++) suma += t->listarCronologico(lista, 50);
        b = steady_clock::now();
        double usOrd = duration_cast<nanoseconds>(b - a).count() / 1000.0 / 100;

        Reserva* muestra[10]; long visit = 0; int totalMes = 0;
        a = steady_clock::now();
        for (int i = 0; i < 100; i++) totalMes = t->consultarRango("2027-01-01", "2027-01-31", muestra, 10, visit);
        b = steady_clock::now();
        double usRango = duration_cast<nanoseconds>(b - a).count() / 1000.0 / 100;

        delete[] arr;
        arr = todas(*t, n);
        // actualizacion de estado: 10 reservas Pendiente -> Confirmada
        long nsAct = 0; int nAct = 0;
        for (int i = 0; i < n && nAct < 10; i++) {
            if (arr[i]->estado != "Pendiente") continue;
            string id = arr[i]->idReserva;
            a = steady_clock::now();
            CodigoOp co = t->actualizarEstado(id, "Confirmada");
            b = steady_clock::now();
            if (co == OP_OK) { nsAct += duration_cast<nanoseconds>(b - a).count(); nAct++; }
        }
        // cancelacion con promocion: 10 reservas con asiento en vuelos con espera
        long nsCan = 0; int nCan = 0; string cancelados[10];
        for (int i = 0; i < n && nCan < 10; i++) {
            Reserva* r = arr[i];
            Vuelo* v = t->buscarVuelo(r->codigoVuelo);
            if (!ocupaAsiento(r) || v->espera.tamano() < 2) continue;
            Reserva* prom;
            a = steady_clock::now();
            CodigoOp co = t->cancelar(r->idReserva, prom);
            b = steady_clock::now();
            if (co == OP_OK && prom) { nsCan += duration_cast<nanoseconds>(b - a).count(); cancelados[nCan++] = r->idReserva; }
        }
        // eliminacion fisica de esas 10
        long nsEli = 0; int nEli = 0;
        for (int i = 0; i < nCan; i++) {
            a = steady_clock::now();
            CodigoOp co = t->eliminarFisica(cancelados[i]);
            b = steady_clock::now();
            if (co == OP_OK) { nsEli += duration_cast<nanoseconds>(b - a).count(); nEli++; }
        }
        delete[] arr;

        cout << "  Busqueda por ID en hash       (1000 rep): " << fmt(usHash, 3) << " us\n"
             << "  Busqueda secuencial mismo ID  (1000 rep): " << fmt(usSec, 3) << " us"
             << "   (la hash es " << (int)(usSec / usHash + 0.5) << " veces mas rapida)\n"
             << "  Insercion individual          (100 rep):  " << fmt(usIns, 3) << " us\n"
             << "  Consulta ordenada, primeras 50 (100 rep): " << fmt(usOrd, 3) << " us\n"
             << "  Consulta por rango, un mes    (100 rep):  " << fmt(usRango, 3) << " us"
             << "   (" << totalMes << " reservas, " << visit << " nodos visitados)\n"
             << "  Actualizacion de estado       (" << nAct << " res):   " << fmt(nsAct / 1000.0 / (nAct ? nAct : 1), 3) << " us\n"
             << "  Cancelacion con promocion     (" << nCan << " res):   " << fmt(nsCan / 1000.0 / (nCan ? nCan : 1), 3) << " us\n"
             << "  Eliminacion fisica            (" << nEli << " res):   " << fmt(nsEli / 1000.0 / (nEli ? nEli : 1), 3) << " us\n"
             << "  (control " << suma << ")\n";
        check(t->verificarConsistencia(det), "consistencia al final de las mediciones");
        delete[] ids; delete[] c.arr;
        delete t;
    }

    titulo("RESUMEN");
    cout << (fallos == 0 ? "  Todas las verificaciones pasaron.\n"
                         : "  Verificaciones fallidas: " + to_string(fallos) + "\n");
    return fallos == 0 ? 0 : 1;
}
