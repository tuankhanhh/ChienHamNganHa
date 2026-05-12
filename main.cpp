#include <graphics.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <mmsystem.h>


#define MAX_COMPANIONS 5
#define MAX_BULLETS 50
#define MAX_ENEMIES 30
#define MAX_PARTICLES 100
#define MAX_STARS 100
#define MAX_POWERUPS 10
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 10
#define BULLET_SPEED 17
#define SHOOT_DELAY 150
#define ULTIMATE_COOLDOWN 5
#define COMPANION_SHOOT_DELAY 2
#define RIGHT_COMPANION_SHOOT_DELAY 1
#define LEFT_COMPANION_SHOOT_DELAY 0.5
#define PI 3.14159265358979323846

// C?u trúc d? li?u
typedef struct {
    float x, y;
    float dx, dy;
    float targetDx, targetDy;
    int radius;
    float lastAngle;
    int lives;
    
    float speedBoostTimer;       // Type 2: Speed
    float fireRateBoostTimer;    // Type 1: Fire Rate
    float shieldTimer;           // Type 4: Shield
    float damageBoostTimer;      // Type 5: Damage Boost
    float bulletSizeTimer;       // Type 6: Bullet Size
    float ultimateTimer;         // Type 7: Ultimate Charge
    float companionBoostTimer;   // Type 8: Companion
    
    // Bi?n tr?ng thái d?n và vu khí
    bool doubleShot;             
    int currentFruitType;        // 0: Ð?n thu?ng, 1: Chu?i, 5: Táo
} Player;

typedef struct {
    float x, y;
    float lastAngle;
    float shootTimer;
    int radius;
    bool active;
} Companion;

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
    bool highDamage;
} Bullet;

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
    int radius;
    int type;
    int health;
    float zigzagTimer;
    float dashTimer;
    float shootTimer;
    float specialTimer;
} Enemy;

typedef struct {
    float x, y;
    float dy;
    bool active;
    int type;
} PowerUp;

typedef struct {
    float x, y;
    float dx, dy;
    int life;
    bool active;
} Particle;

typedef struct {
    float x, y;
    float speed;
    int radius;
} Star;

// Bi?n toàn c?c
Player player;
Companion companions[MAX_COMPANIONS];
Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
PowerUp powerUps[MAX_POWERUPS];
Particle particles[MAX_PARTICLES];
Star stars[MAX_STARS];
int score = 0;
bool gameOver = false;
bool gameWon = false;
clock_t lastShotTime = 0;
int currentShootDelay = SHOOT_DELAY;
float currentPlayerSpeed = PLAYER_SPEED;
float currentBulletSize = 5;
int difficultyLevel = 0;
bool bossActive = false;
int postBossDifficulty = 0;

// Function prototypes
void initGame();
void drawPlayer();
void drawCompanions();
void drawBullets();
void drawEnemies();
void drawPowerUps();
void drawParticles();
void drawBackground();
void updateStars();
void updatePlayer();
void shootBullet();
void shootCompanionBullet(int index);
void updateCompanions();
void updateBullets();
void spawnEnemy();
void spawnBoss();
void updateEnemies();
void spawnPowerUp(float x, float y);
void updatePowerUps();
void createExplosion(float x, float y);
void updateParticles();
void checkCollisions();
void drawUI();
void triggerUltimate();

// ================= CÁC THU?T TOÁN Ð? H?A CO B?N =================

