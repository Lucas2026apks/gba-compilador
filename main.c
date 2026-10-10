#define REG_DISPCNT  *(volatile unsigned short*)0x4000000
#define VRAM         ((volatile unsigned short*)0x6000000)
#define VRAM_BACK    ((volatile unsigned short*)0x600A000)
#define PALETTE      ((volatile unsigned short*)0x5000000)
#define KEYINPUT     *(volatile unsigned short*)0x04000130
#define REG_VCOUNT   *(volatile unsigned short*)0x4000006

#define MODE4        4
#define BG2_ENABLE   (1 << 10)
#define BUFFER_BIT   (1 << 4)

#define COLOR(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

// Controles GBA
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
#define MAP_W 512
#define MAP_H 512

// Índices de la Paleta de Colores
#define PAL_VERDE_BASE   1
#define PAL_RIO          2
#define PAL_ARENA        3
#define PAL_CAMINO       4
#define PAL_ARADA        5
#define PAL_SEMBRADA     6
#define PAL_CRECIENDO    7
#define PAL_TRIGO        8
#define PAL_GASOLINERA   9
#define PAL_SILO        10
#define PAL_CAMIONETA   11
#define PAL_TRACTOR     12
#define PAL_PERSONAJE   13
#define PAL_ROSA_HUD    14
#define PAL_NEGRO       15
#define PAL_AMARILLO    16

// Estados
#define TIERRA_PASTO       0
#define TIERRA_ARADA       1
#define TIERRA_SEMBRADA    2
#define TIERRA_CRECIENDO   3
#define TIERRA_LISTA       4

#define CULTIVO_TRIGO      0
#define MAQ_ARADO          0

typedef struct { int x, y; int isDriver; } Personaje;
typedef struct { int x, y, w, h, speed, isOccupied, implemento, tolva, capTolva, gasolina, aiActive; } Vehiculo;
typedef struct { int x, y, w, h, estado, cultivo, timerCrecimiento, fertilizado; } Campo;
typedef struct { int x, y; } Camera;

unsigned short* currentBuffer = VRAM_BACK;

void initPalette(void) {
    PALETTE[PAL_VERDE_BASE] = COLOR(4, 10, 3);
    PALETTE[PAL_RIO]        = COLOR(5, 15, 25);
    PALETTE[PAL_ARENA]      = COLOR(18, 16, 10);
    PALETTE[PAL_CAMINO]     = COLOR(10, 10, 10);
    PALETTE[PAL_ARADA]      = COLOR(12, 7, 2);
    PALETTE[PAL_SEMBRADA]   = COLOR(15, 10, 3);
    PALETTE[PAL_CRECIENDO]  = COLOR(6, 18, 4);
    PALETTE[PAL_TRIGO]      = COLOR(31, 26, 0);
    PALETTE[PAL_GASOLINERA] = COLOR(25, 5, 5);
    PALETTE[PAL_SILO]       = COLOR(15, 15, 15);
    PALETTE[PAL_CAMIONETA]  = COLOR(2, 8, 26);
    PALETTE[PAL_TRACTOR]    = COLOR(26, 3, 3);
    PALETTE[PAL_PERSONAJE]  = COLOR(31, 0, 0);
    PALETTE[PAL_ROSA_HUD]   = COLOR(28, 5, 18);
    PALETTE[PAL_NEGRO]      = COLOR(0, 0, 0);
    PALETTE[PAL_AMARILLO]   = COLOR(31, 31, 0);
}

void flipBuffer(void) {
    if (REG_DISPCNT & BUFFER_BIT) {
        REG_DISPCNT &= ~BUFFER_BIT;
        currentBuffer = VRAM_BACK;
    } else {
        REG_DISPCNT |= BUFFER_BIT;
        currentBuffer = VRAM;
    }
}

void waitForVBlank(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
}

void drawPixel(int x, int y, unsigned char colorIdx) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    
    int index = (y * SCREEN_W + x) / 2;
    unsigned short current = currentBuffer[index];

    if (x & 1) {
        currentBuffer[index] = (current & 0x00FF) | (colorIdx << 8);
    } else {
        currentBuffer[index] = (current & 0xFF00) | colorIdx;
    }
}

void drawRect(int x, int y, int w, int h, unsigned char colorIdx) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            drawPixel(x + j, y + i, colorIdx);
        }
    }
}

void drawSantaCruzMap(Camera cam, Campo campos[], int numCampos) {
    int startX = cam.x;
    int endX = cam.x + SCREEN_W;
    int startY = cam.y;
    int endY = cam.y + SCREEN_H;

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            unsigned char col = PAL_VERDE_BASE;

            if (x < 70 + (y / 8)) col = PAL_RIO;
            else if (x < 85 + (y / 8)) col = PAL_ARENA;
            else {
                int dx = x - 256;
                int dy = y - 256;
                int distSq = (dx * dx + dy * dy);

                if ((distSq >= 40*40 && distSq <= 44*44) ||
                    (distSq >= 85*85 && distSq <= 90*90) ||
                    (distSq >= 135*135 && distSq <= 141*141) ||
                    (distSq >= 190*190 && distSq <= 197*197) ||
                    (x >= 254 && x <= 258) || (y >= 254 && y <= 258)) {
                    col = PAL_CAMINO;
                }
            }
            drawPixel(x - cam.x, y - cam.y, col);
        }
    }

    for (int i = 0; i < numCampos; i++) {
        unsigned char col = PAL_VERDE_BASE;
        switch (campos[i].estado) {
            case TIERRA_ARADA: col = PAL_ARADA; break;
            case TIERRA_SEMBRADA: col = PAL_SEMBRADA; break;
            case TIERRA_CRECIENDO: col = PAL_CRECIENDO; break;
            case TIERRA_LISTA: col = PAL_TRIGO; break;
        }
        drawRect(campos[i].x - cam.x, campos[i].y - cam.y, campos[i].w, campos[i].h, col);
    }

    drawRect(235 - cam.x, 120 - cam.y, 42, 32, PAL_GASOLINERA);
    drawRect(380 - cam.x, 240 - cam.y, 50, 40, PAL_SILO);
}

