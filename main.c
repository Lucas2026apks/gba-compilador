#define REG_DISPCNT  *(volatile unsigned short*)0x4000000
#define VRAM         (volatile unsigned short*)0x6000000
#define KEYINPUT     *(volatile unsigned short*)0x04000130

#define MODE3        3
#define BG2_ENABLE   (1 << 10)

#define COLOR(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

// Máscaras de botones GBA
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

// Estados de la Tierra (FS14)
#define TIERRA_PASTO       0
#define TIERRA_ARADA       1
#define TIERRA_SEMBRADA    2
#define TIERRA_CRECIENDO   3
#define TIERRA_LISTA       4

// Cultivos
#define CULTIVO_TRIGO      0
#define CULTIVO_MAIZ       1
#define CULTIVO_GIRASOL    2

// Maquinaria e Implementos
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
    int gasolina;    // Nivel de combustible (0-100)
    int aiActive;    // Piloto Automático
} Vehiculo;

typedef struct {
    int x, y, w, h;
    int estado;
    int cultivo;
    int timerCrecimiento;
    int fertilizado;
} Campo;

// --- DIBUJADO DE PANTALLA ---
void drawPixel(int x, int y, unsigned short color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        VRAM[y * SCREEN_W + x] = color;
    }
}

void drawRect(int x, int y, int w, int h, unsigned short color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            drawPixel(x + j, y + i, color);
        }
    }
}

// Mapa de Santa Cruz con Corral de Vacas y Gasolinera
void drawSantaCruzMap(Campo campos[], int numCampos, int lecheDisponible) {
    // Fondo Monte / Vegetación
    drawRect(0, 0, SCREEN_W, SCREEN_H, COLOR(4, 12, 3));

    // Red de Caminos
    drawRect(0, 75, SCREEN_W, 14, COLOR(20, 12, 5));
    drawRect(110, 0, 14, SCREEN_H, COLOR(20, 12, 5));

    // Silo de Acopio (Arriba a la Derecha)
    drawRect(190, 5, 45, 35, COLOR(12, 12, 12));
    drawRect(195, 10, 35, 25, COLOR(28, 22, 0));

    // Estación de Servicio / Gasolinera (Arriba a la Izquierda)
    drawRect(5, 5, 35, 30, COLOR(25, 5, 5)); // Edificio rojo
    drawRect(10, 20, 10, 10, COLOR(31, 31, 0)); // Surtidor

    // Corral de Ganado Vacuno (Abajo a la Izquierda)
    drawRect(5, 100, 50, 50, COLOR(16, 10, 4));  // Cerca/Tierra
    drawRect(15, 110, 6, 4, COLOR(31, 31, 31));  // Vaca 1 (Blanca/Negra)
    drawRect(17, 112, 2, 2, COLOR(0, 0, 0));
    drawRect(30, 125, 6, 4, COLOR(31, 31, 31));  // Vaca 2
    drawRect(32, 127, 2, 2, COLOR(0, 0, 0));

    // Dibujar Campos y sus Estados
    for (int i = 0; i < numCampos; i++) {
        unsigned short col = COLOR(4, 12, 3);
        switch (campos[i].estado) {
            case TIERRA_PASTO:     col = COLOR(4, 12, 3); break;
            case TIERRA_ARADA:     col = COLOR(12, 7, 2); break;
            case TIERRA_SEMBRADA:  col = COLOR(15, 10, 3); break;
            case TIERRA_CRECIENDO: col = COLOR(6, 18, 4); break;
            case TIERRA_LISTA:     
                if (campos[i].cultivo == CULTIVO_TRIGO)   col = COLOR(31, 26, 0); 
                if (campos[i].cultivo == CULTIVO_MAIZ)    col = COLOR(31, 18, 0); 
                if (campos[i].cultivo == CULTIVO_GIRASOL) col = COLOR(25, 25, 5); 
                break;
        }
        drawRect(campos[i].x, campos[i].y, campos[i].w, campos[i].h, col);
    }
}