// 1. ÁP D?NG: Thu?t toán v? du?ng th?ng Bresenham
void bresenhamLine(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        putpixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// 2. ÁP D?NG: Thu?t toán v? du?ng tròn Midpoint
void midpointCircle(int xc, int yc, int r, int color) {
    int x = 0, y = r;
    int p = 1 - r;

    while (x <= y) {
        putpixel(xc + x, yc + y, color);
        putpixel(xc - x, yc + y, color);
        putpixel(xc + x, yc - y, color);
        putpixel(xc - x, yc - y, color);
        putpixel(xc + y, yc + x, color);
        putpixel(xc - y, yc + x, color);
        putpixel(xc + y, yc - x, color);
        putpixel(xc - y, yc - x, color);

        if (p < 0) {
            p += 2 * x + 3;
        } else {
            p += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

// 3. ÁP D?NG: Thu?t toán tô màu d? quy Boundary Fill (4 d?nh)
// Luu ý: Ð? quy sâu có th? gây tràn b? nh? stack (Stack Overflow), 
// nên thu?t toán này ch? áp d?ng cho các vùng di?n tích nh?.
void recursiveBoundaryFill(int x, int y, int fill_color, int boundary_color) {
    // Ch?n d? quy n?u t?a d? l?t ra ngoài gi?i h?n c?a s? d? h?a
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    int current_color = getpixel(x, y);
    if (current_color != boundary_color && current_color != fill_color) {
        putpixel(x, y, fill_color);
        recursiveBoundaryFill(x + 1, y, fill_color, boundary_color);
        recursiveBoundaryFill(x - 1, y, fill_color, boundary_color);
        recursiveBoundaryFill(x, y + 1, fill_color, boundary_color);
        recursiveBoundaryFill(x, y - 1, fill_color, boundary_color);
    }
}
// 4. ÁP D?NG FRACTAL: Thu?t toán d? quy v? du?ng cong Koch
void drawKochLine(float x1, float y1, float x2, float y2, int iter, int color) {
    if (iter == 0) {
        // T?n d?ng luôn thu?t toán Bresenham dã vi?t ? bài tru?c d? v? do?n th?ng
        bresenhamLine((int)x1, (int)y1, (int)x2, (int)y2, color);
    } else {
        float dx = (x2 - x1) / 3.0f;
        float dy = (y2 - y1) / 3.0f;

        float p1x = x1 + dx;
        float p1y = y1 + dy;

        float p2x = x1 + 2 * dx;
        float p2y = y1 + 2 * dy;

        // Tính t?a d? di?m chóp c?a tam giác d?u (quay m?t góc 60 d? ~ PI/3)
        // Do h? t?a d? Y c?a màn hình hu?ng xu?ng, ta dùng d?u tr? cho tr?c Y d? d?nh nhô ra ngoài
        float px = p1x + dx * cos(PI / 3) + dy * sin(PI / 3);
        float py = p1y - dx * sin(PI / 3) + dy * cos(PI / 3);

        // Ð? quy 4 do?n c?a du?ng cong Koch
        drawKochLine(x1, y1, p1x, p1y, iter - 1, color);
        drawKochLine(p1x, p1y, px, py, iter - 1, color);
        drawKochLine(px, py, p2x, p2y, iter - 1, color);
        drawKochLine(p2x, p2y, x2, y2, iter - 1, color);
    }
}

// Hàm bao b?c: Dùng 3 du?ng cong Koch ghép l?i thành hình Bông Tuy?t (áp d?ng cho Enemy)
void drawKochSnowflake(int x, int y, int radius, int iter, int color) {
    // 3 d?nh c?a m?t tam giác d?u bao quanh tâm (x, y)
    float p1x = x,                p1y = y - radius;
    float p2x = x + radius * 0.866f, p2y = y + radius * 0.5f; // 0.866 ~ cos(30 d?)
    float p3x = x - radius * 0.866f, p3y = y + radius * 0.5f;

    // V? 3 c?nh b?ng du?ng cong Koch (chú ý th? t? d?nh d? bông tuy?t nhô ra ngoài)
    drawKochLine(p1x, p1y, p2x, p2y, iter, color);
    drawKochLine(p2x, p2y, p3x, p3y, iter, color);
    drawKochLine(p3x, p3y, p1x, p1y, iter, color);
}
// ================================================================
// Kh?i t?o game
void initGame() {
    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT - 35;
    player.dx = 0;
    player.dy = 0;
    player.targetDx = 0;
    player.targetDy = 0;
    player.radius = 15;
    player.lastAngle = -PI / 2; // Hu?ng lên m?c d?nh
    player.lives = 3;
    player.speedBoostTimer = 0;
    player.fireRateBoostTimer = 0;
    player.ultimateTimer = 0;
    player.doubleShot = false;
    player.shieldTimer = 0;
    player.damageBoostTimer = 0;
    player.bulletSizeTimer = 0;
    player.companionBoostTimer = 0;
    player.currentFruitType = 0; // Tr?ng thái d?n thu?ng ban d?u

    // ===== KH?I T?O MÁY BAY H? TR? =====
    // T?t toàn b? máy bay lúc m?i vào game, chúng ch? b?t lên khi an power-up
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        companions[i].active = false;
        companions[i].radius = 10;
        companions[i].lastAngle = -PI / 2;
        companions[i].shootTimer = 0;
    }

    // ===== KH?I T?O CÁC M?NG KHÁC =====
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
        bullets[i].highDamage = false;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_POWERUPS; i++) powerUps[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = rand() % SCREEN_WIDTH;
        stars[i].y = rand() % SCREEN_HEIGHT;
        stars[i].radius = rand() % 2 + 1;
        stars[i].speed = (rand() % 5 + 1) * 0.1;
    }

    // ===== KH?I T?O CH? S? GAME =====
    score = 0;
    gameOver = false;
    gameWon = false;
    lastShotTime = 0;
    currentShootDelay = SHOOT_DELAY;
    currentPlayerSpeed = PLAYER_SPEED;
    currentBulletSize = 5;
    difficultyLevel = 0;
    bossActive = false;
    postBossDifficulty = 0;

    // ===== NH?C N?N =====
    // Ðóng nh?c cu n?u có (dùng cho tru?ng h?p nh?n R d? choi l?i)
    mciSendString("close bgm", NULL, 0, NULL); 
    // M? file nhacnen.mp3 và phát l?p l?i
    mciSendString("open \"nhacnen.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
}

// V? tàu ngu?i choi
void drawPlayer() {
    int x = player.x;
    int y = player.y;
    int r = player.radius; // S? d?ng radius làm t? l? phóng to/thu nh? cho toàn b? máy bay

    // ===== CÀI Ð?T MÀU S?C =====
    const int BODY_FILL_COLOR = CYAN;
    const int LINE_COLOR = LIGHTCYAN;
    const int COCKPIT_FILL_COLOR = BLUE; 
    const int POD_FILL_COLOR = LIGHTCYAN; 

    setcolor(LINE_COLOR); 
    setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

    // ===== 1. CORE BODY & MUI =====
    int body_points[] = {
        x, y - (int)(r * 1.6),            
        x - (int)(r * 0.4), y - (int)(r * 1.3), 
        x - (int)(r * 0.3), y - (int)(r * 0.4), 
        x - (int)(r * 0.3), y + (int)(r * 0.8), 
        x + (int)(r * 0.3), y + (int)(r * 0.8), 
        x + (int)(r * 0.3), y - (int)(r * 0.4), 
        x + (int)(r * 0.4), y - (int)(r * 1.3), 
        x, y - (int)(r * 1.6)             
    };
    fillpoly(8, body_points);

    // ===== 2. CÁNH TAM GIÁC & CÁC ÐU?NG PANEL =====
    // Cánh trái
    int left_wing[] = {
        x - (int)(r * 0.3), y - (int)(r * 0.3), 
        x - (int)(r * 1.5), y + (int)(r * 0.2), 
        x - (int)(r * 1.5), y + (int)(r * 0.9), 
        x - (int)(r * 0.3), y + (int)(r * 0.7), 
        x - (int)(r * 0.3), y - (int)(r * 0.3)  
    };
    fillpoly(5, left_wing);

    // Cánh ph?i
    int right_wing[] = {
        x + (int)(r * 0.3), y - (int)(r * 0.3), 
        x + (int)(r * 1.5), y + (int)(r * 0.2), 
        x + (int)(r * 1.5), y + (int)(r * 0.9), 
        x + (int)(r * 0.3), y + (int)(r * 0.7), 
        x + (int)(r * 0.3), y - (int)(r * 0.3)  
    };
    fillpoly(5, right_wing);

    // ÐU?NG PANEL VÀ CHI TI?T
    setcolor(BLUE); 
    line(x - (int)(r * 0.4), y - (int)(r * 0.1), x - (int)(r * 1.4), y + (int)(r * 0.3));
    line(x - (int)(r * 0.5), y + (int)(r * 0.1), x - (int)(r * 1.3), y + (int)(r * 0.4));
    line(x - (int)(r * 0.6), y + (int)(r * 0.3), x - (int)(r * 1.2), y + (int)(r * 0.5));
    line(x + (int)(r * 0.4), y - (int)(r * 0.1), x + (int)(r * 1.4), y + (int)(r * 0.3));
    line(x + (int)(r * 0.5), y + (int)(r * 0.1), x + (int)(r * 1.3), y + (int)(r * 0.4));
    line(x + (int)(r * 0.6), y + (int)(r * 0.3), x + (int)(r * 1.2), y + (int)(r * 0.5));
    line(x - (int)(r * 0.2), y - (int)(r * 1.2), x + (int)(r * 0.2), y - (int)(r * 1.2));
    line(x - (int)(r * 0.1), y - (int)(r * 1.0), x + (int)(r * 0.1), y - (int)(r * 1.0));
    line(x - (int)(r * 0.1), y - (int)(r * 0.5), x + (int)(r * 0.1), y - (int)(r * 0.5));

    // ===== 3. BU?NG LÁI =====
    int cockpit_points[] = {
        x, y - (int)(r * 0.9),           
        x - (int)(r * 0.1), y - (int)(r * 0.8), 
        x - (int)(r * 0.1), y - (int)(r * 0.6), 
        x + (int)(r * 0.1), y - (int)(r * 0.6), 
        x + (int)(r * 0.1), y - (int)(r * 0.8), 
        x, y - (int)(r * 0.9)            
    };
    setfillstyle(SOLID_FILL, COCKPIT_FILL_COLOR);
    fillpoly(6, cockpit_points);
    setcolor(LIGHTCYAN);
    line(x - (int)(r * 0.05), y - (int)(r * 0.8), x + (int)(r * 0.05), y - (int)(r * 0.8));
    line(x - (int)(r * 0.05), y - (int)(r * 0.7), x + (int)(r * 0.05), y - (int)(r * 0.7));

    // ===== 4. ÐUÔI VÀ ?NG X? PHÍA SAU =====
    setfillstyle(SOLID_FILL, BODY_FILL_COLOR);
    setcolor(LINE_COLOR);
    
    int left_tail[] = {
        x - (int)(r * 0.1), y + (int)(r * 0.7), 
        x - (int)(r * 0.6), y + (int)(r * 1.1), 
        x - (int)(r * 0.2), y + (int)(r * 1.2), 
        x - (int)(r * 0.1), y + (int)(r * 1.1)  
    };
    fillpoly(4, left_tail);
    
    int right_tail[] = {
        x + (int)(r * 0.1), y + (int)(r * 0.7), 
        x + (int)(r * 0.6), y + (int)(r * 1.1), 
        x + (int)(r * 0.2), y + (int)(r * 1.2), 
        x + (int)(r * 0.1), y + (int)(r * 1.1)  
    };
    fillpoly(4, right_tail);
    
    setcolor(BLUE);
    line(x - (int)(r * 0.2), y + (int)(r * 0.8), x - (int)(r * 0.5), y + (int)(r * 1.0));
    line(x + (int)(r * 0.2), y + (int)(r * 0.8), x + (int)(r * 0.5), y + (int)(r * 1.0));

    // ?NG X? Ð?NG CO
    bar(x - (int)(r * 0.15), y + (int)(r * 1.2), x - (int)(r * 0.05), y + (int)(r * 1.3));
    bar(x + (int)(r * 0.05), y + (int)(r * 1.2), x + (int)(r * 0.15), y + (int)(r * 1.3));

    // ===== 5. V? DU?I CÁNH VÀ VU KHÍ =====
    setfillstyle(SOLID_FILL, POD_FILL_COLOR);
    setcolor(BLUE);
    int left_pod[] = {
        x - (int)(r * 0.7), y + (int)(r * 0.4), 
        x - (int)(r * 0.9), y + (int)(r * 0.5), 
        x - (int)(r * 0.9), y + (int)(r * 0.8), 
        x - (int)(r * 0.7), y + (int)(r * 0.9)  
    };
    fillpoly(4, left_pod);
    
    int right_pod[] = {
        x + (int)(r * 0.7), y + (int)(r * 0.4), 
        x + (int)(r * 0.9), y + (int)(r * 0.5), 
        x + (int)(r * 0.9), y + (int)(r * 0.8), 
        x + (int)(r * 0.7), y + (int)(r * 0.9)  
    };
    fillpoly(4, right_pod);

    setcolor(LIGHTCYAN);
    line(x - (int)(r * 0.8), y + (int)(r * 0.5), x - (int)(r * 0.8), y + (int)(r * 0.8));
    line(x + (int)(r * 0.8), y + (int)(r * 0.5), x + (int)(r * 0.8), y + (int)(r * 0.8));

    recursiveBoundaryFill(x - (int)(r * 0.75), y + (int)(r * 0.6), LIGHTCYAN, LINE_COLOR);

    // ===== 6. HI?U ?NG L?A ? ÐUÔI (ANIMATED EXHAUST FIRE) =====
    // T?o d? dài ng?u nhiên (+0 d?n +0.4r) d? t?o hi?u ?ng nh?p nháy liên t?c m?i frame
    int flameLenL = (int)(r * 0.4) + rand() % (int)(r * 0.4 + 1);
    int flameLenR = (int)(r * 0.4) + rand() % (int)(r * 0.4 + 1);

    // L?a d?ng co TRÁI
    // L?p ngoài (Màu Ð? Sáng / Cam)
    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, LIGHTRED);
    int fire_left_outer[] = {
        x - (int)(r * 0.15), y + (int)(r * 1.3),
        x - (int)(r * 0.10), y + (int)(r * 1.3) + flameLenL, // Ð?nh nh?n c?a l?a
        x - (int)(r * 0.05), y + (int)(r * 1.3),
        x - (int)(r * 0.15), y + (int)(r * 1.3)              // Ðóng vòng
    };
    fillpoly(4, fire_left_outer);

    // L?p trong (Màu Vàng)
    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, YELLOW);
    int fire_left_inner[] = {
        x - (int)(r * 0.13), y + (int)(r * 1.3),
        x - (int)(r * 0.10), y + (int)(r * 1.3) + (int)(flameLenL * 0.6), 
        x - (int)(r * 0.07), y + (int)(r * 1.3),
        x - (int)(r * 0.13), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_left_inner);

    // L?a d?ng co PH?I
    // L?p ngoài
    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, LIGHTRED);
    int fire_right_outer[] = {
        x + (int)(r * 0.05), y + (int)(r * 1.3),
        x + (int)(r * 0.10), y + (int)(r * 1.3) + flameLenR,
        x + (int)(r * 0.15), y + (int)(r * 1.3),
        x + (int)(r * 0.05), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_right_outer);

    // L?p trong
    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, YELLOW);
    int fire_right_inner[] = {
        x + (int)(r * 0.07), y + (int)(r * 1.3),
        x + (int)(r * 0.10), y + (int)(r * 1.3) + (int)(flameLenR * 0.6),
        x + (int)(r * 0.13), y + (int)(r * 1.3),
        x + (int)(r * 0.07), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_right_inner);

    // ===== KHIÊN WITH [ÁP D?NG THU?T TOÁN 2]: MIDPOINT CIRCLE =====
    if (player.shieldTimer > 0) {
        midpointCircle(x, y, r + 10, LIGHTBLUE);
    }
}

