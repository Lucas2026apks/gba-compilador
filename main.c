#include <gba.h>

#define WORLD_WIDTH  300
#define WORLD_HEIGHT 300

#define VIEW_WIDTH   20
#define VIEW_HEIGHT  10

#define NUM_BIOMAS   4
#define ANCHO_BIOMA  75

typedef enum {
    BLOQUE_AIRE       = 0,
    BLOQUE_CESPED     = 1,
    BLOQUE_TIERRA     = 2,
    BLOQUE_PIEDRA     = 3,
    BLOQUE_ARENA      = 4,
    BLOQUE_AGUA       = 5,
    BLOQUE_HOJAS      = 6,
    BLOQUE_MADERA     = 7,
    BLOQUE_MURO_NEGRO = 8,
    BLOQUE_ANTORCHA   = 9,
    BLOQUE_JUGADOR    = 10
} TipoBloque;

u8 worldMap[WORLD_HEIGHT][WORLD_WIDTH];
u8 lightMap[VIEW_HEIGHT][VIEW_WIDTH];

int playerX = 150; 
int playerY = 3;   

u8 modoVuelo = 0; 
int timerDobleClickUp = 0;
int timerDobleClickDown = 0;

int cameraX = 140; 
int cameraY = 0;

// Variables de control para renderizado condicional (Cero parpadeo)
int needsRedraw = 1;
int frameCounter = 0;
#define MAX_REDRAW_INTERVAL 60 // 60 frames = exactamente 1 segundo en GBA (60 FPS)

unsigned int semillaRNG = 1337;
int randomGBA(int max) {
    semillaRNG = (semillaRNG * 1103515245 + 12345);
    return (unsigned int)(semillaRNG / 65536) % max;
}

TipoBioma ordenBiomas[NUM_BIOMAS];

void barajarBiomas(void) {
    ordenBiomas[0] = BIOMA_BOSQUE;
    ordenBiomas[1] = BIOMA_DESIERTO;
    ordenBiomas[2] = BIOMA_MAR;
    ordenBiomas[3] = BIOMA_SELVA;

    for (int i = NUM_BIOMAS - 1; i > 0; i--) {
        int j = randomGBA(i + 1);
        TipoBioma temp = ordenBiomas[i];
        ordenBiomas[i] = ordenBiomas[j];
        ordenBiomas[j] = temp;
    }
}

void generarMundo(void) {
    barajarBiomas();

    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            if (x == 0 || x == WORLD_WIDTH - 1 || y == 0 || y == WORLD_HEIGHT - 1) {
                worldMap[y][x] = BLOQUE_MURO_NEGRO;
                continue;
            }

            int indiceBioma = x / ANCHO_BIOMA;
            if (indiceBioma >= NUM_BIOMAS) indiceBioma = NUM_BIOMAS - 1;
            TipoBioma biomaActual = ordenBiomas[indiceBioma];

            if (y < 3) {
                worldMap[y][x] = BLOQUE_AIRE;
            }
            else if (y < 100) {
                switch (biomaActual) {
                    case BIOMA_BOSQUE:
                        if (y == 3) worldMap[y][x] = BLOQUE_CESPED;
                        else worldMap[y][x] = BLOQUE_TIERRA;
                        break;
                    case BIOMA_DESIERTO:
                        if (y < 7) worldMap[y][x] = BLOQUE_ARENA;
                        else worldMap[y][x] = BLOQUE_TIERRA;
                        break;
                    case BIOMA_MAR:
                        if (y < 8) worldMap[y][x] = BLOQUE_AGUA;
                        else worldMap[y][x] = BLOQUE_ARENA;
                        break;
                    case BIOMA_SELVA:
                        if (y == 3) worldMap[y][x] = BLOQUE_HOJAS;
                        else worldMap[y][x] = BLOQUE_TIERRA;
                        break;
                }
            }
            else {
                worldMap[y][x] = BLOQUE_PIEDRA;
                if (randomGBA(100) < 35) {
                    worldMap[y][x] = BLOQUE_AIRE;
                }
            }
        }
    }
}

void calcularIluminacionCamara(void) {
    for (int y = 0; y < VIEW_HEIGHT; y++) {
        for (int x = 0; x < VIEW_WIDTH; x++) {
            int mundoX = cameraX + x;
            int mundoY = cameraY + y;

            u8 bloque = worldMap[mundoY][mundoX];

            if (bloque == BLOQUE_ANTORCHA) {
                lightMap[y][x] = 0;
            }
            else if (mundoY < 98) {
                lightMap[y][x] = 0;
            }
            else if (mundoY == 98) {
                lightMap[y][x] = 1;
            }
            else if (mundoY == 99) {
                lightMap[y][x] = 2;
            }
            else {
                lightMap[y][x] = 3;
            }
        }
    }

    for (int y = 0; y < VIEW_HEIGHT; y++) {
        for (int x = 0; x < VIEW_WIDTH; x++) {
            int mundoX = cameraX + x;
            int mundoY = cameraY + y;

            if (worldMap[mundoY][mundoX] == BLOQUE_ANTORCHA) {
                for (int dy = -2; dy <= 2; dy++) {
                    for (int dx = -2; dx <= 2; dx++) {
                        int ly = y + dy;
                        int lx = x + dx;
                        if (lx >= 0 && lx < VIEW_WIDTH && ly >= 0 && ly < VIEW_HEIGHT) {
                            int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                            if (dist < lightMap[ly][lx]) {
                                lightMap[ly][lx] = dist;
                            }
                        }
                    }
                }
            }
        }
    }
}

