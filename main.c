#define REG_DISPCNT  *(volatile unsigned short*)0x4000000
#define VRAM         ((volatile unsigned short*)0x6000000)
#define KEYINPUT     *(volatile unsigned short*)0x04000130
#define REG_VCOUNT   *(volatile unsigned short*)0x4000006

#define MODE3        3
#define BG2_ENABLE   (1 << 10)

#define COLOR(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

// Botones de la GBA
#define KEY_A        (1 << 0)
#define KEY_B        (1 << 1)
#define KEY_SELECT   (1 << 2)
#define KEY_START    (1 << 3)
#define KEY_RIGHT    (1 << 4)
#define KEY_LEFT     (1 << 5)
#define KEY_UP       (1 << 6)
#define KEY_DOWN     (1 << 7)
#define KEY_R        (1 << 8)
#define KEY_L        (1 << 9)

#define SCREEN_W 240
#define SCREEN_H 160

// Dimensiones del Mapa Completo de Santa Cruz
#define MAP_W 512
#define MAP_H 512

// Estados de la Tierra y Cultivos
#define TIERRA_PASTO       0
#define TIERRA_ARADA       1
#define TIERRA_SEMBRADA    2
#define TIERRA_CRECIENDO   3
#define TIERRA_LISTA       4

#define CULTIVO_TRIGO      0
#define CULTIVO_MAIZ       1
#define CULTIVO_GIRASOL    2

#define MAQ_ARADO          0
#define MAQ_SEMBRADORA     1
#define MAQ_COSECHADORA    2
#define MAQ_REMOLQUE       3

typedef struct {
    int x, y;
    int isDriver;
} Personaje;

typedef struct {
    int x, y;
    int w, h;
    int speed;
    int isOccupied;
    int implemento;
    int tolva;
    int capTolva;
    int gasolina;
    int aiActive;
} Vehiculo;

typedef struct {
    int x, y, w, h;
    int estado;
    int cultivo;
    int timerCrecimiento;
    int fertilizado;
} Campo;

typedef struct {
    int x, y;
} Camera;

// Espera VBlank para eliminar parpadeos por completo
void waitForVBlank(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

// Dibujar píxel considerando la posición de la Cámara
void drawPixelCam(int worldX, int worldY, unsigned short color, Camera cam) {
    int screenX = worldX - cam.x;
    int screenY = worldY - cam.y;

    if (screenX >= 0 && screenX < SCREEN_W && screenY >= 0 && screenY < SCREEN_H) {
        VRAM[screenY * SCREEN_W + screenX] = color;
    }
}

// Dibujar rectángulo con Cámara
void drawRectCam(int worldX, int worldY, int w, int h, unsigned short color, Camera cam) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            drawPixelCam(worldX + j, worldY + i, color, cam);
        }
    }
}

// Dibujar píxel directo en pantalla (HUD / Minimapa)
void drawPixelScreen(int x, int y, unsigned short color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        VRAM[y * SCREEN_W + x] = color;
    }
}

// Dibujar rectángulo directo en pantalla (HUD / Minimapa)
void drawRectScreen(int x, int y, int w, int h, unsigned short color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            drawPixelScreen(x + j, y + i, color);
        }
    }
}