// V? máy bay h? tr?
void drawCompanions() {
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (!companions[i].active) continue;

        int x = companions[i].x;
        int y = companions[i].y;
        int r = companions[i].radius; // Bán kính (t? l?) c?a máy bay d?ng d?i

        // ===== CÀI Ð?T MÀU S?C (Tone Xanh Lá) =====
        const int BODY_FILL_COLOR = GREEN;
        const int LINE_COLOR = LIGHTGREEN;
        const int COCKPIT_FILL_COLOR = DARKGRAY; // Bu?ng lái màu xám d?m
        const int POD_FILL_COLOR = LIGHTGREEN;

        setcolor(LINE_COLOR);
        setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

        // ===== 1. THÂN CHÍNH & MUI =====
        int body_points[] = {
            x, y - (int)(r * 1.6),            
            x - (int)(r * 0.4), y - (int)(r * 1.3), 
            x - (int)(r * 0.3), y - (int)(r * 0.4), 
            x - (int)(r * 0.3), y + (int)(r * 0.8), 
            x + (int)(r * 0.3), y + (int)(r * 0.8), 
            x + (int)(r * 0.3), y - (int)(r * 0.4), 
            x + (int)(r * 0.4), y - (int)(r * 1.3), 
            x, y - (int)(r * 1.6)             
        };
        fillpoly(8, body_points);

        // ===== 2. CÁNH TAM GIÁC =====
        // Cánh trái
        int left_wing[] = {
            x - (int)(r * 0.3), y - (int)(r * 0.3), 
            x - (int)(r * 1.5), y + (int)(r * 0.2), 
            x - (int)(r * 1.5), y + (int)(r * 0.9), 
            x - (int)(r * 0.3), y + (int)(r * 0.7), 
            x - (int)(r * 0.3), y - (int)(r * 0.3)  
        };
        fillpoly(5, left_wing);

        // Cánh ph?i
        int right_wing[] = {
            x + (int)(r * 0.3), y - (int)(r * 0.3), 
            x + (int)(r * 1.5), y + (int)(r * 0.2), 
            x + (int)(r * 1.5), y + (int)(r * 0.9), 
            x + (int)(r * 0.3), y + (int)(r * 0.7), 
            x + (int)(r * 0.3), y - (int)(r * 0.3)  
        };
        fillpoly(5, right_wing);

        // ===== 3. BU?NG LÁI =====
        int cockpit_points[] = {
            x, y - (int)(r * 0.9),           
            x - (int)(r * 0.1), y - (int)(r * 0.8), 
            x - (int)(r * 0.1), y - (int)(r * 0.6), 
            x + (int)(r * 0.1), y - (int)(r * 0.6), 
            x + (int)(r * 0.1), y - (int)(r * 0.8), 
            x, y - (int)(r * 0.9)            
        };
        setfillstyle(SOLID_FILL, COCKPIT_FILL_COLOR);
        fillpoly(6, cockpit_points);

        // ===== 4. ÐUÔI VÀ ?NG X? =====
        setfillstyle(SOLID_FILL, BODY_FILL_COLOR);
        setcolor(LINE_COLOR);
        
        int left_tail[] = {
            x - (int)(r * 0.1), y + (int)(r * 0.7), 
            x - (int)(r * 0.6), y + (int)(r * 1.1), 
            x - (int)(r * 0.2), y + (int)(r * 1.2), 
            x - (int)(r * 0.1), y + (int)(r * 1.1)  
        };
        fillpoly(4, left_tail);
        
        int right_tail[] = {
            x + (int)(r * 0.1), y + (int)(r * 0.7), 
            x + (int)(r * 0.6), y + (int)(r * 1.1), 
            x + (int)(r * 0.2), y + (int)(r * 1.2), 
            x + (int)(r * 0.1), y + (int)(r * 1.1)  
        };
        fillpoly(4, right_tail);

        // ?ng x? (Bars)
        bar(x - (int)(r * 0.15), y + (int)(r * 1.2), x - (int)(r * 0.05), y + (int)(r * 1.3));
        bar(x + (int)(r * 0.05), y + (int)(r * 1.2), x + (int)(r * 0.15), y + (int)(r * 1.3));

        // ===== 5. V? DU?I CÁNH (Vu khí/Ð?ng co ph?) =====
        setfillstyle(SOLID_FILL, POD_FILL_COLOR);
        int left_pod[] = {
            x - (int)(r * 0.7), y + (int)(r * 0.4), 
            x - (int)(r * 0.9), y + (int)(r * 0.5), 
            x - (int)(r * 0.9), y + (int)(r * 0.8), 
            x - (int)(r * 0.7), y + (int)(r * 0.9)  
        };
        fillpoly(4, left_pod);
        
        int right_pod[] = {
            x + (int)(r * 0.7), y + (int)(r * 0.4), 
            x + (int)(r * 0.9), y + (int)(r * 0.5), 
            x + (int)(r * 0.9), y + (int)(r * 0.8), 
            x + (int)(r * 0.7), y + (int)(r * 0.9)  
        };
        fillpoly(4, right_pod);

        // ===== 6. HI?U ?NG L?A (Ðon gi?n hóa cho d?ng d?i) =====
        // L?a ng?u nhiên ng?n hon máy bay chính m?t chút d? phù h?p t? l?
        int flameLen = (int)(r * 0.3) + rand() % (int)(r * 0.2 + 1);
        
        setcolor(LIGHTRED);
        setfillstyle(SOLID_FILL, YELLOW); // Lõi màu vàng, vi?n d?
        
        // L?a trái
        int fire_left[] = {
            x - (int)(r * 0.15), y + (int)(r * 1.3),
            x - (int)(r * 0.10), y + (int)(r * 1.3) + flameLen,
            x - (int)(r * 0.05), y + (int)(r * 1.3),
            x - (int)(r * 0.15), y + (int)(r * 1.3) // Ðóng vòng
        };
        fillpoly(4, fire_left);

        // L?a ph?i
        int fire_right[] = {
            x + (int)(r * 0.05), y + (int)(r * 1.3),
            x + (int)(r * 0.10), y + (int)(r * 1.3) + flameLen,
            x + (int)(r * 0.15), y + (int)(r * 1.3),
            x + (int)(r * 0.05), y + (int)(r * 1.3)
        };
        fillpoly(4, fire_right);
    }
}

// V? d?n
void drawBullets() {
    // T? d?ng quay v? d?n thu?ng n?u h?t th?i gian hi?u l?c
    if (player.currentFruitType == 1 && player.fireRateBoostTimer <= 0) player.currentFruitType = 0;
    if (player.currentFruitType == 5 && player.damageBoostTimer <= 0) player.currentFruitType = 0;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            int radius = player.bulletSizeTimer > 0 ? currentBulletSize * 1.5 : currentBulletSize;
            int bx = bullets[i].x;
            int by = bullets[i].y;

            if (player.currentFruitType == 1) { 
                // --- HÌNH CHU?I (Ð?N) M?M M?I ---
                setcolor(YELLOW);
                setfillstyle(SOLID_FILL, YELLOW);
                int w = radius * 2.0; // Chi?u dài chu?i d?a trên kích thu?c d?n
                int h = radius * 1.2; // Ð? cong
                
                // V? các di?m n?i ti?p nhau
                for (float t = -1.57; t <= 1.57; t += 0.15) {
                    int cx = bx + sin(t) * w;
                    int cy = by + cos(t) * h;
                    int r = (int)((radius * 0.9) * (1.0 - fabs(t) / 1.57));
                    if (r < 1) r = 1;
                    fillellipse(cx, cy, r, r);
                }
                
                // Núm chu?i
                setcolor(BROWN);
                setfillstyle(SOLID_FILL, BROWN);
                int numSize = radius / 3;
                if (numSize < 1) numSize = 1;
                fillellipse(bx + w, by, numSize, numSize);
            } 
            else if (player.currentFruitType == 5) { 
                // --- HÌNH TÁO (Ð?N) PHÓNG TO ---
                int r_apple = radius * 1.6; // Phóng to d?n táo lên 1.6 l?n so v?i bình thu?ng
                
                setcolor(LIGHTRED);
                setfillstyle(SOLID_FILL, LIGHTRED);
                // V? 2 n?a to bè ra d? th?y rõ lõm táo
                fillellipse(bx - r_apple/2 + 1, by, r_apple/2 + 2, r_apple); 
                fillellipse(bx + r_apple/2 - 1, by, r_apple/2 + 2, r_apple); 
                
                // Cu?ng
                setcolor(BROWN);
                line(bx, by - r_apple + 2, bx, by - r_apple - 8); 
                
                // Lá
                setcolor(LIGHTGREEN);
                setfillstyle(SOLID_FILL, LIGHTGREEN);
                fillellipse(bx + r_apple/2 + 1, by - r_apple - 4, r_apple/2, r_apple/3 + 1); 
            } 
            else { 
                // --- Ð?N THU?NG ---
                setcolor(WHITE);
                setfillstyle(SOLID_FILL, bullets[i].highDamage ? RED : YELLOW);
                fillellipse(bx, by, radius, radius);
            }
        }
    }
}