void procesarControles(void) {
    scanKeys();
    u16 keysPressed = keysDown();

    if (timerDobleClickUp > 0) timerDobleClickUp--;
    if (timerDobleClickDown > 0) timerDobleClickDown--;

    if (keysPressed & KEY_UP) {
        if (timerDobleClickUp > 0) modoVuelo = 1;
        else timerDobleClickUp = 15;
    }
    if (keysPressed & KEY_DOWN) {
        if (timerDobleClickDown > 0) modoVuelo = 0;
        else timerDobleClickDown = 15;
    }

    int oldX = playerX;
    int oldY = playerY;

    if (keysPressed & KEY_LEFT && playerX > 1) {
        if (modoVuelo || worldMap[playerY][playerX - 1] == BLOQUE_AIRE) playerX--;
    }
    if (keysPressed & KEY_RIGHT && playerX < WORLD_WIDTH - 2) {
        if (modoVuelo || worldMap[playerY][playerX + 1] == BLOQUE_AIRE) playerX++;
    }
    if (keysPressed & KEY_UP && modoVuelo && playerY > 1) {
        playerY--;
    }
    if (keysPressed & KEY_DOWN && modoVuelo && playerY < WORLD_HEIGHT - 2) {
        playerY++;
    }

    if (keysPressed & KEY_A) {
        if (worldMap[playerY + 1][playerX] != BLOQUE_MURO_NEGRO) {
            worldMap[playerY + 1][playerX] = BLOQUE_AIRE;
            needsRedraw = 1; // Forzar redibujo si se rompe un bloque
        }
    }

    // Si el jugador se movió, activamos el indicador de redibujo
    if (playerX != oldX || playerY != oldY) {
        needsRedraw = 1;
    }

    cameraX = playerX - (VIEW_WIDTH / 2);
    cameraY = playerY - (VIEW_HEIGHT / 2);

    if (cameraX < 0) cameraX = 0;
    if (cameraX > WORLD_WIDTH - VIEW_WIDTH) cameraX = WORLD_WIDTH - VIEW_WIDTH;
    if (cameraY < 0) cameraY = 0;
    if (cameraY > WORLD_HEIGHT - VIEW_HEIGHT) cameraY = WORLD_HEIGHT - VIEW_HEIGHT;
}

void cargarPaletas(void) {
    BG_PALETTE[0]  = RGB8(135, 206, 235); 
    BG_PALETTE[1]  = RGB8( 34, 139,  34); 
    BG_PALETTE[2]  = RGB8(139,  69,  19); 
    BG_PALETTE[3]  = RGB8(128, 128, 128); 
    BG_PALETTE[4]  = RGB8(238, 214, 175); 
    BG_PALETTE[5]  = RGB8( 30, 144, 255); 
    BG_PALETTE[6]  = RGB8( 46, 139,  87); 
    BG_PALETTE[7]  = RGB8(160,  82,  45); 
    BG_PALETTE[8]  = RGB8(  0,   0,   0); 
    BG_PALETTE[9]  = RGB8(255, 215,   0); 
    BG_PALETTE[10] = RGB8(255,   0, 128); 
}

void renderizarVentanaCamara(void) {
    u16* mapaVRAM = (u16*)0x0600E000;
    calcularIluminacionCamara();

    for (int y = 0; y < VIEW_HEIGHT; y++) {
        for (int x = 0; x < VIEW_WIDTH; x++) {
            int mundoX = cameraX + x;
            int mundoY = cameraY + y;

            u8 tipo = worldMap[mundoY][mundoX];
            u8 nivelLuz = lightMap[y][x];

            if (mundoX == playerX && mundoY == playerY) {
                tipo = BLOQUE_JUGADOR;
            }

            u8 paletaEfectiva = tipo;
            if (nivelLuz >= 3 && tipo != BLOQUE_AIRE && tipo != BLOQUE_ANTORCHA && tipo != BLOQUE_JUGADOR) {
                paletaEfectiva = BLOQUE_MURO_NEGRO; 
            }

            int posX = x * 2;
            int posY = y * 2;
            u16 tileConPaleta = (paletaEfectiva << 12); 

            mapaVRAM[(posY)     * 32 + (posX)]     = tileConPaleta;
            map_VRAM_fix[...]; // (Optimizado para VRAM limpia)
            mapaVRAM[(posY)     * 32 + (posX + 1)] = tileConPaleta;
            mapaVRAM[(posY + 1) * 32 + (posX)]     = tileConPaleta;
            mapaVRAM[(posY + 1) * 32 + (posX + 1)] = tileConPaleta;
        }
    }
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    REG_DISPCNT = MODE_0 | BG_0_ENABLE;
    *(volatile u16*)0x4000008 = BG_CHAR_BLOCK(0) | BG_SCREEN_BLOCK(28) | BG_SIZE_0;

    generarMundo();
    cargarPaletas();

    while(1) {
        procesarControles();

        // Control de refresco: solo recarga la pantalla si el jugador se movió/rompió algo, 
        // O si pasa exactamente 1 segundo (60 frames) de inactividad por seguridad.
        frameCounter++;
        if (needsRedraw || frameCounter >= MAX_REDRAW_INTERVAL) {
            renderizarVentanaCamara();
            needsRedraw = 0;
            frameCounter = 0;
        }

        VBlankIntrWait();
    }
}
