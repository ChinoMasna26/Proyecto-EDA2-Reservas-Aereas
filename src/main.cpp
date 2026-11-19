// EDA II - Proyecto reservas aereas
#include "Menu.h"

int main() {
    Sistema* sistema = new Sistema();   // con new porque es grande (tiene los 60 vuelos)
    Menu menu(*sistema);
    menu.ejecutar();
    delete sistema;   // aca se llaman los destructores
    return 0;
}