// Dibujo del mapa de Santa Cruz (Anillos, Radiales y Río Piraí)
void drawSantaCruzMap(Camera cam, Campo campos[], int numCampos) {
    int startX = cam.x;
    int endX = cam.x + SCREEN_W;
    int startY = cam.y;
    int endY = cam.y + SCREEN_H;

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            unsigned short color = COLOR(4, 10, 3); // Verde base

            // Río Piraí
            if (x < 70 + (y / 8)) {
                color = COLOR(5, 15, 25);
            }
            // Ribera / Playón
            else if (x < 85 + (y / 8)) {
                color = COLOR(18, 16, 10);
            }
            else {
                int dx = x - 256;
                int dy = y - 256;
                int distSq = (dx * dx + dy * dy);

                // Anillos
                if (distSq >= 40*40 && distSq <= 44*44) color = COLOR(12, 12, 12);
                else if (distSq >= 85*85 && distSq <= 90*90) color = COLOR(12, 12, 12);
                else if (distSq >= 135*135 && distSq <= 141*141) color = COLOR(12, 12, 12);
                else if (distSq >= 190*190 && distSq <= 197*197) color = COLOR(12, 12, 12);

                // Radiales
                else if ((x >= 254 && x <= 258) || (y >= 254 && y <= 258)) color = COLOR(12, 12, 12);
                else if ((x - y >= -4 && x - y <= 4) || (x + y >= 508 && x + y <= 516)) color = COLOR(10, 10, 10);
            }

            drawPixelCam(x, y, color, cam);
        }
    }

    // Dibujar Campos
    for (int i = 0; i < numCampos; i++) {
        unsigned short col = COLOR(4, 10, 3);
        switch (campos[i].estado) {
            case TIERRA_PASTO:     col = COLOR(5, 12, 4); break;
            case TIERRA_ARADA:     col = COLOR(12, 7, 2); break;
            case TIERRA_SEMBRADA:  col = COLOR(15, 10, 3); break;
            case TIERRA_CRECIENDO: col = COLOR(6, 18, 4); break;
            case TIERRA_LISTA:     
                if (campos[i].cultivo == CULTIVO_TRIGO)   col = COLOR(31, 26, 0); 
                if (campos[i].cultivo == CULTIVO_MAIZ)    col = COLOR(31, 18, 0); 
                if (campos[i].cultivo == CULTIVO_GIRASOL) col = COLOR(25, 25, 5); 
                break;
        }
        drawRectCam(campos[i].x, campos[i].y, campos[i].w, campos[i].h, col, cam);
    }

    // Gasolinera y Silos
    drawRectCam(235, 120, 42, 32, COLOR(25, 5, 5), cam);
    drawRectCam(380, 240, 50, 40, COLOR(15, 15, 15), cam);
}

// Minimapa en la Esquina Superior Izquierda
void drawMinimap(int playerX, int playerY) {
    int miniW = 48;
    int miniH = 48;
    int miniX = 6;
    int miniY = 6;

    drawRectScreen(miniX - 2, miniY - 2, miniW + 4, miniH + 4, COLOR(0, 0, 0));
    drawRectScreen(miniX, miniY, miniW, miniH, COLOR(3, 8, 2));

    drawRectScreen(miniX + 2, miniY, 6, miniH, COLOR(5, 15, 25)); // Río
    drawRectScreen(miniX + 16, miniY + 16, 16, 16, COLOR(12, 12, 12)); // Anillos
    drawRectScreen(miniX + 8, miniY + 8, 32, 32, COLOR(8, 8, 8));

    int pMiniX = miniX + (playerX * miniW) / MAP_W;
    int pMiniY = miniY + (playerY * miniH) / MAP_H;

    drawRectScreen(pMiniX - 1, pMiniY - 1, 3, 3, COLOR(31, 31, 0));
}