int main(void) {
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    int dinero = 1000;
    int tipoSemilla = CULTIVO_TRIGO;
    int charActivo = 0; // 0 = Dueño, 1 = Trabajador
    int abonoCorral = 100;
    int timerGasolina = 0;

    Personaje dueno = { .x = 120, .y = 90, .isDriver = 0 };
    Personaje trabajador = { .x = 35, .y = 35, .isDriver = 1 };

    Vehiculo camioneta = { .x = 125, .y = 77, .w = 12, .h = 8, .speed = 2, .isOccupied = 0, .implemento = -1, .tolva = 0, .capTolva = 0, .gasolina = 100, .aiActive = 0 };
    Vehiculo tractor = { .x = 30, .y = 30, .w = 10, .h = 10, .speed = 1, .isOccupied = 1, .implemento = MAQ_ARADO, .tolva = 0, .capTolva = 100, .gasolina = 100, .aiActive = 0 };

    Campo campos[2] = {
        { .x = 10, .y = 40, .w = 90, .h = 30, .estado = TIERRA_ARADA, .cultivo = CULTIVO_TRIGO, .timerCrecimiento = 0, .fertilizado = 0 },
        { .x = 135, .y = 95, .w = 90, .h = 55, .estado = TIERRA_LISTA, .cultivo = CULTIVO_TRIGO, .timerCrecimiento = 0, .fertilizado = 0 }
    };

    unsigned short prevKeys = 0;

    while (1) {
        // VSync
        volatile unsigned short* scanline = (volatile unsigned short*)0x4000006;
        while (*scanline < 160);

        unsigned short keys = ~KEYINPUT;
        timerGasolina++;

        // --- CONSUMO DE GASOLINA ---
        if (timerGasolina > 180) { // Cada ~3 segundos consume gasolina en uso
            if (tractor.gasolina > 0 && (tractor.aiActive || charActivo == 1)) tractor.gasolina--;
            if (camioneta.gasolina > 0 && dueno.isDriver) camioneta.gasolina--;
            timerGasolina = 0;
        }

        // --- CICLO DE CRECIMIENTO DE CULTIVOS ---
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

        // --- CAMBIO DE PERSONAJE (L / R) ---
        if ((keys & KEY_L) && !(prevKeys & KEY_L)) charActivo = 0;
        if ((keys & KEY_R) && !(prevKeys & KEY_R)) charActivo = 1;

        // SELECT: Cambiar Tipo de Semilla
        if ((keys & KEY_SELECT) && !(prevKeys & KEY_SELECT)) {
            tipoSemilla = (tipoSemilla + 1) % 3;
        }

        // START: Activar/Desactivar Piloto Automático IA
        if ((keys & KEY_START) && !(prevKeys & KEY_START)) {
            if (tractor.gasolina > 0) tractor.aiActive = !tractor.aiActive;
        }

        // --- RECARGAR GASOLINA (EN GASOLINERA) ---
        if (keys & KEY_START) {
            // Recargar Camioneta
            if (camioneta.x <= 40 && camioneta.y <= 40 && dinero >= 5 && camioneta.gasolina < 100) {
                camioneta.gasolina += 2;
                dinero -= 1;
            }
            // Recargar Tractor
            if (tractor.x <= 40 && tractor.y <= 40 && dinero >= 5 && tractor.gasolina < 100) {
                tractor.gasolina += 2;
                dinero -= 1;
            }
        }

        // --- PILOTO AUTOMÁTICO IA ---
        if (tractor.aiActive && tractor.gasolina > 0) {
            tractor.x += 1;
            if (tractor.x > 95) tractor.x = 10;
        } else {
            tractor.aiActive = 0;
        }

        // --- CONTROL DEL DUEÑO (A PIE / VEHÍCULO) ---
        if (charActivo == 0) {
            if (!dueno.isDriver) {
                if (keys & KEY_RIGHT) dueno.x++;
                if (keys & KEY_LEFT)  dueno.x--;
                if (keys & KEY_UP)    dueno.y--;
                if (keys & KEY_DOWN)  dueno.y++;

                if ((keys & KEY_A) && !(prevKeys & KEY_A)) {
                    if (dueno.x >= camioneta.x - 5 && dueno.x <= camioneta.x + 15 &&
                        dueno.y >= camioneta.y - 5 && dueno.y <= camioneta.y + 15) {
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
                    dueno.x = camioneta.x + 15;
                }
            }
        }

        // --- CONTROL DEL TRABAJADOR / TRACTOR ---
        if (charActivo == 1 && !tractor.aiActive && tractor.gasolina > 0) {
            if (keys & KEY_RIGHT) { tractor.x += tractor.speed; trabajador.x = tractor.x; }
            if (keys & KEY_LEFT)  { tractor.x -= tractor.speed; trabajador.x = tractor.x; }
            if (keys & KEY_UP)    { tractor.y -= tractor.speed; trabajador.y = tractor.y; }
            if (keys & KEY_DOWN)  { tractor.y += tractor.speed; trabajador.y = tractor.y; }

            // B: Cambiar Implemento
            if ((keys & KEY_B) && !(prevKeys & KEY_B)) {
                tractor.implemento = (tractor.implemento + 1) % 4;
            }

            // Trabajo en Campo
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
                    } else if (tractor.implemento == MAQ_REMOLQUE && tractor.tolva > 0) {
                        campos[i].fertilizado = 1; // Abonar campo
                    }
                }
            }

            // Cargar Abono en el Corral de Vacas
            if (tractor.implemento == MAQ_REMOLQUE && tractor.x <= 60 && tractor.y >= 90) {
                if ((keys & KEY_A) && !(prevKeys & KEY_A) && abonoCorral >= 20) {
                    tractor.tolva = 100;
                    abonoCorral -= 20;
                }
            }

            // R + B: Vender Carga en Silo
            if ((keys & KEY_R) && (keys & KEY_B)) {
                if (tractor.x >= 190 && tractor.y <= 40 && tractor.tolva > 0) {
                    dinero += tractor.tolva * 3;
                    tractor.tolva = 0;
                }
            }
        }

        // --- RENDERIZADO GENERAL ---
        drawSantaCruzMap(campos, 2, abonoCorral);

        // Vehículos
        drawRect(camioneta.x, camioneta.y, camioneta.w, camioneta.h, COLOR(2, 10, 28)); 
        drawRect(tractor.x, tractor.y, tractor.w, tractor.h, COLOR(28, 3, 3));        

        // Barritas de Gasolina sobre los vehículos
        drawRect(tractor.x, tractor.y - 3, (tractor.gasolina * tractor.w) / 100, 2, COLOR(0, 31, 0));
        drawRect(camioneta.x, camioneta.y - 3, (camioneta.gasolina * camioneta.w) / 100, 2, COLOR(0, 31, 0));

        // Implemento Enganchado
        unsigned short colImp = COLOR(10, 10, 10);
        if (tractor.implemento == MAQ_ARADO)       colImp = COLOR(18, 9, 2);  
        if (tractor.implemento == MAQ_SEMBRADORA)   colImp = COLOR(2, 20, 2);  
        if (tractor.implemento == MAQ_COSECHADORA)  colImp = COLOR(31, 22, 0); 
        if (tractor.implemento == MAQ_REMOLQUE)     colImp = COLOR(15, 15, 25); 

        drawRect(tractor.x - 6, tractor.y + 2, 5, 6, colImp);

        // Dueño a Pie
        if (!dueno.isDriver) {
            drawRect(dueno.x, dueno.y, 4, 4, COLOR(31, 31, 0));
        }

        // HUD (Esquina Superior Izquierda)
        drawRect(4, 4, 6, 6, charActivo == 0 ? COLOR(31, 31, 0) : COLOR(31, 0, 0)); 
        drawRect(12, 4, 4, 4, tractor.aiActive ? COLOR(0, 31, 0) : COLOR(10, 10, 10)); 

        prevKeys = keys;

        for (volatile int i = 0; i < 1200; i++);
    }

    return 0;
}