// V? k? d?ch
void drawEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        int size = enemies[i].radius;
        float angle = atan2(
            player.y - enemies[i].y,
            player.x - enemies[i].x
        );

        switch (enemies[i].type) {

        case 1: // Drone
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size,
                enemies[i].x + size,
                enemies[i].y + size
            );

            floodfill(enemies[i].x, enemies[i].y, LIGHTRED);

            setcolor(WHITE);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size,
                enemies[i].x + size,
                enemies[i].y + size
            );

            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(enemies[i].x, enemies[i].y, 5, 5);
            break;

        case 2: // Scout
        {
            setcolor(LIGHTGREEN);
            setfillstyle(SOLID_FILL, GREEN);

            int points[8];

            points[0] = enemies[i].x + size * cos(angle);
            points[1] = enemies[i].y + size * sin(angle);

            points[2] = enemies[i].x + size * cos(angle + 2.4);
            points[3] = enemies[i].y + size * sin(angle + 2.4);

            points[4] = enemies[i].x + size * cos(angle - 2.4);
            points[5] = enemies[i].y + size * sin(angle - 2.4);

            points[6] = points[0];
            points[7] = points[1];

            fillpoly(4, points);

            setcolor(WHITE);
            drawpoly(4, points);

            setcolor(YELLOW);

            line(
                enemies[i].x,
                enemies[i].y,
                enemies[i].x - size * cos(angle),
                enemies[i].y - size * sin(angle)
            );

            break;
        }

        case 3: // Tank
        {
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, LIGHTGRAY);

            int points_tank[14];

            for (int j = 0; j < 6; j++) {
                points_tank[j * 2] =
                    enemies[i].x + size * cos(j * PI / 3);

                points_tank[j * 2 + 1] =
                    enemies[i].y + size * sin(j * PI / 3);
            }

            points_tank[12] = points_tank[0];
            points_tank[13] = points_tank[1];

            fillpoly(7, points_tank);

            setcolor(WHITE);
            drawpoly(7, points_tank);

            setcolor(LIGHTCYAN);
            circle(enemies[i].x, enemies[i].y, size * 0.7);

            break;
        }

        case 4: // Chaser
            setcolor(MAGENTA);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size,
                size
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size);

            setcolor(LIGHTMAGENTA);
            circle(enemies[i].x, enemies[i].y, size * 1.2);

            break;

        case 5: // Sniper
        {
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);

            int points_sniper[12];

            for (int j = 0; j < 5; j++) {
                points_sniper[j * 2] =
                    enemies[i].x + size * cos(j * 2 * PI / 5);

                points_sniper[j * 2 + 1] =
                    enemies[i].y + size * sin(j * 2 * PI / 5);
            }

            points_sniper[10] = points_sniper[0];
            points_sniper[11] = points_sniper[1];

            fillpoly(6, points_sniper);

            setcolor(WHITE);
            drawpoly(6, points_sniper);

            setfillstyle(
                SOLID_FILL,
                (rand() % 2) ? WHITE : LIGHTCYAN
            );

            fillellipse(enemies[i].x, enemies[i].y, 5, 5);

            break;
        }

        case 6: // Bomber
            setcolor(RED);
            setfillstyle(SOLID_FILL, DARKGRAY);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size,
                size * 0.7
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size);

            break;

        case 7: // Spinner
        {
            setcolor(LIGHTBLUE);
            setfillstyle(SOLID_FILL, BLUE);

            int points_spinner[10];

            for (int j = 0; j < 4; j++) {
                points_spinner[j * 2] =
                    enemies[i].x +
                    size * cos(j * PI / 2 + enemies[i].zigzagTimer);

                points_spinner[j * 2 + 1] =
                    enemies[i].y +
                    size * sin(j * PI / 2 + enemies[i].zigzagTimer);
            }

            points_spinner[8] = points_spinner[0];
            points_spinner[9] = points_spinner[1];

            fillpoly(5, points_spinner);

            setcolor(WHITE);
            drawpoly(5, points_spinner);

            break;
        }

        case 8: // Stealth
        {
            setcolor(LIGHTGRAY);
            setfillstyle(SOLID_FILL, DARKGRAY);

            int points_stealth[8];

            points_stealth[0] =
                enemies[i].x + size * cos(angle + 0.5);

            points_stealth[1] =
                enemies[i].y + size * sin(angle + 0.5);

            points_stealth[2] =
                enemies[i].x + size * cos(angle + 3.14);

            points_stealth[3] =
                enemies[i].y + size * sin(angle + 3.14);

            points_stealth[4] =
                enemies[i].x + size * cos(angle - 0.5);

            points_stealth[5] =
                enemies[i].y + size * sin(angle - 0.5);

            points_stealth[6] = points_stealth[0];
            points_stealth[7] = points_stealth[1];

            fillpoly(4, points_stealth);

            setcolor(WHITE);
            drawpoly(4, points_stealth);

            break;
        }

        case 9: // Kamikaze
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, LIGHTRED);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size,
                size
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size);

            break;

        case 10: // Dodger
        {
            setcolor(LIGHTCYAN);
            setfillstyle(SOLID_FILL, CYAN);

            int points_dodger[10];

            for (int j = 0; j < 4; j++) {
                points_dodger[j * 2] =
                    enemies[i].x + size * cos(j * PI / 2);

                points_dodger[j * 2 + 1] =
                    enemies[i].y + size * sin(j * PI / 2);
            }

            points_dodger[8] = points_dodger[0];
            points_dodger[9] = points_dodger[1];

            fillpoly(5, points_dodger);

            setcolor(WHITE);
            drawpoly(5, points_dodger);

            break;
        }

        case 11: // Blaster
            setcolor(MAGENTA);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size * 0.7,
                enemies[i].x + size,
                enemies[i].y + size * 0.7
            );

            floodfill(enemies[i].x, enemies[i].y, MAGENTA);

            setcolor(WHITE);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size * 0.7,
                enemies[i].x + size,
                enemies[i].y + size * 0.7
            );

            break;

        case 12: // Swarmer
            setcolor(GREEN);
            setfillstyle(SOLID_FILL, LIGHTGREEN);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size * 0.7,
                size * 0.7
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size * 0.7);

            break;

        case 13: // Phantom
        {
            setcolor(LIGHTBLUE);
            setfillstyle(SOLID_FILL, BLUE);

            int points_phantom[12];

            for (int j = 0; j < 5; j++) {
                points_phantom[j * 2] =
                    enemies[i].x +
                    size * cos(j * 2 * PI / 5 + enemies[i].zigzagTimer);

                points_phantom[j * 2 + 1] =
                    enemies[i].y +
                    size * sin(j * 2 * PI / 5 + enemies[i].zigzagTimer);
            }

            points_phantom[10] = points_phantom[0];
            points_phantom[11] = points_phantom[1];

            fillpoly(6, points_phantom);

            setcolor(WHITE);
            drawpoly(6, points_phantom);

            break;
        }

        case 14: // Charger
        {
            setcolor(RED);
            setfillstyle(SOLID_FILL, LIGHTRED);

            int points_charger[8];

            points_charger[0] =
                enemies[i].x + size * cos(angle);

            points_charger[1] =
                enemies[i].y + size * sin(angle);

            points_charger[2] =
                enemies[i].x + size * cos(angle + 2.8);

            points_charger[3] =
                enemies[i].y + size * sin(angle + 2.8);

            points_charger[4] =
                enemies[i].x + size * cos(angle - 2.8);

            points_charger[5] =
                enemies[i].y + size * sin(angle - 2.8);

            points_charger[6] = points_charger[0];
            points_charger[7] = points_charger[1];

            fillpoly(4, points_charger);

            setcolor(WHITE);
            drawpoly(4, points_charger);

            break;
        }

        case 15: // Sniper Elite
        {
            setcolor(CYAN);
            setfillstyle(SOLID_FILL, LIGHTCYAN);

            int points_elite[14];

            for (int j = 0; j < 6; j++) {
                points_elite[j * 2] =
                    enemies[i].x + size * cos(j * PI / 3);

                points_elite[j * 2 + 1] =
                    enemies[i].y + size * sin(j * PI / 3);
            }

            points_elite[12] = points_elite[0];
            points_elite[13] = points_elite[1];

            fillpoly(7, points_elite);

            setcolor(WHITE);
            drawpoly(7, points_elite);

            break;
        }

        case 16: // Boss 1: Circle Shooter
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size,
                size
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size);

            setcolor(RED);
            circle(enemies[i].x, enemies[i].y, size * 0.8);

            break;

        case 17: // Boss 2: Grid Shooter
            setcolor(MAGENTA);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size,
                enemies[i].x + size,
                enemies[i].y + size
            );

            floodfill(enemies[i].x, enemies[i].y, MAGENTA);

            setcolor(WHITE);

            rectangle(
                enemies[i].x - size,
                enemies[i].y - size,
                enemies[i].x + size,
                enemies[i].y + size
            );

            setcolor(YELLOW);
            circle(enemies[i].x, enemies[i].y, size * 0.5);

            break;

        case 18: // Boss 3: Summoner
        {
            setcolor(CYAN);
            setfillstyle(SOLID_FILL, LIGHTCYAN);

            int points_boss3[16];

            for (int j = 0; j < 7; j++) {
                points_boss3[j * 2] =
                    enemies[i].x + size * cos(j * 2 * PI / 7);

                points_boss3[j * 2 + 1] =
                    enemies[i].y + size * sin(j * 2 * PI / 7);
            }

            points_boss3[14] = points_boss3[0];
            points_boss3[15] = points_boss3[1];

            fillpoly(8, points_boss3);

            setcolor(WHITE);
            drawpoly(8, points_boss3);

            break;
        }

        case 19: // Boss 4: Spiral Shooter
            setcolor(RED);
            setfillstyle(SOLID_FILL, LIGHTRED);

            fillellipse(
                enemies[i].x,
                enemies[i].y,
                size,
                size * 0.8
            );

            setcolor(WHITE);
            circle(enemies[i].x, enemies[i].y, size);

            setcolor(YELLOW);
            circle(enemies[i].x, enemies[i].y, size * 0.6);

            break;
        }
    }
}