int main(void) {
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    int dinero = 1000;
    int tipoSemilla = CULTIVO_TRIGO;
    int charActivo = 0; 
    int timerGasolina = 0;

    Camera cam = { .x = 180, .y = 180 };

    Personaje dueno = { .x = 250, .y = 250, .isDriver = 0 };

    Vehiculo camioneta = { .x = 265, .y = 250, .w = 16, .h = 10, .speed = 2, .isOccupied = 0, .implemento = -1, .tolva = 0, .capTolva = 0, .gasolina = 100, .aiActive = 0 };
    Vehiculo tractor = { .x = 310, .y = 300, .w = 14, .h = 12, .speed = 1, .isOccupied = 1, .implemento = MAQ_ARADO, .tolva = 0, .capTolva = 100, .gasolina = 100, .aiActive = 0 };

    Campo campos[2] = {
        { .x = 120, .y = 280, .w = 110, .h = 70, .estado = TIERRA_ARADA, .cultivo = CULTIVO_TRIGO, .timerCrecimiento = 0, .fertilizado = 0 },
        { .x = 320, .y = 120, .w = 120, .h = 80, .estado = TIERRA_LISTA, .cultivo = CULTIVO_TRIGO, .timerCrecimiento = 0, .fertilizado = 0 }
    };

    unsigned short prevKeys = 0;

    while (1) {
        unsigned short keys = ~KEYINPUT;
        timerGasolina++;

        // Consumo de Gasolina
        if (timerGasolina > 180) {
            if (tractor.gasolina > 0 && (tractor.aiActive || charActivo == 1)) tractor.gasolina--;
            if (camioneta.gasolina > 0 && dueno.isDriver) camioneta.gasolina--;
            timerGasolina = 0;
        }

        // Crecimiento de Cultivos
        for (int i = 0; i < 2; i++) {
            int multFertil = campos[i].fertilizado ? 2 : 1;
            if (campos[i].estado == TIERRA_SEMBRADA) {
                campos[i].timerCrecimiento += multFertil;
                if (campos[i].timerCrecimiento > 300) campos[i].estado = TIERRA_CRECIENDO;
            } else if (campos[i].estado == TIERRA_CRECIENDO) {
                campos[i].timerCrecimiento += multFertil;
                if (campos[i].timerCrecimiento > 600) {
                    campos[i].estado = TIERRA_LISTA;
                    campos[i].timerCrecimiento = 0;
                }
            }
        }

        // Cambio de Personaje
        if ((keys & KEY_L) && !(prevKeys & KEY_L)) charActivo = 0;
        if ((keys & KEY_R) && !(prevKeys & KEY_R)) charActivo = 1;

        if ((keys & KEY_SELECT) && !(prevKeys & KEY_SELECT)) {
            tipoSemilla = (tipoSemilla + 1) % 3;
        }

        if ((keys & KEY_START) && !(prevKeys & KEY_START)) {
            if (tractor.gasolina > 0) tractor.aiActive = !tractor.aiActive;
        }

        // Movimiento Dueño / Camioneta
        if (charActivo == 0) {
            if (!dueno.isDriver) {
                if (keys & KEY_RIGHT) dueno.x++;
                if (keys & KEY_LEFT)  dueno.x--;
                if (keys & KEY_UP)    dueno.y--;
                if (keys & KEY_DOWN)  dueno.y++;

                if ((keys & KEY_A) && !(prevKeys & KEY_A)) {
                    if (dueno.x >= camioneta.x - 8 && dueno.x <= camioneta.x + 20 &&
                        dueno.y >= camioneta.y - 8 && dueno.y <= camioneta.y + 20) {
                        dueno.isDriver = 1;
                        camioneta.isOccupied = 1;
                    }
                }
            } else if (camioneta.gasolina > 0) {
                if (keys & KEY_RIGHT) { camioneta.x += camioneta.speed; dueno.x = camioneta.x; }
                if (keys & KEY_LEFT)  { camioneta.x -= camioneta.speed; dueno.x = camioneta.x; }
                if (keys & KEY_UP)    { camioneta.y -= camioneta.speed; dueno.y = camioneta.y; }
                if (keys & KEY_DOWN)  { camioneta.y += camioneta.speed; dueno.y = camioneta.y; }

                if ((keys & KEY_A) && !(prevKeys & KEY_A)) {
                    dueno.isDriver = 0;
                    camioneta.isOccupied = 0;
                    dueno.x = camioneta.x + 20;
                }
            }
        }

        // Movimiento Tractor
        if (charActivo == 1 && !tractor.aiActive && tractor.gasolina > 0) {
            if (keys & KEY_RIGHT) { tractor.x += tractor.speed; }
            if (keys & KEY_LEFT)  { tractor.x -= tractor.speed; }
            if (keys & KEY_UP)    { tractor.y -= tractor.speed; }
            if (keys & KEY_DOWN)  { tractor.y += tractor.speed; }

            if ((keys & KEY_B) && !(prevKeys & KEY_B)) {
                tractor.implemento = (tractor.implemento + 1) % 4;
            }

            for (int i = 0; i < 2; i++) {
                if (tractor.x >= campos[i].x && tractor.x <= campos[i].x + campos[i].w &&
                    tractor.y >= campos[i].y && tractor.y <= campos[i].y + campos[i].h) {
                    
                    if (tractor.implemento == MAQ_ARADO && campos[i].estado == TIERRA_PASTO) {
                        campos[i].estado = TIERRA_ARADA;
                    } else if (tractor.implemento == MAQ_SEMBRADORA && campos[i].estado == TIERRA_ARADA) {
                        campos[i].estado = TIERRA_SEMBRADA;
                        campos[i].cultivo = tipoSemilla;
                        campos[i].timerCrecimiento = 0;
                    } else if (tractor.implemento == MAQ_COSECHADORA && campos[i].estado == TIERRA_LISTA) {
                        campos[i].estado = TIERRA_ARADA;
                        if (tractor.tolva < tractor.capTolva) tractor.tolva += 25;
                    }
                }
            }
        }

        // Seguir con Cámara
        int focusX = (charActivo == 0) ? dueno.x : tractor.x;
        int focusY = (charActivo == 0) ? dueno.y : tractor.y;

        cam.x = focusX - SCREEN_W / 2;
        cam.y = focusY - SCREEN_H / 2;

        if (cam.x < 0) cam.x = 0;
        if (cam.y < 0) cam.y = 0;
        if (cam.x > MAP_W - SCREEN_W) cam.x = MAP_W - SCREEN_W;
        if (cam.y > MAP_H - SCREEN_H) cam.y = MAP_H - SCREEN_H;

        // Esperar pantalla lista para dibujar
        waitForVBlank();

        // Mapa
        drawSantaCruzMap(cam, campos, 2);

        // Vehículos
        drawRectCam(camioneta.x, camioneta.y, camioneta.w, camioneta.h, COLOR(2, 8, 26), cam);
        drawRectCam(camioneta.x + 3, camioneta.y + 2, 6, 6, COLOR(20, 25, 31), cam);

        drawRectCam(tractor.x, tractor.y, tractor.w, tractor.h, COLOR(26, 3, 3), cam);
        drawRectCam(tractor.x + 3, tractor.y + 3, 5, 5, COLOR(31, 31, 31), cam);
        drawRectCam(tractor.x - 2, tractor.y, 2, 4, COLOR(0, 0, 0), cam); // Rueda Corregida
        drawRectCam(tractor.x - 2, tractor.y + 8, 2, 4, COLOR(0, 0, 0), cam); // Rueda Corregida

        // Implemento
        unsigned short colImp = COLOR(10, 10, 10);
        if (tractor.implemento == MAQ_ARADO)       colImp = COLOR(18, 9, 2);  
        if (tractor.implemento == MAQ_SEMBRADORA)   colImp = COLOR(2, 20, 2);  
        if (tractor.implemento == MAQ_COSECHADORA)  colImp = COLOR(31, 22, 0); 
        if (tractor.implemento == MAQ_REMOLQUE)     colImp = COLOR(15, 15, 25); 

        drawRectCam(tractor.x - 8, tractor.y + 2, 7, 8, colImp, cam);

        // Personaje
        if (!dueno.isDriver) {
            drawRectCam(dueno.x, dueno.y, 6, 6, COLOR(31, 24, 18), cam);
            drawRectCam(dueno.x + 1, dueno.y + 2, 4, 4, COLOR(31, 0, 0), cam);
        }

        // Minimapa
        drawMinimap(focusX, focusY);

        (void)dinero; // Evita warning de variable no usada
        prevKeys = keys;
    }

    return 0;
}