void drawMinimap(int playerX, int playerY) {
    int miniW = 48, miniH = 48, miniX = 6, miniY = 6;

    drawRect(miniX - 2, miniY - 2, miniW + 4, miniH + 4, PAL_NEGRO);
    drawRect(miniX, miniY, miniW, miniH, PAL_VERDE_BASE);
    drawRect(miniX + 2, miniY, 6, miniH, PAL_RIO);
    drawRect(miniX + 16, miniY + 16, 16, 16, PAL_CAMINO);

    int pMiniX = miniX + (playerX * miniW) / MAP_W;
    int pMiniY = miniY + (playerY * miniH) / MAP_H;
    drawRect(pMiniX - 1, pMiniY - 1, 3, 3, PAL_AMARILLO);
}

int main(void) {
    REG_DISPCNT = MODE4 | BG2_ENABLE;
    initPalette();

    int charActivo = 0;
    Camera cam = { .x = 180, .y = 180 };

    Personaje dueno = { .x = 250, .y = 250, .isDriver = 0 };
    Vehiculo camioneta = { .x = 265, .y = 250, .w = 16, .h = 10, .speed = 2, .isOccupied = 0, .gasolina = 100 };
    Vehiculo tractor = { .x = 310, .y = 300, .w = 14, .h = 12, .speed = 1, .isOccupied = 1, .implemento = MAQ_ARADO, .gasolina = 100 };

    Campo campos[2] = {
        { .x = 120, .y = 280, .w = 110, .h = 70, .estado = TIERRA_ARADA },
        { .x = 320, .y = 120, .w = 120, .h = 80, .estado = TIERRA_LISTA }
    };

    unsigned short prevKeys = 0;

    while (1) {
        unsigned short keys = ~KEYINPUT;

        if ((keys & KEY_L) && !(prevKeys & KEY_L)) charActivo = 0;
        if ((keys & KEY_R) && !(prevKeys & KEY_R)) charActivo = 1;

        if (charActivo == 0) {
            if (!dueno.isDriver) {
                if (keys & KEY_RIGHT) dueno.x++;
                if (keys & KEY_LEFT)  dueno.x--;
                if (keys & KEY_UP)    dueno.y--;
                if (keys & KEY_DOWN)  dueno.y++;
            } else {
                if (keys & KEY_RIGHT) { camioneta.x += camioneta.speed; dueno.x = camioneta.x; }
                if (keys & KEY_LEFT)  { camioneta.x -= camioneta.speed; dueno.x = camioneta.x; }
                if (keys & KEY_UP)    { camioneta.y -= camioneta.speed; dueno.y = camioneta.y; }
                if (keys & KEY_DOWN)  { camioneta.y += camioneta.speed; dueno.y = camioneta.y; }
            }
        } else {
            if (keys & KEY_RIGHT) tractor.x += tractor.speed;
            if (keys & KEY_LEFT)  tractor.x -= tractor.speed;
            if (keys & KEY_UP)    tractor.y -= tractor.speed;
            if (keys & KEY_DOWN)  tractor.y += tractor.speed;
        }

        int focusX = (charActivo == 0) ? dueno.x : tractor.x;
        int focusY = (charActivo == 0) ? dueno.y : tractor.y;

        cam.x = focusX - SCREEN_W / 2;
        cam.y = focusY - SCREEN_H / 2;

        if (cam.x < 0) cam.x = 0;
        if (cam.y < 0) cam.y = 0;
        if (cam.x > MAP_W - SCREEN_W) cam.x = MAP_W - SCREEN_W;
        if (cam.y > MAP_H - SCREEN_H) cam.y = MAP_H - SCREEN_H;

        // Renderizado Suave
        drawSantaCruzMap(cam, campos, 2);
        drawRect(camioneta.x - cam.x, camioneta.y - cam.y, camioneta.w, camioneta.h, PAL_CAMIONETA);
        drawRect(tractor.x - cam.x, tractor.y - cam.y, tractor.w, tractor.h, PAL_TRACTOR);

        if (!dueno.isDriver) {
            drawRect(dueno.x - cam.x, dueno.y - cam.y, 6, 6, PAL_PERSONAJE);
        }

        drawMinimap(focusX, focusY);

        waitForVBlank();
        flipBuffer(); // Intercambia la pantalla instantáneamente sin parpadeo

        prevKeys = keys;
    }

    return 0;
}