// V? v?t ph?m
void drawPowerUps() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerUps[i].active)
            continue;

        int px = powerUps[i].x;
        int py = powerUps[i].y;

        switch (powerUps[i].type) {
            case 1: // Fire Rate -> Hình Chu?i M?m M?i
            {
                setcolor(YELLOW);
                setfillstyle(SOLID_FILL, YELLOW);
                for (float t = -1.57; t <= 1.57; t += 0.15) {
                    int cx = px + sin(t) * 14; 
                    int cy = py + cos(t) * 8;  
                    int r = (int)(6 * (1.0 - fabs(t) / 1.57)); 
                    if (r < 1) r = 1;
                    fillellipse(cx, cy, r, r);
                }
                setcolor(BROWN);
                setfillstyle(SOLID_FILL, BROWN);
                fillellipse(px + 14, py, 2, 2);
                break;
            }

            case 2: // Speed -> Hình Tia Ch?p (Lightning Bolt)
            {
                setcolor(LIGHTCYAN);
                setfillstyle(SOLID_FILL, LIGHTCYAN);
                int lightning[14] = {
                    px + 2, py - 8,
                    px - 6, py + 2,
                    px,     py + 2,
                    px - 2, py + 8,
                    px + 6, py - 2,
                    px,     py - 2,
                    px + 2, py - 8
                };
                fillpoly(7, lightning);
                break;
            }

            case 3: // Life -> Hình Trái Tim (Heart)
            {
                setcolor(LIGHTRED);
                setfillstyle(SOLID_FILL, LIGHTRED);
                // 2 n?a vòng tròn ? trên
                fillellipse(px - 4, py - 3, 4, 4); 
                fillellipse(px + 4, py - 3, 4, 4); 
                // Hình tam giác nh?n ? du?i
                int heart_bottom[8] = {
                    px - 8, py - 2, 
                    px + 8, py - 2, 
                    px,     py + 7, 
                    px - 8, py - 2
                };
                fillpoly(4, heart_bottom);
                break;
            }

            case 4: // Shield -> Hình Chi?c Khiên (Shield shape)
            {
                setcolor(LIGHTBLUE);
                setfillstyle(SOLID_FILL, LIGHTBLUE);
                int shield[12] = {
                    px - 7, py - 6,  // Góc trên trái
                    px + 7, py - 6,  // Góc trên ph?i
                    px + 7, py + 2,  // C?nh ph?i
                    px,     py + 9,  // Mui nh?n du?i
                    px - 7, py + 2,  // C?nh trái
                    px - 7, py - 6
                };
                fillpoly(6, shield);
                
                // V? ch? th?p tr?ng ? gi?a khiên
                setcolor(WHITE);
                line(px, py - 4, px, py + 2);
                line(px - 3, py - 1, px + 3, py - 1);
                break;
            }

            case 5: // Damage Boost -> Hình Táo
            {
                setcolor(LIGHTRED);
                setfillstyle(SOLID_FILL, LIGHTRED);
                fillellipse(px - 6, py, 8, 10); 
                fillellipse(px + 6, py, 8, 10); 
                
                setcolor(BROWN);
                line(px, py - 8, px, py - 16);
                
                setcolor(LIGHTGREEN);
                setfillstyle(SOLID_FILL, LIGHTGREEN);
                fillellipse(px + 7, py - 12, 5, 3);
                break;
            }
            
			case 6: // Bullet Size -> Hình Kính Lúp (Magnifying Glass) soi viên d?n
            {
                // 1. V? tròng kính lúp (màu xanh ng?c sáng)
                setcolor(WHITE);
                setfillstyle(SOLID_FILL, LIGHTCYAN);
                fillellipse(px - 3, py - 3, 6, 6);
                
                // 2. V? viên d?n màu vàng ? tâm kính lúp (?n d? vi?c d?n du?c phóng to)
                setcolor(YELLOW);
                setfillstyle(SOLID_FILL, YELLOW);
                fillellipse(px - 3, py - 3, 2, 2);
                
                // 3. V? tay c?m c?a kính lúp (màu nâu, chéo xu?ng góc du?i bên ph?i)
                setcolor(BROWN);
                // V? 3 du?ng th?ng sát nhau d? t?o d? dày cho tay c?m
                line(px + 1, py + 1, px + 7, py + 7);
                line(px + 2, py + 1, px + 8, py + 7);
                line(px + 1, py + 2, px + 7, py + 8);
                break;
            }

            case 7: // Ultimate Charge -> Hình Ngôi Sao 5 Cánh (Star) - Ð?i t? case 8
            {
                setcolor(LIGHTMAGENTA);
                setfillstyle(SOLID_FILL, LIGHTMAGENTA);
                int star[22];
                // Tính toán t?a d? 10 d?nh c?a ngôi sao
                for (int j = 0; j < 10; j++) {
                    float angle = j * PI / 5 - PI / 2;
                    int r = (j % 2 == 0) ? 10 : 4; // Ð?nh ngoài xa, d?nh trong g?n
                    star[j * 2] = px + cos(angle) * r;
                    star[j * 2 + 1] = py + sin(angle) * r;
                }
                star[20] = star[0]; 
                star[21] = star[1];
                fillpoly(11, star);
                break;
            }
            case 8: // Companion Boost -> Hình máy bay d?ng d?i thu nh? có vòng tròn bao quanh
            {
                int r = 6; // Ðã thu nh? bán kính t? l? t? 10 xu?ng 6

                // ===== CÀI Ð?T MÀU S?C =====
                const int BODY_FILL_COLOR = GREEN;
                const int LINE_COLOR = LIGHTGREEN;
                const int COCKPIT_FILL_COLOR = DARKGRAY;
                const int POD_FILL_COLOR = LIGHTGREEN;

                setcolor(LINE_COLOR);
                setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

                // ===== 1. THÂN CHÍNH & MUI =====
                int body_points[] = {
                    px, py - (int)(r * 1.6),            
                    px - (int)(r * 0.4), py - (int)(r * 1.3), 
                    px - (int)(r * 0.3), py - (int)(r * 0.4), 
                    px - (int)(r * 0.3), py + (int)(r * 0.8), 
                    px + (int)(r * 0.3), py + (int)(r * 0.8), 
                    px + (int)(r * 0.3), py - (int)(r * 0.4), 
                    px + (int)(r * 0.4), py - (int)(r * 1.3), 
                    px, py - (int)(r * 1.6)              
                };
                fillpoly(8, body_points);

                // ===== 2. CÁNH TAM GIÁC =====
                int left_wing[] = {
                    px - (int)(r * 0.3), py - (int)(r * 0.3), 
                    px - (int)(r * 1.5), py + (int)(r * 0.2), 
                    px - (int)(r * 1.5), py + (int)(r * 0.9), 
                    px - (int)(r * 0.3), py + (int)(r * 0.7), 
                    px - (int)(r * 0.3), py - (int)(r * 0.3)  
                };
                fillpoly(5, left_wing);

                int right_wing[] = {
                    px + (int)(r * 0.3), py - (int)(r * 0.3), 
                    px + (int)(r * 1.5), py + (int)(r * 0.2), 
                    px + (int)(r * 1.5), py + (int)(r * 0.9), 
                    px + (int)(r * 0.3), py + (int)(r * 0.7), 
                    px + (int)(r * 0.3), py - (int)(r * 0.3)  
                };
                fillpoly(5, right_wing);

                // ===== 3. BU?NG LÁI =====
                int cockpit_points[] = {
                    px, py - (int)(r * 0.9),            
                    px - (int)(r * 0.1), py - (int)(r * 0.8), 
                    px - (int)(r * 0.1), py - (int)(r * 0.6), 
                    px + (int)(r * 0.1), py - (int)(r * 0.6), 
                    px + (int)(r * 0.1), py - (int)(r * 0.8), 
                    px, py - (int)(r * 0.9)             
                };
                setfillstyle(SOLID_FILL, COCKPIT_FILL_COLOR);
                fillpoly(6, cockpit_points);

                // ===== 4. ÐUÔI VÀ ?NG X? =====
                setfillstyle(SOLID_FILL, BODY_FILL_COLOR);
                setcolor(LINE_COLOR);
                
                int left_tail[] = {
                    px - (int)(r * 0.1), py + (int)(r * 0.7), 
                    px - (int)(r * 0.6), py + (int)(r * 1.1), 
                    px - (int)(r * 0.2), py + (int)(r * 1.2), 
                    px - (int)(r * 0.1), py + (int)(r * 1.1)  
                };
                fillpoly(4, left_tail);
                
                int right_tail[] = {
                    px + (int)(r * 0.1), py + (int)(r * 0.7), 
                    px + (int)(r * 0.6), py + (int)(r * 1.1), 
                    px + (int)(r * 0.2), py + (int)(r * 1.2), 
                    px + (int)(r * 0.1), py + (int)(r * 1.1)  
                };
                fillpoly(4, right_tail);

                bar(px - (int)(r * 0.15), py + (int)(r * 1.2), px - (int)(r * 0.05), py + (int)(r * 1.3));
                bar(px + (int)(r * 0.05), py + (int)(r * 1.2), px + (int)(r * 0.15), py + (int)(r * 1.3));

                // ===== 5. VU KHÍ DU?I CÁNH =====
                setfillstyle(SOLID_FILL, POD_FILL_COLOR);
                int left_pod[] = {
                    px - (int)(r * 0.7), py + (int)(r * 0.4), 
                    px - (int)(r * 0.9), py + (int)(r * 0.5), 
                    px - (int)(r * 0.9), py + (int)(r * 0.8), 
                    px - (int)(r * 0.7), py + (int)(r * 0.9)  
                };
                fillpoly(4, left_pod);
                
                int right_pod[] = {
                    px + (int)(r * 0.7), py + (int)(r * 0.4), 
                    px + (int)(r * 0.9), py + (int)(r * 0.5), 
                    px + (int)(r * 0.9), py + (int)(r * 0.8), 
                    px + (int)(r * 0.7), py + (int)(r * 0.9)  
                };
                fillpoly(4, right_pod);

                // ===== 6. HI?U ?NG L?A =====
                int flameLen = (int)(r * 0.3) + rand() % ((int)(r * 0.2) + 1);
                
                setcolor(LIGHTRED);
                setfillstyle(SOLID_FILL, YELLOW);
                
                int fire_left[] = {
                    px - (int)(r * 0.15), py + (int)(r * 1.3),
                    px - (int)(r * 0.10), py + (int)(r * 1.3) + flameLen,
                    px - (int)(r * 0.05), py + (int)(r * 1.3),
                    px - (int)(r * 0.15), py + (int)(r * 1.3) 
                };
                fillpoly(4, fire_left);

                int fire_right[] = {
                    px + (int)(r * 0.05), py + (int)(r * 1.3),
                    px + (int)(r * 0.10), py + (int)(r * 1.3) + flameLen,
                    px + (int)(r * 0.15), py + (int)(r * 1.3),
                    px + (int)(r * 0.05), py + (int)(r * 1.3)
                };
                fillpoly(4, fire_right);
                
                // ===== 7. VÒNG TRÒN TR?NG BAO QUANH =====
                setcolor(WHITE);
                circle(px, py, 14); // Bán kính 14 bao tr?n v?n chi?c máy bay ? bên trong
                
                break;
            }
        }
    }
}
// V? h?t v? n?
void drawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            setcolor(particles[i].life > 15 ? YELLOW : RED);
            circle(particles[i].x, particles[i].y, 2);
        }
    }
}

