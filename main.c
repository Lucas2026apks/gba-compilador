#include <gba.h>

int main(void) {
    // Inicializar interrupciones y sistema de video de la GBA
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Configurar el modo de video a Modo 0 (ideal para gráficos de bloques / tiles 2D)
    SetMode(MODE_0 | BG0_ON);

    // Bucle principal del juego
    while(1) {
        // Leer el estado actual de los botones de la consola
        scanKeys();
        u16 teclas = keysHeld();

        // Ejemplo de movimiento básico del personaje con la cruceta (D-Pad)
        if (tecles & KEY_UP) {
            // Lógica para mover hacia arriba
        }
        if (tecles & KEY_DOWN) {
            // Lógica para mover hacia abajo
        }
        if (tecles & KEY_LEFT) {
            // Lógica para mover a la izquierda
        }
        if (tecles & KEY_RIGHT) {
            // Lógica para mover a la derecha
        }

        // Sincronizar con el refresco de pantalla para evitar parpadeos
        VBlankIntrWait();
    }
}