// V? n?n
void drawBackground() {
    cleardevice();
    setcolor(WHITE);
    for (int i = 0; i < MAX_STARS; i++) {
        circle(stars[i].x, stars[i].y, stars[i].radius);
    }
}

// C?p nh?t ngôi sao
void updateStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].speed;
        if (stars[i].y > SCREEN_HEIGHT) {
            stars[i].y = 0;
            stars[i].x = rand() % SCREEN_WIDTH;
        }
    }
}

void updatePlayer() {
    if (!gameOver && !gameWon) {
        // ===== C? Ð?NH V? TRÍ Y & HU?NG SÚNG =====
        player.lastAngle = -PI / 2;  // Luôn hu?ng lên
        player.y = SCREEN_HEIGHT - player.radius - 20; 
        player.dy = 0;
        player.targetDy = 0;

        // ===== ÐI?U KHI?N NGANG (S?A L?I KH?NG & K?T PHÍM UNIKEY) =====
        player.dx = 0; // Ð?t m?c d?nh là 0 tru?c khi ki?m tra phím

        // S? d?ng mã Hex (0x41 = A, 0x44 = D) d? tránh l?i k?t phím do b? gõ Ti?ng Vi?t
        if (GetAsyncKeyState(0x41) & 0x8000) {
            player.dx -= currentPlayerSpeed;
        }
        if (GetAsyncKeyState(0x44) & 0x8000) {
            player.dx += currentPlayerSpeed;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            exit(0);

        // ===== C?P NH?T V? TRÍ =====
        player.x += player.dx; 

        // ===== GI? PLAYER TRONG MÀN HÌNH =====
        if (player.x < player.radius)
            player.x = player.radius;

        if (player.x > SCREEN_WIDTH - player.radius)
            player.x = SCREEN_WIDTH - player.radius;

        // ===== B?N Ð?N (CHU?T TRÁI) =====
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            clock_t now = clock();
            if ((now - lastShotTime) * 1000 / CLOCKS_PER_SEC >= currentShootDelay) {
                shootBullet();      // Ð?n s? b?n lên trên
                lastShotTime = now;
            }
        }

        // ===== ULTIMATE (CHU?T PH?I) =====
        if (player.ultimateTimer < ULTIMATE_COOLDOWN) {
            player.ultimateTimer += 0.02;
            if (player.ultimateTimer > ULTIMATE_COOLDOWN)
                player.ultimateTimer = ULTIMATE_COOLDOWN;
        }

        if (ismouseclick(WM_RBUTTONDOWN)) {
            clearmouseclick(WM_RBUTTONDOWN);
            if (player.ultimateTimer >= ULTIMATE_COOLDOWN) {
                triggerUltimate();
                player.ultimateTimer = 0;
            }
        }

        // ===== BUFF / POWER-UP =====
        if (player.speedBoostTimer > 0) {
            currentPlayerSpeed = 7;
            player.speedBoostTimer -= 0.02;
            if (player.speedBoostTimer <= 0)
                currentPlayerSpeed = PLAYER_SPEED;
        }

        if (player.fireRateBoostTimer > 0) {
            currentShootDelay = 100;
            player.fireRateBoostTimer -= 0.02;
            if (player.fireRateBoostTimer <= 0)
                currentShootDelay = SHOOT_DELAY;
        }

        if (player.bulletSizeTimer > 0) {
            currentBulletSize = 7.5;
            player.bulletSizeTimer -= 0.02;
            if (player.bulletSizeTimer <= 0)
                currentBulletSize = 5;
        }

        if (player.shieldTimer > 0)
            player.shieldTimer -= 0.02;

        if (player.damageBoostTimer > 0)
            player.damageBoostTimer -= 0.02;

        // ===== BUFF MÁY BAY Ð?NG Ð?I =====
        if (player.companionBoostTimer > 0) {
            player.companionBoostTimer -= 0.02; // Gi?m d?n th?i gian buff
        
            updateCompanions(); 
            
        } else {
            // Khi h?t th?i gian buff, l?p t?c t?t c? 2 máy bay
            companions[1].active = false;
            companions[2].active = false;
        }
    }
}

// B?n d?n tàu chính
void shootBullet() {
	// ---------- THÊM DÒNG NÀY VÀO Ð?U HÀM ----------
    PlaySound(TEXT("ban.wav"), NULL, SND_FILENAME | SND_ASYNC);
    // Ð?n player
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = player.x;
            bullets[i].y = player.y - player.radius;
            bullets[i].dx = 0;
            bullets[i].dy = -BULLET_SPEED;
            bullets[i].highDamage = player.damageBoostTimer > 0;
            break;
        }
    }

    // === Companion b?n theo ===
    for (int i = 0; i < 3; i++) {
        if (!companions[i].active) continue;

        for (int j = 0; j < MAX_BULLETS; j++) {
            if (!bullets[j].active) {
                bullets[j].active = true;
                bullets[j].x = companions[i].x;
                bullets[j].y = companions[i].y - companions[i].radius;
                bullets[j].dx = 0;
                bullets[j].dy = -BULLET_SPEED;
                bullets[j].highDamage = player.damageBoostTimer > 0;
                break;
            }
        }
    }
}

// C?p nh?t máy bay h? tr?
void updateCompanions() {
    // Kho?ng cách bên Trái (-60) và Ph?i (+60)
    int sideOffsetX[2] = { -60, 60 }; 
    // Ð? cao: Ð? 10 d? máy bay ph? lùi l?i phía sau m?t chút so v?i máy bay chính
    int sideOffsetY = 10; 

    // ===== Máy bay bên TRÁI =====
    // (Dùng index 1 gi?ng v?i code b?t máy bay ? các ph?n tru?c c?a b?n)
    if (companions[1].active) {
        companions[1].x = player.x + sideOffsetX[0];
        companions[1].y = player.y + sideOffsetY;
    }

    // ===== Máy bay bên PH?I =====
    // (Dùng index 2)
    if (companions[2].active) {
        companions[2].x = player.x + sideOffsetX[1];
        companions[2].y = player.y + sideOffsetY;
    }
}
// C?p nh?t d?n
void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].dx;
            bullets[i].y += bullets[i].dy;
            if (bullets[i].x < 0 || bullets[i].x > SCREEN_WIDTH || 
                bullets[i].y < 0 || bullets[i].y > SCREEN_HEIGHT) {
                bullets[i].active = false;
            }
        }
    }
}

// Sinh k? d?ch
void spawnEnemy() {
    if (gameOver || gameWon) return;

    int spawnChance = 50 - difficultyLevel * 10 - postBossDifficulty * 5;
    if (spawnChance < 10) spawnChance = 10;

    if (rand() % spawnChance != 0) return;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = rand() % SCREEN_WIDTH;
            enemies[i].y = -20;   // ngoài màn hình m?t chút

            int randType = rand() % 100;
            int type;

            if (postBossDifficulty > 0) {
                if (randType < 10) type = 1;
                else if (randType < 20) type = 2;
                else if (randType < 30) type = 3;
                else if (randType < 40) type = 4;
                else if (randType < 50) type = 5;
                else if (randType < 60) type = 6;
                else if (randType < 70) type = 7;
                else if (randType < 80) type = 8;
                else if (randType < 85) type = 9;
                else if (randType < 90) type = 10;
                else if (randType < 93) type = 11;
                else if (randType < 95) type = 12;
                else if (randType < 97) type = 13;
                else if (randType < 99) type = 14;
                else type = 15;
            } else {
                if (randType < 20) type = 1;
                else if (randType < 35) type = 2;
                else if (randType < 50) type = 3;
                else if (randType < 60) type = 4;
                else if (randType < 70) type = 5;
                else if (randType < 75) type = 6;
                else if (randType < 80) type = 7;
                else if (randType < 85) type = 8;
                else if (randType < 90) type = 9;
                else if (randType < 92) type = 10;
                else if (randType < 94) type = 11;
                else if (randType < 96) type = 12;
                else if (randType < 98) type = 13;
                else if (randType < 99) type = 14;
                else type = 15;
            }

            enemies[i].type = type;
            enemies[i].radius =
                (type <= 5) ? (10 + type * 2) :
                (type <= 10) ? (12 + type) :
                               (15 + type / 2);

            enemies[i].health =
                (type <= 5)  ? (1 + type / 4 + difficultyLevel / 2 + postBossDifficulty) :
                (type <= 10) ? (1 + type / 6 + difficultyLevel / 2 + postBossDifficulty) :
                               (2 + type / 8 + difficultyLevel / 2 + postBossDifficulty);

            enemies[i].zigzagTimer = 0;
            enemies[i].dashTimer = 0;
            enemies[i].shootTimer = 0;
            enemies[i].specialTimer = 0;
            enemies[i].active = true;
            break;
        }
    }
}

// Sinh boss
void spawnBoss() {
    if (bossActive) return;
    int bossType = 0;
    if (score >= 2000) bossType = 19;
    else if (score >= 1500) bossType = 18;
    else if (score >= 1000) bossType = 17;
    else if (score >= 500) bossType = 16;
    else return;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = SCREEN_WIDTH / 2;
            enemies[i].y = 50;
            enemies[i].type = bossType;
            enemies[i].radius = bossType == 16 ? 40 : bossType == 17 ? 50 : bossType == 18 ? 60 : 70;
            enemies[i].health = bossType == 16 ? 50 : bossType == 17 ? 100 : bossType == 18 ? 150 : 200;
            enemies[i].zigzagTimer = 0;
            enemies[i].dashTimer = 0;
            enemies[i].shootTimer = 0;
            enemies[i].specialTimer = 0;
            enemies[i].active = true;
            bossActive = true;
            postBossDifficulty = 0;
            break;
        }
    }
}

// C?p nh?t k? d?ch
void updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            float dx = player.x - enemies[i].x;
            float dy = player.y - enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            float speed = 2.0 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
            if (enemies[i].type == 2 || enemies[i].type == 7 || enemies[i].type == 12) speed = 3.5 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
            else if (enemies[i].type == 3) speed = 1.5 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
            else if (enemies[i].type == 4 || enemies[i].type == 9 || enemies[i].type == 14) {
                speed = 2.5 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
                enemies[i].dashTimer += 0.02;
                if (enemies[i].dashTimer > 2) {
                    speed = 5.0 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
                    if (enemies[i].dashTimer > 2.5) enemies[i].dashTimer = 0;
                }
            }
            else if (enemies[i].type == 5 || enemies[i].type == 11 || enemies[i].type == 15) speed = 1.0 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;
            else if (enemies[i].type >= 16) speed = enemies[i].type == 19 ? 3.0 : 1.5;

            if (dist > 0) {
                enemies[i].dx = speed * dx / dist;
                enemies[i].dy = speed * dy / dist;
            }

            if (enemies[i].type == 2 || enemies[i].type == 7 || enemies[i].type == 13) {
                enemies[i].zigzagTimer += 0.1;
                enemies[i].x += 5 * sin(enemies[i].zigzagTimer);
            }

            if (enemies[i].type == 10) {
                for (int j = 0; j < MAX_BULLETS; j++) {
                    if (bullets[j].active) {
                        float bulletDist = sqrt(pow(bullets[j].x - enemies[i].x, 2) + pow(bullets[j].y - enemies[i].y, 2));
                        if (bulletDist < 50) {
                            float angle = atan2(bullets[j].y - enemies[i].y, bullets[j].x - enemies[i].x);
                            enemies[i].dx -= 2 * cos(angle);
                            enemies[i].dy -= 2 * sin(angle);
                        }
                    }
                }
            }

            if (enemies[i].type == 5 || enemies[i].type == 11 || enemies[i].type == 15) {
                enemies[i].shootTimer += 0.02;
                if (enemies[i].shootTimer > 2) {
                    for (int j = 0; j < MAX_BULLETS; j++) {
                        if (!bullets[j].active) {
                            bullets[j].x = enemies[i].x;
                            bullets[j].y = enemies[i].y;
                            float angle = atan2(player.y - enemies[i].y, player.x - enemies[i].x);
                            bullets[j].dx = BULLET_SPEED * cos(angle) * 0.5;
                            bullets[j].dy = BULLET_SPEED * sin(angle) * 0.5;
                            bullets[j].active = true;
                            break;
                        }
                    }
                    enemies[i].shootTimer = 0;
                }
                if (dist < 200) {
                    enemies[i].dx = 0;
                    enemies[i].dy = 0;
                }
            }

            if (enemies[i].type == 16) {
                enemies[i].specialTimer += 0.02;
                if (enemies[i].specialTimer > 3) {
                    for (int j = 0; j < 8; j++) {
                        for (int k = 0; k < MAX_BULLETS; k++) {
                            if (!bullets[k].active) {
                                bullets[k].x = enemies[i].x;
                                bullets[k].y = enemies[i].y;
                                float angle = j * PI / 4;
                                bullets[k].dx = BULLET_SPEED * cos(angle) * 0.4;
                                bullets[k].dy = BULLET_SPEED * sin(angle) * 0.4;
                                bullets[k].active = true;
                                break;
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            if (enemies[i].type == 17) {
                enemies[i].specialTimer += 0.02;
                if (enemies[i].specialTimer > 4) {
                    for (int j = -2; j <= 2; j++) {
                        for (int k = -2; k <= 2; k++) {
                            for (int l = 0; l < MAX_BULLETS; l++) {
                                if (!bullets[l].active) {
                                    bullets[l].x = enemies[i].x;
                                    bullets[l].y = enemies[i].y;
                                    float angle = atan2(player.y - enemies[i].y + k * 50, player.x - enemies[i].x + j * 50);
                                    bullets[l].dx = BULLET_SPEED * cos(angle) * 0.3;
                                    bullets[l].dy = BULLET_SPEED * sin(angle) * 0.3;
                                    bullets[l].active = true;
                                    break;
                                }
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            if (enemies[i].type == 18) {
                enemies[i].specialTimer += 0.02;
                if (enemies[i].specialTimer > 5) {
                    for (int j = 0; j < 2; j++) {
                        for (int k = 0; k < MAX_ENEMIES; k++) {
                            if (!enemies[k].active) {
                                enemies[k].x = enemies[i].x + (rand() % 100 - 50);
                                enemies[k].y = enemies[i].y + (rand() % 100 - 50);
                                enemies[k].type = rand() % 2 + 1;
                                enemies[k].radius = enemies[k].type == 1 ? 15 : 10;
                                enemies[k].health = 1 + difficultyLevel / 2 + postBossDifficulty;
                                enemies[k].zigzagTimer = 0;
                                enemies[k].dashTimer = 0;
                                enemies[k].shootTimer = 0;
                                enemies[k].specialTimer = 0;
                                enemies[k].active = true;
                                break;
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            if (enemies[i].type == 19) {
                enemies[i].specialTimer += 0.02;
                enemies[i].zigzagTimer += 0.1;
                enemies[i].x += 10 * sin(enemies[i].zigzagTimer);
                if (enemies[i].specialTimer > 2) {
                    for (int j = 0; j < 10; j++) {
                        for (int k = 0; k < MAX_BULLETS; k++) {
                            if (!bullets[k].active) {
                                bullets[k].x = enemies[i].x;
                                bullets[k].y = enemies[i].y;
                                float angle = enemies[i].specialTimer * 2 + j * PI / 5;
                                bullets[k].dx = BULLET_SPEED * cos(angle) * 0.5;
                                bullets[k].dy = BULLET_SPEED * sin(angle) * 0.5;
                                bullets[k].active = true;
                                break;
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            enemies[i].x += enemies[i].dx;
            enemies[i].y += enemies[i].dy;
            if (enemies[i].x < enemies[i].radius) enemies[i].x = enemies[i].radius;
            if (enemies[i].x > SCREEN_WIDTH - enemies[i].radius) enemies[i].x = SCREEN_WIDTH - enemies[i].radius;
            if (enemies[i].y < enemies[i].radius) enemies[i].y = enemies[i].radius;
            if (enemies[i].y > SCREEN_HEIGHT - enemies[i].radius) enemies[i].y = SCREEN_HEIGHT - enemies[i].radius;
        }
    }
}

// T?o v?t ph?m
void spawnPowerUp(float x, float y) {
    if (rand() % 100 < 40) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (!powerUps[i].active) {
                powerUps[i].x = x;
                powerUps[i].y = y;
                powerUps[i].dy = 2.0;
                
                // C?P NH?T: Ch? random t? 1 d?n 8 tuong ?ng v?i 8 lo?i v?t ph?m hi?n t?i
                powerUps[i].type = rand() % 8 + 1; 
                
                powerUps[i].active = true;
                break;
            }
        }
    }
}

// C?p nh?t v?t ph?m
void updatePowerUps() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerUps[i].active) {
            powerUps[i].y += powerUps[i].dy;
            
            if (powerUps[i].y > SCREEN_HEIGHT) 
                powerUps[i].active = false;
                
            float dist = sqrt(pow(player.x - powerUps[i].x, 2) + pow(player.y - powerUps[i].y, 2));
            if (dist < player.radius + 8) {
                powerUps[i].active = false;
                
                switch (powerUps[i].type) {
                    case 1: 
                        player.fireRateBoostTimer = 10.0; 
                        player.currentFruitType = 1; // Chu?i
                        break; 
                    case 2: 
                        player.speedBoostTimer = 10.0; 
                        break;
                    case 3: 
                        if (player.lives < 5) player.lives++; 
                        break;
                    case 4: 
                        player.shieldTimer = 5.0; 
                        break;
                    case 5: 
                        player.damageBoostTimer = 10.0; 
                        player.currentFruitType = 5; // Táo
                        break; 
                    case 6: 
                        player.bulletSizeTimer = 10.0; 
                        break;
                    case 7: // Ultimate Charge (Cu là 8)
                        player.ultimateTimer = ULTIMATE_COOLDOWN * 0.5; 
                        break;
					case 8: // Companion Boost (Nh?t l?n 1 ra trái, l?n 2 ra ph?i)
                        player.companionBoostTimer = 10.0; // Luôn reset th?i gian v? 10 giây
                        
                        // Ki?m tra d? b?t t?ng chi?c m?t
                        if (!companions[1].active) {
                            companions[1].active = true; // Nh?t l?n 1: B?t tàu bên trái
                        } 
                        else if (!companions[2].active) {
                            companions[2].active = true; // Nh?t l?n 2: B?t n?t tàu bên ph?i
                        }
                        // Nh?t l?n 3 tr? di thì ch? h?i th?i gian (vì t?i da 2 tàu)
                        break;
                }
            }
        }
    }
}

// T?o v? n?
void createExplosion(float x, float y) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].x = x;
            particles[i].y = y;
            float angle = rand() % 360 * PI / 180;
            float speed = (rand() % 3) + 1;
            particles[i].dx = speed * cos(angle);
            particles[i].dy = speed * sin(angle);
            particles[i].life = 30;
            particles[i].active = true;
            break;
        }
    }
}

// Kích ho?t chiêu cu?i
void triggerUltimate() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            float dist = sqrt(pow(player.x - enemies[i].x, 2) + pow(player.y - enemies[i].y, 2));
            if (dist < 100) {
                enemies[i].active = false;
                int points = (enemies[i].type <= 5) ? 10 + enemies[i].type * 5 : 
                             (enemies[i].type <= 10) ? 15 + enemies[i].type * 3 : 
                             (enemies[i].type <= 15) ? 20 + enemies[i].type * 2 : 
                             (enemies[i].type == 16) ? 100 : (enemies[i].type == 17) ? 200 : 
                             (enemies[i].type == 18) ? 300 : 500;
                score += points;
                if (enemies[i].type >= 16) {
                    bossActive = false;
                    postBossDifficulty++;
                }
                if (enemies[i].type == 19) gameWon = true;
                createExplosion(enemies[i].x, enemies[i].y);
                spawnPowerUp(enemies[i].x, enemies[i].y);
            }
        }
    }
    for (int i = 0; i < 20; i++) {
        createExplosion(player.x + (rand() % 50 - 25), player.y + (rand() % 50 - 25));
    }
    setcolor(LIGHTCYAN);
    circle(player.x, player.y, 100);
}

// C?p nh?t h?t v? n?
void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].dx;
            particles[i].y += particles[i].dy;
            particles[i].life--;
            if (particles[i].life <= 0) particles[i].active = false;
        }
    }
}

// Ki?m tra va ch?m
void checkCollisions() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active) {
                    float dist = sqrt(pow(bullets[i].x - enemies[j].x, 2) + 
                                      pow(bullets[i].y - enemies[j].y, 2));
                    float bulletRadius = player.bulletSizeTimer > 0 ? currentBulletSize * 1.5 : currentBulletSize;
                    if (dist < enemies[j].radius + bulletRadius) {
                        bullets[i].active = false;
                        enemies[j].health -= bullets[i].highDamage ? 2 : 1;
                        if (enemies[j].health <= 0) {
                            enemies[j].active = false;
                            int points = (enemies[j].type <= 5) ? 10 + enemies[j].type * 5 : 
                                         (enemies[j].type <= 10) ? 15 + enemies[j].type * 3 : 
                                         (enemies[j].type <= 15) ? 20 + enemies[j].type * 2 : 
                                         (enemies[j].type == 16) ? 100 : (enemies[j].type == 17) ? 200 : 
                                         (enemies[j].type == 18) ? 300 : 500;
                            score += points;
                            if (enemies[j].type >= 16) {
                                bossActive = false;
                                postBossDifficulty++;
                            }
                            if (enemies[j].type == 19) gameWon = true;
                            createExplosion(enemies[j].x, enemies[j].y);
                            spawnPowerUp(enemies[j].x, enemies[j].y);
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && player.shieldTimer <= 0) {
            float dist = sqrt(pow(player.x - enemies[i].x, 2) + pow(player.y - enemies[i].y, 2));
            if (dist < player.radius + enemies[i].radius) {
                player.lives--;
                if (player.lives <= 0) gameOver = true;
                enemies[i].active = false;
                if (enemies[i].type >= 16) {
                    bossActive = false;
                    postBossDifficulty++;
                }
                createExplosion(enemies[i].x, enemies[i].y);
            }
        }
    }
    if (score >= 2000 && difficultyLevel < 4) {
        difficultyLevel = 4;
        player.doubleShot = true;
        companions[1].active = true;
        companions[2].active = true;
    } else if (score >= 1500 && difficultyLevel < 3) {
        difficultyLevel = 3;
        player.doubleShot = true;
        companions[1].active = true;
        companions[2].active = true;
    } else if (score >= 1000 && difficultyLevel < 2) {
        difficultyLevel = 2;
        companions[1].active = true;
        companions[2].active = true;
    } else if (score >= 500 && difficultyLevel < 1) {
        difficultyLevel = 1;
        companions[1].active = true;
    }
    if ((score >= 500 && !bossActive && difficultyLevel == 1) ||
        (score >= 1000 && !bossActive && difficultyLevel == 2) ||
        (score >= 1500 && !bossActive && difficultyLevel == 3) ||
        (score >= 2000 && !bossActive && difficultyLevel == 4)) {
        spawnBoss();
    }
}

// V? giao di?n
void drawUI() {
    setcolor(LIGHTCYAN);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    outtextxy(10, 10, scoreText); // Bi?n char m?ng truy?n vào bình thu?ng
    
    char livesText[20];
    sprintf(livesText, "Mang: %d", player.lives);
    outtextxy(10, 40, livesText);
    
    setcolor(player.ultimateTimer >= ULTIMATE_COOLDOWN ? LIGHTCYAN : LIGHTGRAY);
    rectangle(10, 70, 110, 90);
    setfillstyle(SOLID_FILL, player.ultimateTimer >= ULTIMATE_COOLDOWN ? LIGHTCYAN : LIGHTGRAY);
    float barProgress = player.ultimateTimer < ULTIMATE_COOLDOWN ? player.ultimateTimer : ULTIMATE_COOLDOWN;
    bar(12, 72, 12 + (barProgress / ULTIMATE_COOLDOWN) * 96, 88);

    char levelText[30];
    sprintf(levelText, "Moc: %d", difficultyLevel);
    outtextxy(10, 100, levelText);

    if (gameOver) {
        setcolor(LIGHTRED);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 3);
        // THÊM (char*) VÀO TRU?C CÁC CHU?I C? Ð?NH
        outtextxy(SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, (char*)"Ket Thuc!");
        
        char finalScore[30];
        sprintf(finalScore, "Diem Cuoi: %d", score);
        outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 10, finalScore);
        
        outtextxy(SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 + 50, (char*)"Nhan R de Choi Lai");
    } else if (gameWon) {
        setcolor(LIGHTGREEN);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 3);
        
        outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 30, (char*)"Chien Thang!");
        
        char finalScore[30];
        sprintf(finalScore, "Diem Cuoi: %d", score);
        outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 10, finalScore);
        
        outtextxy(SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 + 50, (char*)"Nhan R de Choi Lai");
    }
}


// Hàm chính
int main() {
    // Kh?i t?o d? h?a v?i c?a s? chu?n thay vì fullscreen m?c d?nh c?a BGI
    initwindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter Game");
    srand(time(NULL));

    initGame();

    int page = 0; // Ph?c v? cho Double Buffering (ch?ng nháy màn hình)

    while (1) {
        setactivepage(page); // V? lên trang ?n

        if ((gameOver || gameWon) && GetAsyncKeyState('R') & 0x8000) {
            initGame();
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        updateStars();
        updatePlayer();
        updateCompanions();
        updateBullets();
        spawnEnemy();
        updateEnemies();
        updatePowerUps();
        updateParticles();
        checkCollisions();

        drawBackground(); // Ðã ch?a cleardevice()
        drawParticles();
        drawBullets();
        drawEnemies();
        drawPowerUps();
        drawPlayer();
        drawCompanions();
        drawUI();

        setvisualpage(page); // Hi?n th? trang v?a v? xong
        page = 1 - page;     // L?t trang

        delay(20);
    }

    closegraph();
    return 0;
}
