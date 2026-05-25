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

// DANG KY HANG SO (DEFINES)
#define MAX_COMPANIONS 5
#define MAX_BULLETS 150
#define MAX_ENEMIES 30
#define MAX_PARTICLES 400
#define MAX_STARS 100
#define MAX_POWERUPS 10
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 10
#define BULLET_SPEED 17
#define SHOOT_DELAY 150
#define ULTIMATE_COOLDOWN 5
#define PI 3.14159265358979323846

// CAU TRUC DU LIEU (STRUCTS)
typedef struct {
    float x, y;
    float dx, dy;
    int radius;
    float lastAngle;
    int lives;
    
    // Thoi gian hieu luc cua buff
    float speedBoostTimer;       
    float fireRateBoostTimer;    
    float shieldTimer;           
    float damageBoostTimer;      
    float bulletSizeTimer;       
    float ultimateTimer;         
    float companionBoostTimer;   
    float invincibilityTimer;
    
    // Trang thai vu khi
    bool doubleShot;             
    int currentFruitType;        // 0: Dan thuong, 1: Chuoi, 5: Tao
    float laserTimer;
} Player;

typedef struct {
    float x, y;
    float lastAngle;
    int radius;
    bool active;
} Companion;

typedef struct {
    float x, y;
    float dx, dy;
    bool active;
    bool highDamage;
    bool isEnemy;
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
    int maxLife;     
    int color;       
    float size;      
    int type;        // 0: Tia lua, 1: Song xung kich (Shockwave)
    bool active;
} Particle;

typedef struct {
    float x, y;
    float speed;
    int radius;
} Star;

// BIEN TOAN CUC (GLOBALS)
float levelTransitionTimer = 0;
int nextBossScore = 500;
Player player;
Companion companions[MAX_COMPANIONS];
Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
PowerUp powerUps[MAX_POWERUPS];
Particle particles[MAX_PARTICLES];
Star stars[MAX_STARS];

int score = 0;
bool gameOver = false;
clock_t lastShotTime = 0;
int currentShootDelay = SHOOT_DELAY;
float currentPlayerSpeed = PLAYER_SPEED;
float currentBulletSize = 5;
int difficultyLevel = 0;
bool bossActive = false;
int postBossDifficulty = 0;
bool isLaserSoundPlaying = false; 

// KHAI BAO HAM (FUNCTION PROTOTYPES)
void initGame();
void updateStars();
void updatePlayer();
void updateCompanions();
void updateBullets();
void updateEnemies();
void updatePowerUps();
void updateParticles();
void checkCollisions();
void spawnEnemy();
void spawnBoss();
void spawnPowerUp(float x, float y);
void shootBullet();
void triggerUltimate();
void createExplosion(float x, float y);

void drawBackground();
void drawPlayer();
void drawCompanions();
void drawBullets();
void drawEnemies();
void drawPowerUps();
void drawParticles();
void drawLaser();
void drawUI();

void midpointCircle(int xc, int yc, int r, int color);
void midpointLine(int x1, int y1, int x2, int y2, int color);

void bresenhamLine(int x1, int y1, int x2, int y2, int color);
void bresenhamCircle(int xc, int yc, int r, int color);

void recursiveBoundaryFill(int x, int y, int fill_color, int boundary_color);

void drawKochLine(float x1, float y1, float x2, float y2, int iter, int color);
void drawKochSnowflake(int x, int y, int radius, int iter, float angle, int color);

void playExplosionSound();
void showInstructions();

void showInstructions() {
    int page = 0;
    
    while (GetAsyncKeyState(VK_RETURN) & 0x8000) {}

    while (1) {
        setactivepage(page);
        cleardevice();

        updateStars();
        drawBackground();

        setcolor(YELLOW);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 5);
        outtextxy(SCREEN_WIDTH / 2 - 340, 120, "CHIEN HAM NGAN HA");

        setcolor(WHITE);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
        outtextxy(SCREEN_WIDTH / 2 - 120, 230, "HUONG DAN CHOI:");

        setcolor(LIGHTCYAN);
        outtextxy(SCREEN_WIDTH / 2 - 250, 280, "- Phim A / D: Di chuyen trai / phai");
        outtextxy(SCREEN_WIDTH / 2 - 250, 320, "- Chuot Trai: Ban dan");
        outtextxy(SCREEN_WIDTH / 2 - 250, 360, "- Chuot Phai: Kich hoat Ky nang (Laser)");
        outtextxy(SCREEN_WIDTH / 2 - 250, 400, "- An cac vat pham de nang cap vu khi");

        // T?o hi?u ?ng ch? nh?p nháy (chu k? m?i 500ms)
        if ((clock() / 500) % 2 == 0) {
            setcolor(LIGHTGREEN);
            outtextxy(SCREEN_WIDTH / 2 - 175, 480, ">> NHAN ENTER DE BAT DAU <<");
        }

        setvisualpage(page);
        page = 1 - page;

        // N?u ngu?i choi nh?n Enter (Mã phím: VK_RETURN) thì thoát vòng l?p
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            break; 
        }
        delay(20);
    }
}
// Ham kiem soat am thanh no (Tranh lag chong cheo)
void playExplosionSound() {
    static clock_t lastExplosionTime = 0;
    clock_t now = clock();
    
    // Chi cho phep phat am thanh no moi 100 mili-giay (0.1 giay)
    if ((now - lastExplosionTime) * 1000 / CLOCKS_PER_SEC >= 100) {
        mciSendString("seek vuno_sound to start", NULL, 0, NULL);
        mciSendString("play vuno_sound", NULL, 0, NULL);
        lastExplosionTime = now;
    }
}

// LOGIC KHOI TAO GAME
void initGame() {
    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT - 35;
    player.dx = 0;
    player.dy = 0;
    player.radius = 15;
    player.lastAngle = -PI / 2; 
    player.lives = 3;
    player.speedBoostTimer = 0;
    player.fireRateBoostTimer = 0;
    player.ultimateTimer = 0;
    player.doubleShot = false;
    player.shieldTimer = 0;
    player.damageBoostTimer = 0;
    player.bulletSizeTimer = 0;
    player.companionBoostTimer = 0;
    player.currentFruitType = 0; 
    player.laserTimer = 0;
    player.invincibilityTimer = 0;

    // Khoi tao phi thuyen de (Companions)
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        companions[i].active = false;
        companions[i].radius = 10;
        companions[i].lastAngle = -PI / 2;
    }

    // Khoi tao mang du lieu
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
        bullets[i].highDamage = false;
        bullets[i].isEnemy = false;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_POWERUPS; i++) powerUps[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
    
    // Khoi tao sao bang lam nen
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = rand() % SCREEN_WIDTH;
        stars[i].y = rand() % SCREEN_HEIGHT;
        stars[i].radius = rand() % 2 + 1;
        stars[i].speed = (rand() % 5 + 1) * 0.1;
    }

    // Reset thong so diem & level
    score = 0;
    gameOver = false;
    lastShotTime = 0;
    currentShootDelay = SHOOT_DELAY;
    currentPlayerSpeed = PLAYER_SPEED;
    currentBulletSize = 5;
    difficultyLevel = 0;
    bossActive = false;
    postBossDifficulty = 0;
    levelTransitionTimer = 0;
    nextBossScore = 500;

    // Bat nhac nen & dong cac tap am thanh cu
    mciSendString("close bgm", NULL, 0, NULL); 
    mciSendString("open \"nhacnen.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
    mciSendString("close laser_sound", NULL, 0, NULL); 
    mciSendString("open \"laser.wav\" type mpegvideo alias laser_sound", NULL, 0, NULL);
    
    mciSendString("close vuno_sound", NULL, 0, NULL); 
    mciSendString("open \"vuno.wav\" type waveaudio alias vuno_sound", NULL, 0, NULL);
}

// CHUC NANG CAP NHAT (UPDATE LOGIC)
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
    // Co dinh vi tri Y & huong ban
    player.lastAngle = -PI / 2;  
    player.y = SCREEN_HEIGHT - player.radius - 20; 
    player.dy = 0;

    // Dieu khien ngang (Su dung ma Hex de tranh loi Unikey)
    player.dx = 0; 
    if (GetAsyncKeyState(0x41) & 0x8000) player.dx -= currentPlayerSpeed; // Phim A
    if (GetAsyncKeyState(0x44) & 0x8000) player.dx += currentPlayerSpeed; // Phim D

    // Cap nhat toa do
    player.x += player.dx; 

    // Gioi han man hinh
    if (player.x < player.radius) player.x = player.radius;
    if (player.x > SCREEN_WIDTH - player.radius) player.x = SCREEN_WIDTH - player.radius;

    // Ban dan (Chuot trai)
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        clock_t now = clock();
        if ((now - lastShotTime) * 1000 / CLOCKS_PER_SEC >= currentShootDelay) {
            shootBullet();      
            lastShotTime = now;
        }
    }

    // He thong Ultimate (Tia Laser)
    if (player.laserTimer <= 0) {
        if (player.ultimateTimer < ULTIMATE_COOLDOWN) {
            player.ultimateTimer += 0.015;
        }
        if (isLaserSoundPlaying) {
            mciSendString("stop laser_sound", NULL, 0, NULL);
            mciSendString("seek laser_sound to start", NULL, 0, NULL); 
            isLaserSoundPlaying = false;
        }
    } else {
        player.ultimateTimer -= 0.05; 
        if (!isLaserSoundPlaying) {
            mciSendString("play laser_sound repeat", NULL, 0, NULL);
            isLaserSoundPlaying = true;
        }
        if (player.ultimateTimer <= 0) {
            player.ultimateTimer = 0;
            player.laserTimer = 0; 
        }
    }

    // Kich hoat khi nhan chuot phai
    if (ismouseclick(WM_RBUTTONDOWN)) {
        clearmouseclick(WM_RBUTTONDOWN);
        if (player.ultimateTimer >= ULTIMATE_COOLDOWN) {
            player.laserTimer = 1; 
            triggerUltimate();
        }
    }
    
    // Giam thoi gian bat tu
    if (player.invincibilityTimer > 0) player.invincibilityTimer -= 0.02; 
    
    // Xu ly Buff / Power-up
    if (player.speedBoostTimer > 0) {
        currentPlayerSpeed = 15;
        player.speedBoostTimer -= 0.02;
        if (player.speedBoostTimer <= 0) currentPlayerSpeed = PLAYER_SPEED;
    }

    if (player.fireRateBoostTimer > 0) {
        currentShootDelay = 100;
        player.fireRateBoostTimer -= 0.02;
        if (player.fireRateBoostTimer <= 0) currentShootDelay = SHOOT_DELAY;
    }

    if (player.bulletSizeTimer > 0) {
        currentBulletSize = 7.5;
        player.bulletSizeTimer -= 0.02;
        if (player.bulletSizeTimer <= 0) currentBulletSize = 5;
    }

    if (player.shieldTimer > 0) player.shieldTimer -= 0.02;
    if (player.damageBoostTimer > 0) player.damageBoostTimer -= 0.02;

    // Buff may bay ho tro
    if (player.companionBoostTimer > 0) {
        player.companionBoostTimer -= 0.02; 
        updateCompanions(); 
    } else {
        companions[1].active = false;
        companions[2].active = false;
    }
}

void updateCompanions() {
    int sideOffsetX[2] = { -60, 60 }; 
    int sideOffsetY = 10; 

    if (companions[1].active) {
        companions[1].x = player.x + sideOffsetX[0];
        companions[1].y = player.y + sideOffsetY;
    }
    if (companions[2].active) {
        companions[2].x = player.x + sideOffsetX[1];
        companions[2].y = player.y + sideOffsetY;
    }
}

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

void updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            float dx = player.x - enemies[i].x;
            float dy = player.y - enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            float speed = 2.0 + difficultyLevel * 0.5 + postBossDifficulty * 0.3;

            // Xet toc do tuy thuoc vao loai quai
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

            // Huong di chuyen bam theo nguoi choi
            if (dist > 0) {
                enemies[i].dx = speed * dx / dist;
                enemies[i].dy = speed * dy / dist;
            }

            // Hanh dong dac biet
            if (enemies[i].type == 2 || enemies[i].type == 7 || enemies[i].type == 13) {
                enemies[i].zigzagTimer += 0.1;
                enemies[i].x += 5 * sin(enemies[i].zigzagTimer);
            }

            if (enemies[i].type == 10) { // Dodger - Ne dan
                for (int j = 0; j < MAX_BULLETS; j++) {
                    if (bullets[j].active && !bullets[j].isEnemy) {
                        float bulletDist = sqrt(pow(bullets[j].x - enemies[i].x, 2) + pow(bullets[j].y - enemies[i].y, 2));
                        if (bulletDist < 50) {
                            float angle = atan2(bullets[j].y - enemies[i].y, bullets[j].x - enemies[i].x);
                            enemies[i].dx -= 2 * cos(angle);
                            enemies[i].dy -= 2 * sin(angle);
                        }
                    }
                }
            }

            // Linh ban tia
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
                            bullets[j].isEnemy = true;
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

            // Pattern ban dan cua Boss
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
                                bullets[k].isEnemy = true;
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
                                    bullets[l].isEnemy = true;
                                    break;
                                }
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            if (enemies[i].type == 18) { // Boss Trieu hoi
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
                                bullets[k].isEnemy = true;
                                break;
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            // Cap nhat vi tri cuoi cung
            enemies[i].x += enemies[i].dx;
            enemies[i].y += enemies[i].dy;
            
            // Gioi han map
            if (enemies[i].x < enemies[i].radius) enemies[i].x = enemies[i].radius;
            if (enemies[i].x > SCREEN_WIDTH - enemies[i].radius) enemies[i].x = SCREEN_WIDTH - enemies[i].radius;
            if (enemies[i].y < enemies[i].radius) enemies[i].y = enemies[i].radius;
            
            // GIOI HAN TAM THAP CUA DICH
            int lowerLimitY = SCREEN_HEIGHT - 60; 
            if (enemies[i].y > lowerLimitY) {
                enemies[i].y = lowerLimitY;
            }
        }
    }
}

void updatePowerUps() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerUps[i].active) {
            powerUps[i].y += powerUps[i].dy;
            if (powerUps[i].y > SCREEN_HEIGHT) powerUps[i].active = false;
            
            float distPlayer = sqrt(pow(player.x - powerUps[i].x, 2) + pow(player.y - powerUps[i].y, 2));
            bool pickedUp = false;

            if (distPlayer < player.radius + 8) pickedUp = true;

            if (!pickedUp) {
                for (int j = 0; j < MAX_COMPANIONS; j++) {
                    if (companions[j].active) {
                        float distComp = sqrt(pow(companions[j].x - powerUps[i].x, 2) + pow(companions[j].y - powerUps[i].y, 2));
                        if (distComp < companions[j].radius + 8) {
                            pickedUp = true;
                            break; 
                        }
                    }
                }
            }

            if (pickedUp) {
                powerUps[i].active = false;
                createExplosion(powerUps[i].x, powerUps[i].y);

                switch (powerUps[i].type) {
                    case 1: 
                        player.fireRateBoostTimer = 10.0; 
                        player.currentFruitType = 1; 
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
                        player.currentFruitType = 5; 
                        break; 
                    case 6: 
                        player.bulletSizeTimer = 10.0; 
                        break;
                    case 7: 
                        player.ultimateTimer += ULTIMATE_COOLDOWN * 0.3; 
                        if (player.ultimateTimer > ULTIMATE_COOLDOWN) player.ultimateTimer = ULTIMATE_COOLDOWN;
                        break;
                    case 8: 
                        player.companionBoostTimer = 10.0; 
                        if (!companions[1].active) companions[1].active = true;
                        else if (!companions[2].active) companions[2].active = true;
                        break;
                }
            }
        }
    }
}

void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].dx;
            particles[i].y += particles[i].dy;
            
            if (particles[i].type == 1) {
                particles[i].size += 4.5; 
            } else {
                particles[i].dx *= 0.85;
                particles[i].dy *= 0.85;
            }

            particles[i].life--;
            if (particles[i].life <= 0) particles[i].active = false;
        }
    }
}

// KHOI LENH XU LY VA CHAM (COLLISIONS)
void checkCollisions() {
    // --- 1. XU LY SAT THUONG LASER ---
    if (player.laserTimer > 0) {
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j].active) {
                if (enemies[j].x > player.x - 25 && enemies[j].x < player.x + 25 && enemies[j].y < player.y) {
                    enemies[j].health -= 2; 
                    if (rand() % 3 == 0) createExplosion(enemies[j].x, enemies[j].y);

                    if (enemies[j].health <= 0) {
                        
                        enemies[j].active = false;
                        int points = (enemies[j].type <= 5) ? 10 : (enemies[j].type <= 10) ? 20 : 50;
                        score += points;
                        if (enemies[j].type >= 16) { bossActive = false; postBossDifficulty++; }
                        
                        createExplosion(enemies[j].x, enemies[j].y);
                        spawnPowerUp(enemies[j].x, enemies[j].y);
                    }
                }
            }
        }
    }

    // --- 2. XU LY VA CHAM DAN ---
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            
            // Truong hop 1: Dan nguoi choi ban trung dich
            if (!bullets[i].isEnemy) {
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    if (enemies[j].active) {
                        float dist = sqrt(pow(bullets[i].x - enemies[j].x, 2) + pow(bullets[i].y - enemies[j].y, 2));
                        float bulletRadius = player.bulletSizeTimer > 0 ? currentBulletSize * 1.5 : currentBulletSize;

                        if (dist < enemies[j].radius + bulletRadius) {
                            bullets[i].active = false; 
                            createExplosion(bullets[i].x, bullets[i].y);
                            enemies[j].health -= bullets[i].highDamage ? 2 : 1;

                            if (enemies[j].health <= 0) {
                                
                                enemies[j].active = false;
                                int points = (enemies[j].type <= 5) ? 10 + enemies[j].type * 5 : 
                                             (enemies[j].type <= 10) ? 15 + enemies[j].type * 3 : 20;
                                score += points;

                                if (enemies[j].type >= 16) { 
                                    bossActive = false; 
                                    postBossDifficulty++; 
                                }

                                for(int k = 0; k < 3; k++) 
                                    createExplosion(enemies[j].x + rand()%10-5, enemies[j].y + rand()%10-5);
                                
                                spawnPowerUp(enemies[j].x, enemies[j].y);
                            }
                        }
                    }
                }
            } 
            // Truong hop 2: Dan dich ban trung nguoi choi
            else {
                if (player.invincibilityTimer <= 0) { 
                    float dist = sqrt(pow(bullets[i].x - player.x, 2) + pow(bullets[i].y - player.y, 2));
                    if (dist < player.radius + 5) { 
                        bullets[i].active = false; 
                        createExplosion(bullets[i].x, bullets[i].y);
                        playExplosionSound();
                        
                        if (player.shieldTimer > 0) {
                            player.shieldTimer = 0;          
                            player.invincibilityTimer = 1.0; 
                        } else {
                            player.lives--;
                            player.invincibilityTimer = 2.0; 
                            if (player.lives <= 0) gameOver = true;
                        }
                    }
                }
            }
        }
    }

    // --- 3. VA CHAM NGUOI CHOI TONG VAO KE DICH ---
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && player.invincibilityTimer <= 0) {
            float dist = sqrt(pow(player.x - enemies[i].x, 2) + pow(player.y - enemies[i].y, 2));
            if (dist < player.radius + enemies[i].radius) {
                playExplosionSound();
                
                if (player.shieldTimer > 0) {
                    player.shieldTimer = 0;          
                    player.invincibilityTimer = 1.0; 
                } else {
                    player.lives--;
                    player.invincibilityTimer = 2.0; 
                    if (player.lives <= 0) gameOver = true;
                }
                
                enemies[i].active = false;
                if (enemies[i].type >= 16) { bossActive = false; postBossDifficulty++; }
                createExplosion(enemies[i].x, enemies[i].y);
            }
        }
    }

    // --- 4. LOGIC LEN CAP & GOI BOSS (CHE DO VO TAN) ---
    int oldLevel = difficultyLevel; 

    if (score >= 2000 && difficultyLevel < 4) {
        difficultyLevel = 4; player.doubleShot = true;
        companions[1].active = true; companions[2].active = true;
    } else if (score >= 1500 && difficultyLevel < 3) {
        difficultyLevel = 3; player.doubleShot = true;
        companions[1].active = true; companions[2].active = true;
    } else if (score >= 1000 && difficultyLevel < 2) {
        difficultyLevel = 2; companions[1].active = true; companions[2].active = true;
    } else if (score >= 500 && difficultyLevel < 1) {
        difficultyLevel = 1; companions[1].active = true;
    }

    // Ngu sinh linh 3 giay de thong bao
    if (difficultyLevel > oldLevel) {
        levelTransitionTimer = 3.0; 
    }

    // Boss se lien tuc xuat hien moi 500 diem
    if (!bossActive && levelTransitionTimer <= 0 && score >= nextBossScore) {
        spawnBoss();
        nextBossScore += 500; 
    }
}

// CAC HAM HO TRO (HELPERS: SINH DICH, BAN DAN, NO...)
void spawnEnemy() {
    if (levelTransitionTimer > 0) return;

    int spawnChance = 50 - difficultyLevel * 10 - postBossDifficulty * 5;
    if (spawnChance < 10) spawnChance = 10;
    if (rand() % spawnChance != 0) return;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = rand() % SCREEN_WIDTH;
            enemies[i].y = -20;   

            int randType = rand() % 100;
            int type;

            if (postBossDifficulty > 0) {
                if (randType < 10) type = 1; else if (randType < 20) type = 2;
                else if (randType < 30) type = 3; else if (randType < 40) type = 4;
                else if (randType < 50) type = 5; else if (randType < 60) type = 6;
                else if (randType < 70) type = 7; else if (randType < 80) type = 8;
                else if (randType < 85) type = 9; else if (randType < 90) type = 10;
                else if (randType < 93) type = 11; else if (randType < 95) type = 12;
                else if (randType < 97) type = 13; else if (randType < 99) type = 14;
                else type = 15;
            } else {
                if (randType < 20) type = 1; else if (randType < 35) type = 2;
                else if (randType < 50) type = 3; else if (randType < 60) type = 4;
                else if (randType < 70) type = 5; else if (randType < 75) type = 6;
                else if (randType < 80) type = 7; else if (randType < 85) type = 8;
                else if (randType < 90) type = 9; else if (randType < 92) type = 10;
                else if (randType < 94) type = 11; else if (randType < 96) type = 12;
                else if (randType < 98) type = 13; else if (randType < 99) type = 14;
                else type = 15;
            }

            enemies[i].type = type;
            enemies[i].radius = (type <= 5) ? (10 + type * 2) : (type <= 10) ? (12 + type) : (15 + type / 2);
            enemies[i].health = (type <= 5)  ? (1 + type / 4 + difficultyLevel / 2 + postBossDifficulty) :
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

void spawnBoss() {
    if (bossActive) return;
    int bossType = 0;
    
    // Giai doan diem sieu cao se ramdom Boss de tao thu thach
    if (score >= 2000) bossType = 16 + (rand() % 4); 
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

void spawnPowerUp(float x, float y) {
    if (rand() % 100 < 40) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (!powerUps[i].active) {
                powerUps[i].x = x;
                powerUps[i].y = y;
                powerUps[i].dy = 2.0;
                powerUps[i].type = rand() % 8 + 1; 
                powerUps[i].active = true;
                break;
            }
        }
    }
}

void shootBullet() {
    PlaySound(TEXT("ban.wav"), NULL, SND_FILENAME | SND_ASYNC);
    
    // Dan cua ban than (Player)
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = player.x;
            bullets[i].y = player.y - player.radius;
            bullets[i].dx = 0;
            bullets[i].dy = -BULLET_SPEED;
            bullets[i].highDamage = player.damageBoostTimer > 0;
            bullets[i].isEnemy = false; 
            break;
        }
    }

    // Dan cua De (Companions)
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (!companions[i].active) continue;

        for (int j = 0; j < MAX_BULLETS; j++) {
            if (!bullets[j].active) {
                bullets[j].active = true;
                bullets[j].x = companions[i].x;
                bullets[j].y = companions[i].y - companions[i].radius;
                bullets[j].dx = 0;
                bullets[j].dy = -BULLET_SPEED;
                bullets[j].highDamage = player.damageBoostTimer > 0;
                bullets[j].isEnemy = false; 
                break;
            }
        }
    }
}

void triggerUltimate() {
    for (int i = 0; i < 15; i++) {
        createExplosion(player.x, player.y - player.radius);
    }
}

void createExplosion(float x, float y) {
    // 1. Tao vong song xung kich
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].dx = 0;
            particles[i].dy = 0;
            particles[i].life = 12;      
            particles[i].maxLife = 12;
            particles[i].size = 2;       
            particles[i].type = 1;       
            particles[i].active = true;
            break;
        }
    }

    // 2. Tao bui tia lua vang tu tung
    int numSparks = 10 + rand() % 6; 
    for (int k = 0; k < numSparks; k++) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                particles[i].x = x;
                particles[i].y = y;
                
                float angle = (rand() % 360) * PI / 180.0;
                float speed = (rand() % 60) / 10.0 + 2.0; 
                
                particles[i].dx = speed * cos(angle);
                particles[i].dy = speed * sin(angle);
                particles[i].life = 15 + rand() % 15;
                particles[i].maxLife = particles[i].life;
                particles[i].size = (rand() % 3) + 2; 
                
                int col = rand() % 4;
                particles[i].color = (col == 0) ? WHITE : (col == 1) ? YELLOW : (col == 2) ? LIGHTRED : RED;
                
                particles[i].type = 0; 
                particles[i].active = true;
                break;
            }
        }
    }
}

// CAC HAM DO HOA (DRAWING)
void drawBackground() {
    cleardevice();
    
    for (int i = 0; i < MAX_STARS; i++) {
        int twinkleChance = rand() % 100;
        int starColor;
        
        if (twinkleChance < 15) {
            starColor = DARKGRAY;
        } else if (twinkleChance < 40) {
            starColor = LIGHTGRAY;
        } else {
            starColor = WHITE;
        }
        
        setcolor(starColor);
        setfillstyle(SOLID_FILL, starColor);
        
        fillellipse(stars[i].x, stars[i].y, stars[i].radius, stars[i].radius);
    }
}

// Thuat toan ve duong tron Midpoint
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

void midpointLine(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    
    int isSwap = 0;
    // Neu do doc > 45 do, hoan vi dx va dy de dam bao dx luon lon nhat
    if (dy > dx) {
        int temp = dx; dx = dy; dy = temp;
        isSwap = 1;
    }
    
    // Cac bien quyet dinh cua Midpoint
    int d = 2 * dy - dx;
    int incE = 2 * dy;
    int incNE = 2 * (dy - dx);
    
    int x = x1, y = y1;
    putpixel(x, y, color);
    
    for (int i = 1; i <= dx; i++) {
        if (d < 0) {
            d += incE;
            if (isSwap) y += sy; // Neu da hoan vi, bien di chuyen doc lap la y
            else x += sx;
        } else {
            d += incNE;
            x += sx;
            y += sy;
        }
        putpixel(x, y, color);
    }
}

// Thuat toan Boundary Fill de to mau
void recursiveBoundaryFill(int x, int y, int fill_color, int boundary_color) {
    // 1. Kiem tra dieu kien dung: Toa do nam ngoai gioi han man hinh
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    // 2. Lay ma mau cua diem anh (pixel) hien tai
    int current_color = getpixel(x, y);

    // 3. Kiem tra dieu kien to: Chua cham vao duong bien va chua duoc to mau nay
    if (current_color != boundary_color && current_color != fill_color) {
        
        // To mau cho pixel hien tai
        putpixel(x, y, fill_color); 

        // 4. Goi de quy lan toa ra 4 huong lan can (Pixel luy tuyen)
        recursiveBoundaryFill(x + 1, y, fill_color, boundary_color); // Sang phai
        recursiveBoundaryFill(x - 1, y, fill_color, boundary_color); // Sang trai
        recursiveBoundaryFill(x, y + 1, fill_color, boundary_color); // Xuong duoi
        recursiveBoundaryFill(x, y - 1, fill_color, boundary_color); // Len tren
    }
}
// Thuat toan ve duong thang Bresenham
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

void bresenhamCircle(int xc, int yc, int r, int color) {
    int x = 0, y = r;
    int d = 3 - 2 * r; // Bien quyet dinh cua Bresenham

    while (y >= x) {
        // Ve 8 diem doi xung
        putpixel(xc + x, yc + y, color);
        putpixel(xc - x, yc + y, color);
        putpixel(xc + x, yc - y, color);
        putpixel(xc - x, yc - y, color);
        putpixel(xc + y, yc + x, color);
        putpixel(xc - y, yc + x, color);
        putpixel(xc + y, yc - x, color);
        putpixel(xc - y, yc - x, color);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}
// Thuat toan de quy ve duong cong Koch
void drawKochLine(float x1, float y1, float x2, float y2, int iter, int color) {
    if (iter == 0) {
    	// Ap dung thuat toan BresenhamLine
        bresenhamLine((int)x1, (int)y1, (int)x2, (int)y2, color);
    } else {
        float dx = (x2 - x1) / 3.0f;
        float dy = (y2 - y1) / 3.0f;

        float p1x = x1 + dx;
        float p1y = y1 + dy;

        float p2x = x1 + 2 * dx;
        float p2y = y1 + 2 * dy;

        float px = p1x + dx * cos(PI / 3) + dy * sin(PI / 3);
        float py = p1y - dx * sin(PI / 3) + dy * cos(PI / 3);
		
        drawKochLine(x1, y1, p1x, p1y, iter - 1, color);
        drawKochLine(p1x, p1y, px, py, iter - 1, color);
        drawKochLine(px, py, p2x, p2y, iter - 1, color);
        drawKochLine(p2x, p2y, x2, y2, iter - 1, color);
    }
}

// Ghep 3 duong Koch thanh hinh Bong tuyet xoay
void drawKochSnowflake(int x, int y, int radius, int iter, float angle, int color) {
    // Tinh toan 3 dinh cua tam giac deu duoc xoay theo goc 'angle'
    float p1x = x + radius * cos(angle - PI / 2);
    float p1y = y + radius * sin(angle - PI / 2);
    float p2x = x + radius * cos(angle + PI / 6);
    float p2y = y + radius * sin(angle + PI / 6);
    float p3x = x + radius * cos(angle + 5 * PI / 6);
    float p3y = y + radius * sin(angle + 5 * PI / 6);

    // Ve 3 canh bang duong cong Koch
    // Ap dung thuat toan ve duong cong Koch
    drawKochLine(p1x, p1y, p2x, p2y, iter, color);
    drawKochLine(p2x, p2y, p3x, p3y, iter, color);
    drawKochLine(p3x, p3y, p1x, p1y, iter, color);
}
void drawPlayer() {
    // Hieu ung nhap nhay khi bat tu
    if (player.invincibilityTimer > 0) {
        if ((int)(player.invincibilityTimer * 15) % 2 == 0) {
            // Van ve khien neu co
            if (player.shieldTimer > 0) {
            	// Ap dung thuat toan MidpointCircle
                midpointCircle(player.x, player.y, player.radius + 10, LIGHTBLUE);
            }
            return; // Khong ve tau bay de tao hieu ung chop tat
        }
    }

    int x = player.x;
    int y = player.y;
    int r = player.radius;

    const int BODY_FILL_COLOR = CYAN;
    const int LINE_COLOR = LIGHTCYAN;
    const int COCKPIT_FILL_COLOR = BLUE; 
    const int POD_FILL_COLOR = LIGHTCYAN; 

    setcolor(LINE_COLOR); 
    setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

    // 1. Than chinh & Mui
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

    // 2. Canh tam giac
    int left_wing[] = {
        x - (int)(r * 0.3), y - (int)(r * 0.3), 
        x - (int)(r * 1.5), y + (int)(r * 0.2), 
        x - (int)(r * 1.5), y + (int)(r * 0.9), 
        x - (int)(r * 0.3), y + (int)(r * 0.7), 
        x - (int)(r * 0.3), y - (int)(r * 0.3)  
    };
    fillpoly(5, left_wing);

    int right_wing[] = {
        x + (int)(r * 0.3), y - (int)(r * 0.3), 
        x + (int)(r * 1.5), y + (int)(r * 0.2), 
        x + (int)(r * 1.5), y + (int)(r * 0.9), 
        x + (int)(r * 0.3), y + (int)(r * 0.7), 
        x + (int)(r * 0.3), y - (int)(r * 0.3)  
    };
    fillpoly(5, right_wing);

    // Duong phan tach
    // Ap dung thuat toan MidpointLine
    midpointLine(x - (int)(r * 0.4), y - (int)(r * 0.1), x - (int)(r * 1.4), y + (int)(r * 0.3), BLUE);
    midpointLine(x - (int)(r * 0.5), y + (int)(r * 0.1), x - (int)(r * 1.3), y + (int)(r * 0.4), BLUE);
    midpointLine(x - (int)(r * 0.6), y + (int)(r * 0.3), x - (int)(r * 1.2), y + (int)(r * 0.5), BLUE);
    midpointLine(x + (int)(r * 0.4), y - (int)(r * 0.1), x + (int)(r * 1.4), y + (int)(r * 0.3), BLUE);
    midpointLine(x + (int)(r * 0.5), y + (int)(r * 0.1), x + (int)(r * 1.3), y + (int)(r * 0.4), BLUE);
    midpointLine(x + (int)(r * 0.6), y + (int)(r * 0.3), x + (int)(r * 1.2), y + (int)(r * 0.5), BLUE);
    midpointLine(x - (int)(r * 0.2), y - (int)(r * 1.2), x + (int)(r * 0.2), y - (int)(r * 1.2), BLUE);
    midpointLine(x - (int)(r * 0.1), y - (int)(r * 1.0), x + (int)(r * 0.1), y - (int)(r * 1.0), BLUE);
    midpointLine(x - (int)(r * 0.1), y - (int)(r * 0.5), x + (int)(r * 0.1), y - (int)(r * 0.5), BLUE);

    // 3. Buong lai
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

    // 4. Duoi va ong xa phia sau
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

    bar(x - (int)(r * 0.15), y + (int)(r * 1.2), x - (int)(r * 0.05), y + (int)(r * 1.3));
    bar(x + (int)(r * 0.05), y + (int)(r * 1.2), x + (int)(r * 0.15), y + (int)(r * 1.3));

    // 5. Vu khi duoi canh
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
	// Ap dung thuat toan to mau de quy
    recursiveBoundaryFill(x - (int)(r * 0.75), y + (int)(r * 0.6), LIGHTCYAN, LINE_COLOR);

    // 6. Hieu ung lua dong co (Animated exhaust fire)
    int flameLenL = (int)(r * 0.4) + rand() % (int)(r * 0.4 + 1);
    int flameLenR = (int)(r * 0.4) + rand() % (int)(r * 0.4 + 1);

    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, LIGHTRED);
    int fire_left_outer[] = {
        x - (int)(r * 0.15), y + (int)(r * 1.3),
        x - (int)(r * 0.10), y + (int)(r * 1.3) + flameLenL, 
        x - (int)(r * 0.05), y + (int)(r * 1.3),
        x - (int)(r * 0.15), y + (int)(r * 1.3)              
    };
    fillpoly(4, fire_left_outer);

    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, YELLOW);
    int fire_left_inner[] = {
        x - (int)(r * 0.13), y + (int)(r * 1.3),
        x - (int)(r * 0.10), y + (int)(r * 1.3) + (int)(flameLenL * 0.6), 
        x - (int)(r * 0.07), y + (int)(r * 1.3),
        x - (int)(r * 0.13), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_left_inner);

    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, LIGHTRED);
    int fire_right_outer[] = {
        x + (int)(r * 0.05), y + (int)(r * 1.3),
        x + (int)(r * 0.10), y + (int)(r * 1.3) + flameLenR,
        x + (int)(r * 0.15), y + (int)(r * 1.3),
        x + (int)(r * 0.05), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_right_outer);

    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, YELLOW);
    int fire_right_inner[] = {
        x + (int)(r * 0.07), y + (int)(r * 1.3),
        x + (int)(r * 0.10), y + (int)(r * 1.3) + (int)(flameLenR * 0.6),
        x + (int)(r * 0.13), y + (int)(r * 1.3),
        x + (int)(r * 0.07), y + (int)(r * 1.3)
    };
    fillpoly(4, fire_right_inner);

    // 7. Khien (Su dung thuat toan Midpoint Circle)
    if (player.shieldTimer > 0) {
        midpointCircle(x, y, r + 10, LIGHTBLUE);
    }
}

void drawCompanions() {
    for (int i = 0; i < MAX_COMPANIONS; i++) {
        if (!companions[i].active) continue;

        int x = companions[i].x;
        int y = companions[i].y;
        int r = companions[i].radius; 

        const int BODY_FILL_COLOR = GREEN;
        const int LINE_COLOR = LIGHTGREEN;
        const int COCKPIT_FILL_COLOR = DARKGRAY; 
        const int POD_FILL_COLOR = LIGHTGREEN;

        setcolor(LINE_COLOR);
        setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

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

        int left_wing[] = {
            x - (int)(r * 0.3), y - (int)(r * 0.3), 
            x - (int)(r * 1.5), y + (int)(r * 0.2), 
            x - (int)(r * 1.5), y + (int)(r * 0.9), 
            x - (int)(r * 0.3), y + (int)(r * 0.7), 
            x - (int)(r * 0.3), y - (int)(r * 0.3)  
        };
        fillpoly(5, left_wing);

        int right_wing[] = {
            x + (int)(r * 0.3), y - (int)(r * 0.3), 
            x + (int)(r * 1.5), y + (int)(r * 0.2), 
            x + (int)(r * 1.5), y + (int)(r * 0.9), 
            x + (int)(r * 0.3), y + (int)(r * 0.7), 
            x + (int)(r * 0.3), y - (int)(r * 0.3)  
        };
        fillpoly(5, right_wing);

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

        bar(x - (int)(r * 0.15), y + (int)(r * 1.2), x - (int)(r * 0.05), y + (int)(r * 1.3));
        bar(x + (int)(r * 0.05), y + (int)(r * 1.2), x + (int)(r * 0.15), y + (int)(r * 1.3));

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

        int flameLen = (int)(r * 0.3) + rand() % ((int)(r * 0.2) + 1);
        
        setcolor(LIGHTRED);
        setfillstyle(SOLID_FILL, YELLOW);
        
        int fire_left[] = {
            x - (int)(r * 0.15), y + (int)(r * 1.3),
            x - (int)(r * 0.10), y + (int)(r * 1.3) + flameLen,
            x - (int)(r * 0.05), y + (int)(r * 1.3),
            x - (int)(r * 0.15), y + (int)(r * 1.3) 
        };
        fillpoly(4, fire_left);

        int fire_right[] = {
            x + (int)(r * 0.05), y + (int)(r * 1.3),
            x + (int)(r * 0.10), y + (int)(r * 1.3) + flameLen,
            x + (int)(r * 0.15), y + (int)(r * 1.3),
            x + (int)(r * 0.05), y + (int)(r * 1.3)
        };
        fillpoly(4, fire_right);
    }
}

void drawBullets() {
    if (player.currentFruitType == 1 && player.fireRateBoostTimer <= 0) player.currentFruitType = 0;
    if (player.currentFruitType == 5 && player.damageBoostTimer <= 0) player.currentFruitType = 0;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            int bx = bullets[i].x;
            int by = bullets[i].y;

            if (bullets[i].isEnemy) {
                // Dan dich ban ra (Tron, Do)
                int enemyBulletRadius = 5;
                setcolor(WHITE); 
                setfillstyle(SOLID_FILL, LIGHTRED); 
                fillellipse(bx, by, enemyBulletRadius, enemyBulletRadius);
                circle(bx, by, enemyBulletRadius);
            } 
            else {
                // Dan nguoi choi
                int radius = player.bulletSizeTimer > 0 ? currentBulletSize * 1.5 : currentBulletSize;

                if (player.currentFruitType == 1) { 
                    // Chuoi
                    setcolor(YELLOW);
                    setfillstyle(SOLID_FILL, YELLOW);
                    int w = radius * 2.0; 
                    int h = radius * 1.2; 
                    for (float t = -1.57; t <= 1.57; t += 0.15) {
                        int cx = bx + sin(t) * w;
                        int cy = by + cos(t) * h;
                        int r = (int)((radius * 0.9) * (1.0 - fabs(t) / 1.57));
                        if (r < 1) r = 1;
                        fillellipse(cx, cy, r, r);
                    }
                    setcolor(BROWN);
                    setfillstyle(SOLID_FILL, BROWN);
                    int numSize = radius / 3;
                    if (numSize < 1) numSize = 1;
                    fillellipse(bx + w, by, numSize, numSize);
                } 
                else if (player.currentFruitType == 5) { 
                    // Tao
                    int r_apple = radius * 1.6; 
                    setcolor(LIGHTRED);
                    setfillstyle(SOLID_FILL, LIGHTRED);
                    fillellipse(bx - r_apple/2 + 1, by, r_apple/2 + 2, r_apple); 
                    fillellipse(bx + r_apple/2 - 1, by, r_apple/2 + 2, r_apple); 
                    setcolor(BROWN);
                    line(bx, by - r_apple + 2, bx, by - r_apple - 8); 
                    setcolor(LIGHTGREEN);
                    setfillstyle(SOLID_FILL, LIGHTGREEN);
                    fillellipse(bx + r_apple/2 + 1, by - r_apple - 4, r_apple/2, r_apple/3 + 1); 
                } 
                else { 
                    // Dan thuong
                    setcolor(WHITE);
                    setfillstyle(SOLID_FILL, bullets[i].highDamage ? RED : YELLOW);
                    fillellipse(bx, by, radius, radius);
                }
            }
        }
    }
}

void drawEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        int size = enemies[i].radius;
        // Goc xoay huong ve phia may bay nguoi choi (dung cho cac quai biet nham muc tieu)
        float angle = atan2(
            player.y - enemies[i].y,
            player.x - enemies[i].x
        );

        switch (enemies[i].type) {

        case 1: // Drone - Thiet ke lai thanh vat the ngoai hanh tinh (Mat co khi)
        {
            int cx = enemies[i].x;
            int cy = enemies[i].y;
            int s = size;

            // 1. Ve cac xuc tu / ang-ten nang luong (ve truoc de nam duoi than)
            setcolor(LIGHTMAGENTA);
            line(cx, cy, cx - (int)(s * 1.2), cy - (int)(s * 0.8));
            line(cx, cy, cx + (int)(s * 1.2), cy - (int)(s * 0.8));
            line(cx, cy, cx - (int)(s * 0.8), cy + (int)(s * 1.2));
            line(cx, cy, cx + (int)(s * 0.8), cy + (int)(s * 1.2));

            // 2. Ve vo kim loai ben ngoai (Hinh thoi vuong vuc)
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, DARKGRAY);
            int alien_body[] = {
                cx, cy - s,          // Dinh tren
                cx + s, cy,          // Dinh phai
                cx, cy + s,          // Dinh duoi
                cx - s, cy,          // Dinh trai
                cx, cy - s           // Dong vong chot khoi
            };
            fillpoly(5, alien_body);

            // 3. Ve con mat sinh hoc o giua
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, (int)(s * 0.6), (int)(s * 0.6));

            // 4. Ve dong tu (Loi vang ke doc giong mat ran/quai vat)
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, (int)(s * 0.15), (int)(s * 0.4));
            
            // 5. Ve cham sang de tao do bong cho mat
            setcolor(WHITE);
            putpixel(cx - 2, cy - 2, WHITE);
            putpixel(cx - 3, cy - 2, WHITE);

            break;
        }

        case 2: // Scout - Mui ten sinh hoc (Biet bam duoi, sac nhon)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTGREEN);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // Khung xuong sac nhon huong ve phia nguoi choi
            int pts[] = {
                cx + (int)(s * 1.5 * cos(angle)), cy + (int)(s * 1.5 * sin(angle)),
                cx + (int)(s * cos(angle + 2.5)), cy + (int)(s * sin(angle + 2.5)),
                cx + (int)(s * 0.2 * cos(angle)), cy + (int)(s * 0.2 * sin(angle)), // Loi lom o duoi
                cx + (int)(s * cos(angle - 2.5)), cy + (int)(s * sin(angle - 2.5)),
                cx + (int)(s * 1.5 * cos(angle)), cy + (int)(s * 1.5 * sin(angle))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            // Mat doc nhan mau xanh luc o trung tam
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, GREEN);
            fillellipse(cx + (int)(s*0.3*cos(angle)), cy + (int)(s*0.3*sin(angle)), s/3, s/3);
            break;
        }

        case 3: // Tank - Bo hung boc thep (Cham chap, giap day, luc luong)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // Giap luc giac khong lo
            int pts[14];
            for (int j = 0; j < 6; j++) {
                pts[j * 2] = cx + s * cos(j * PI / 3 + PI/6);
                pts[j * 2 + 1] = cy + s * sin(j * PI / 3 + PI/6);
            }
            pts[12] = pts[0]; pts[13] = pts[1];
            fillpoly(7, pts);
            
            // Duong gan thep tren lung
            setcolor(LIGHTGRAY);
            line(cx - s, cy, cx + s, cy);
            line(cx, cy - s, cx, cy + s);

            // Loi lo phan ung nhiet hach o giua
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, s/2, s/2);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s/4, s/4);
            break;
        }

        case 4: // Chaser - Mong vuot tu than (Biet lao nhanh, 2 luoi hai lon)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTMAGENTA);
            
            // Ve 2 luoi hai sac ben chom ra phia truoc
            int claw1[] = {
                cx, cy, 
                cx + (int)(s*1.8 * cos(angle - 0.6)), cy + (int)(s*1.8 * sin(angle - 0.6)),
                cx + (int)(s*1.2 * cos(angle - 0.2)), cy + (int)(s*1.2 * sin(angle - 0.2)),
                cx, cy
            };
            int claw2[] = {
                cx, cy, 
                cx + (int)(s*1.8 * cos(angle + 0.6)), cy + (int)(s*1.8 * sin(angle + 0.6)),
                cx + (int)(s*1.2 * cos(angle + 0.2)), cy + (int)(s*1.2 * sin(angle + 0.2)),
                cx, cy
            };
            setfillstyle(SOLID_FILL, MAGENTA);
            fillpoly(4, claw1); fillpoly(4, claw2);
            
            // Khoi cau nao (brain) boc lo ra ngoai
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);
            fillellipse(cx, cy, s*0.7, s*0.7);
            break;
        }

        case 5: // Sniper - Con mat vien vong (Dung lai ban tia laser nham muc tieu)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // 4 manh vo ve tinh lo lung xung quanh loi
            setcolor(CYAN);
            for(int j = 0; j < 4; j++) {
                float a = angle + j * PI/2;
                line(cx + (int)(s*0.8*cos(a)), cy + (int)(s*0.8*sin(a)), 
                     cx + (int)(s*1.5*cos(a)), cy + (int)(s*1.5*sin(a)));
            }
            
            // Con mat laza khong lo, con nguoi huong ve player
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);
            int eye[] = {cx, cy-s, cx+s, cy, cx, cy+s, cx-s, cy, cx, cy-s};
            fillpoly(5, eye);
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, CYAN);
            fillellipse(cx + (int)(s*0.2*cos(angle)), cy + (int)(s*0.2*sin(angle)), s/3, s/3);
            break;
        }

        case 6: // Bomber - Tui bao tu doc (To, bau binh, san sang no)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(RED);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // Than hinh phi nop, phinh to
            fillellipse(cx, cy, s, s);
            
            // Nhung cuc buou (bao tu doc) noi len
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx - s/2, cy - s/3, s/3, s/3);
            fillellipse(cx + s/3, cy + s/2, s/4, s/4);
            fillellipse(cx + s/2, cy - s/4, s/3, s/3);
            fillellipse(cx - s/3, cy + s/3, s/4, s/4);
            break;
        }

        case 7: // Spinner - Sieu banh rang (Quay tron de cat)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTBLUE);
            setfillstyle(SOLID_FILL, BLUE);
            
            // 8 luoi dao sac nhon quay quanh truc thong qua zigzagTimer
            int pts[18];
            for (int j = 0; j < 8; j++) {
                float a = enemies[i].zigzagTimer + j * PI / 4;
                float r = (j % 2 == 0) ? s * 1.4 : s * 0.5; // Chong cheo tao hinh rang cua
                pts[j * 2] = cx + r * cos(a);
                pts[j * 2 + 1] = cy + r * sin(a);
            }
            pts[16] = pts[0]; pts[17] = pts[1];
            fillpoly(9, pts);
            
            // Loi bac bemat cat
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTGRAY);
            fillellipse(cx, cy, s*0.3, s*0.3);
            break;
        }

        case 8: // Stealth - Bong ma Manta (Bay luot, canh tau bay hinh luoi liem)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTGRAY);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // Hinh dang canh luoi liem sac canh
            int pts[] = {
                cx + (int)(s*1.8 * cos(angle + PI/2)), cy + (int)(s*1.8 * sin(angle + PI/2)), // Canh phai
                cx + (int)(s*0.5 * cos(angle)), cy + (int)(s*0.5 * sin(angle)),               // Mui nho nhon
                cx + (int)(s*1.8 * cos(angle - PI/2)), cy + (int)(s*1.8 * sin(angle - PI/2)), // Canh trai
                cx - (int)(s*0.5 * cos(angle)), cy - (int)(s*0.5 * sin(angle)),               // Duoi chom ra
                cx + (int)(s*1.8 * cos(angle + PI/2)), cy + (int)(s*1.8 * sin(angle + PI/2))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            // 2 mat do ngau
            setcolor(RED);
            fillellipse(cx + (int)(s*0.5*cos(angle+0.5)), cy + (int)(s*0.5*sin(angle+0.5)), 2, 2);
            fillellipse(cx + (int)(s*0.5*cos(angle-0.5)), cy + (int)(s*0.5*sin(angle-0.5)), 2, 2);
            break;
        }

        case 9: // Kamikaze - Ten lua tu sat (Lua chay phung phuc o duoi)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Duoi lua chay
            setcolor(RED);
            setfillstyle(SOLID_FILL, LIGHTRED);
            int flame[] = {
                cx, cy, 
                cx - (int)(s*1.5 * cos(angle-0.3)), cy - (int)(s*1.5 * sin(angle-0.3)),
                cx - (int)(s*2.0 * cos(angle)), cy - (int)(s*2.0 * sin(angle)), // Dinh lua chot
                cx - (int)(s*1.5 * cos(angle+0.3)), cy - (int)(s*1.5 * sin(angle+0.3)),
                cx, cy
            };
            fillpoly(5, flame);

            // Dau dan xuyen giap
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            int missile[] = {
                cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle)),
                cx + (int)(s*0.6 * cos(angle+1.5)), cy + (int)(s*0.6 * sin(angle+1.5)),
                cx - (int)(s*0.2 * cos(angle)), cy - (int)(s*0.2 * sin(angle)),
                cx + (int)(s*0.6 * cos(angle-1.5)), cy + (int)(s*0.6 * sin(angle-1.5)),
                cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle))
            };
            fillpoly(5, missile);
            break;
        }

        case 10: // Dodger - Bong ma luot gio (Ne dan nhanhnhen)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, CYAN);
            
            // Co the khi dong hoc, thuon dai
            int pts[] = {
                cx + (int)(s * cos(angle)), cy + (int)(s * sin(angle)),
                cx + (int)(s * 1.2 * cos(angle + 2.0)), cy + (int)(s * 1.2 * sin(angle + 2.0)),
                cx, cy, 
                cx + (int)(s * 1.2 * cos(angle - 2.0)), cy + (int)(s * 1.2 * sin(angle - 2.0)),
                cx + (int)(s * cos(angle)), cy + (int)(s * sin(angle))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            // Mach dien phat sang o 2 canh
            setcolor(LIGHTCYAN);
            circle(cx + (int)(s*0.5*cos(angle+2.0)), cy + (int)(s*0.5*sin(angle+2.0)), 2);
            circle(cx + (int)(s*0.5*cos(angle-2.0)), cy + (int)(s*0.5*sin(angle-2.0)), 2);
            break;
        }

        case 11: // Blaster - Phao dai thit (Bo may ban dan hang nang)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTMAGENTA);
            setfillstyle(SOLID_FILL, MAGENTA);
            
            // Khung chu nhat hinh xe tank
            int pts[] = {
                cx + (int)(s*1.2 * cos(angle-0.5)), cy + (int)(s*1.2 * sin(angle-0.5)),
                cx + (int)(s*1.2 * cos(angle+0.5)), cy + (int)(s*1.2 * sin(angle+0.5)),
                cx - (int)(s*0.8 * cos(angle-0.8)), cy - (int)(s*0.8 * sin(angle-0.8)),
                cx - (int)(s*0.8 * cos(angle+0.8)), cy - (int)(s*0.8 * sin(angle+0.8)),
                cx + (int)(s*1.2 * cos(angle-0.5)), cy + (int)(s*1.2 * sin(angle-0.5))
            };
            fillpoly(5, pts);

            // Nong sung khong lo o mat truoc
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);
            fillellipse(cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle)), s*0.4, s*0.4);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle)), 3, 3);
            break;
        }

        case 12: // Swarmer - Ky sinh trung (Bay theo dan, nhieu xuc tu)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Xuc tu ngoe nguay (dung thoi gian he thong hoac toa do lam lech pha)
            setcolor(LIGHTGREEN);
            for(int j=0; j<6; j++) {
                float a = angle + j * PI/3 + sin(cx + cy)*0.5; // Wriggle effect
                line(cx, cy, cx + (int)(s*1.2 * cos(a)), cy + (int)(s*1.2 * sin(a)));
            }
            
            // Than hinh tron ky sinh
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, GREEN);
            fillellipse(cx, cy, (int)(s*0.7), (int)(s*0.7));
            
            // Mat den lanh leo
            setcolor(BLACK);
            setfillstyle(SOLID_FILL, BLACK);
            fillellipse(cx + (int)(s*0.3 * cos(angle)), cy + (int)(s*0.3 * sin(angle)), 2, 2);
            break;
        }

        case 13: // Phantom (Bong Tuyet Koch xoay) - GIU NGUYEN
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            // enemies[i].zigzagTimer duoc lam goc xoay de hinh bong tuyet quay tron theo thoi gian
            // Ap dung thuat toan Fractal
            drawKochSnowflake(cx, cy, s + 5, 2, enemies[i].zigzagTimer, LIGHTBLUE);
            
            // Ve them loi nang luong phat sang o giua
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx, cy, 3, 3);
            break;
        }

        case 14: // Charger - Te giac mau (Lao toi dam nguoi choi)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, RED);
            
            // Than hinh tho thap
            int body[] = {
                cx + (int)(s * cos(angle - 1.5)), cy + (int)(s * sin(angle - 1.5)),
                cx + (int)(s * cos(angle + 1.5)), cy + (int)(s * sin(angle + 1.5)),
                cx - (int)(s * 1.5 * cos(angle - 0.5)), cy - (int)(s * 1.5 * sin(angle - 0.5)),
                cx - (int)(s * 1.5 * cos(angle + 0.5)), cy - (int)(s * 1.5 * sin(angle + 0.5)),
                cx + (int)(s * cos(angle - 1.5)), cy + (int)(s * sin(angle - 1.5))
            };
            fillpoly(5, body);

            // 2 chiec sung / nga khong lo chia ra tran ap
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, LIGHTRED);
            int horn1[] = {
                cx + (int)(s * cos(angle - 1.0)), cy + (int)(s * sin(angle - 1.0)),
                cx + (int)(s * 2.0 * cos(angle - 0.3)), cy + (int)(s * 2.0 * sin(angle - 0.3)),
                cx + (int)(s * 0.5 * cos(angle - 0.2)), cy + (int)(s * 0.5 * sin(angle - 0.2)),
                cx + (int)(s * cos(angle - 1.0)), cy + (int)(s * sin(angle - 1.0))
            };
            int horn2[] = {
                cx + (int)(s * cos(angle + 1.0)), cy + (int)(s * sin(angle + 1.0)),
                cx + (int)(s * 2.0 * cos(angle + 0.3)), cy + (int)(s * 2.0 * sin(angle + 0.3)),
                cx + (int)(s * 0.5 * cos(angle + 0.2)), cy + (int)(s * 0.5 * sin(angle + 0.2)),
                cx + (int)(s * cos(angle + 1.0)), cy + (int)(s * sin(angle + 1.0))
            };
            fillpoly(4, horn1); fillpoly(4, horn2);
            break;
        }

        case 15: // Sniper Elite - Thien nhan (Mat laser phuc tap)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Ngoi sao 8 canh (Octagram) xoay cham
            setcolor(CYAN);
            for(int j=0; j<8; j++) {
                float a = enemies[i].zigzagTimer * 0.5 + j * PI/4;
                line(cx, cy, cx + (int)(s * 1.5 * cos(a)), cy + (int)(s * 1.5 * sin(a)));
            }

            // Dong tu da giac ben trong
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);
            int pts[10];
            for (int j = 0; j < 4; j++) {
                pts[j*2] = cx + (int)(s * cos(angle + j*PI/2));
                pts[j*2+1] = cy + (int)(s * sin(angle + j*PI/2));
            }
            pts[8] = pts[0]; pts[9] = pts[1];
            fillpoly(5, pts);
            
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx + (int)(s*0.2 * cos(angle)), cy + (int)(s*0.2 * sin(angle)), 4, 4);
            break;
        }

        // ================== CAC MAU BOSS ==================
        case 16: // Boss 1: Loi mat troi chet (Dead Sun Core)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Vong hao quang buc xa
            setcolor(LIGHTRED);
            for (int r = s; r < s * 1.5; r += 5) {
            	// Ap dung thuat toan BresenhamCircle
                if (rand()%2 == 0) bresenhamCircle(cx, cy, r, LIGHTRED);
            }
            
            // Qua cau nang luong mat troi
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s, s);
            
            // Mat ran ác doc o giua loi mat troi
            setcolor(RED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, s/4, s*0.8);
            
            // 4 Module ve tinh xoay quanh de bao ve
            float t = enemies[i].specialTimer * 3.0; // Toc do xoay cua ve tinh
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, RED);
            for(int j=0; j<4; j++) {
                float a = t + j * PI/2;
                int px = cx + (int)(s * 1.6 * cos(a));
                int py = cy + (int)(s * 1.6 * sin(a));
                fillellipse(px, py, 12, 12);
                line(cx, cy, px, py); // Tia nang luong noi voi loi
            }
            break;
        }

        case 17: // Boss 2: Khoi lap phuong huc vo (Void Monolith)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, MAGENTA);
            
            // Ve mot hinh luc giac khong lo de tao hieu ung Khoi lap phuong 3D
            int pts[14];
            for (int j = 0; j < 6; j++) {
                pts[j*2] = cx + s * cos(j * PI / 3 + PI/6);
                pts[j*2+1] = cy + s * sin(j * PI / 3 + PI/6);
            }
            pts[12] = pts[0]; pts[13] = pts[1];
            fillpoly(7, pts);
            
            // Duong ke luoi (Grid) tao chieu sau 3D
            setcolor(LIGHTMAGENTA);
            line(cx, cy, pts[0], pts[1]);
            line(cx, cy, pts[4], pts[5]);
            line(cx, cy, pts[8], pts[9]);
            
            // Loi mang luoi dien tu
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s/3, s/3);
            
            // Vong tron luoi quay cham
            circle(cx, cy, s/1.5);
            break;
        }

        case 18: // Boss 3: To dia nguc (Hell Hive - Ke Trieu hoi)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Khung to ong luc giac (Hive Matrix)
            setcolor(CYAN);
            setfillstyle(SOLID_FILL, DARKGRAY);
            int pts[14];
            for (int j = 0; j < 6; j++) {
                pts[j*2] = cx + s * cos(j * PI / 3);
                pts[j*2+1] = cy + s * sin(j * PI / 3);
            }
            pts[12] = pts[0]; pts[13] = pts[1];
            fillpoly(7, pts);
            drawpoly(7, pts);

            // Nhung boc trung phat sang noi len giua to ong
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);
            for (int j = 0; j < 6; j++) {
                int px = cx + (int)(s * 0.6 * cos(j * PI / 3));
                int py = cy + (int)(s * 0.6 * sin(j * PI / 3));
                fillellipse(px, py, s/4, s/4);
            }
            
            // Lo tich tu nang luong khong gian (Giao diem goi dan em)
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx, cy, s/3, s/3);
            break;
        }

        case 19: // Boss 4: Sieu ho den (Black Hole Leviathan)
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // Tu dong quay qua zigzagTimer
            float rot = enemies[i].zigzagTimer * 2.0; 
            
            // 5 Xuc tu quai vat xoay vinh cuu giong vong xoay ngan ha
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, RED);
            for(int j = 0; j < 5; j++) {
                float a = rot + j * 2 * PI / 5;
                int arm[] = {
                    cx, cy,
                    cx + (int)(s * 0.5 * cos(a - 0.5)), cy + (int)(s * 0.5 * sin(a - 0.5)),
                    cx + (int)(s * 1.5 * cos(a + 0.5)), cy + (int)(s * 1.5 * sin(a + 0.5)), // Phan tuon cong chong chong
                    cx + (int)(s * 0.5 * cos(a + 1.0)), cy + (int)(s * 0.5 * sin(a + 1.0)),
                    cx, cy
                };
                fillpoly(5, arm);
            }
            
            // Mat trung tam: Nuot chung moi thu
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, BLACK);
            fillellipse(cx, cy, s*0.6, s*0.6);
            
            // Vong tron di diem ben trong ho den
            setcolor(RED);
            circle(cx, cy, (int)(s * 0.4));
            setcolor(YELLOW);
            circle(cx, cy, (int)(s * 0.2));
            break;
        }

        }
    }
}

void drawPowerUps() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerUps[i].active) continue;

        int px = powerUps[i].x;
        int py = powerUps[i].y;

        switch (powerUps[i].type) {
            case 1: 
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
            case 2: 
            {
                setcolor(LIGHTCYAN);
                setfillstyle(SOLID_FILL, LIGHTCYAN);
                int lightning[14] = {
                    px + 2, py - 8, px - 6, py + 2, px, py + 2,
                    px - 2, py + 8, px + 6, py - 2, px, py - 2,
                    px + 2, py - 8
                };
                fillpoly(7, lightning);
                break;
            }
            case 3: 
            {
                setcolor(LIGHTRED);
                setfillstyle(SOLID_FILL, LIGHTRED);
                fillellipse(px - 4, py - 3, 4, 4); 
                fillellipse(px + 4, py - 3, 4, 4); 
                int heart_bottom[8] = {
                    px - 8, py - 2, px + 8, py - 2, 
                    px, py + 7, px - 8, py - 2
                };
                fillpoly(4, heart_bottom);
                break;
            }
            case 4: 
            {
                setcolor(LIGHTBLUE);
                setfillstyle(SOLID_FILL, LIGHTBLUE);
                int shield[12] = {
                    px - 7, py - 6, px + 7, py - 6, px + 7, py + 2, 
                    px, py + 9, px - 7, py + 2, px - 7, py - 6
                };
                fillpoly(6, shield);
                setcolor(WHITE);
                line(px, py - 4, px, py + 2);
                line(px - 3, py - 1, px + 3, py - 1);
                break;
            }
            case 5: 
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
            case 6: 
            {
                setcolor(WHITE);
                setfillstyle(SOLID_FILL, LIGHTCYAN);
                fillellipse(px - 3, py - 3, 6, 6);
                setcolor(YELLOW);
                setfillstyle(SOLID_FILL, YELLOW);
                fillellipse(px - 3, py - 3, 2, 2);
                setcolor(BROWN);
                line(px + 1, py + 1, px + 7, py + 7);
                line(px + 2, py + 1, px + 8, py + 7);
                line(px + 1, py + 2, px + 7, py + 8);
                break;
            }
            case 7: 
            {
                setcolor(LIGHTMAGENTA);
                setfillstyle(SOLID_FILL, LIGHTMAGENTA);
                int star[22];
                for (int j = 0; j < 10; j++) {
                    float angle = j * PI / 5 - PI / 2;
                    int r = (j % 2 == 0) ? 10 : 4; 
                    star[j * 2] = px + cos(angle) * r;
                    star[j * 2 + 1] = py + sin(angle) * r;
                }
                star[20] = star[0]; 
                star[21] = star[1];
                fillpoly(11, star);
                break;
            }
            case 8: 
            {
                int r = 6; 
                const int BODY_FILL_COLOR = GREEN;
                const int LINE_COLOR = LIGHTGREEN;
                const int COCKPIT_FILL_COLOR = DARKGRAY;
                const int POD_FILL_COLOR = LIGHTGREEN;

                setcolor(LINE_COLOR);
                setfillstyle(SOLID_FILL, BODY_FILL_COLOR);

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
                
                setcolor(WHITE);
                circle(px, py, 14); 
                break;
            }
        }
    }
}

void drawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            if (particles[i].type == 1) {
                // Ve Shockwave
                setcolor(particles[i].life > 6 ? WHITE : LIGHTCYAN); 
                circle(particles[i].x, particles[i].y, (int)particles[i].size);
                circle(particles[i].x, particles[i].y, (int)particles[i].size - 1); 
            } else {
                // Ve tia lua vang
                int col = particles[i].color;
                
                // Hat nguoi dan
                if (particles[i].life < particles[i].maxLife / 3) {
                    col = DARKGRAY; 
                } else if (particles[i].life < particles[i].maxLife / 2 && col == WHITE) {
                    col = YELLOW;   
                }
                
                setcolor(col);
                setfillstyle(SOLID_FILL, col);
                
                int r = (int)(particles[i].size * ((float)particles[i].life / particles[i].maxLife));
                if (r < 1) r = 1;
                
                fillellipse(particles[i].x, particles[i].y, r, r);
            }
        }
    }
}

void drawLaser() {
    if (player.laserTimer > 0) {
        int x = player.x;
        int y_top = 0; 
        int y_bottom = player.y - (int)(player.radius * 1.6);
        int width = 30; 

        setfillstyle(SOLID_FILL, (rand() % 2) ? RED : LIGHTRED);
        bar(x - width/2, y_top, x + width/2, y_bottom);

        setfillstyle(SOLID_FILL, YELLOW);
        bar(x - width/4, y_top, x + width/4, y_bottom);

        setcolor(WHITE);
        line(x, y_top, x, y_bottom);
        
        setcolor(LIGHTRED);
        circle(x, y_bottom, rand() % 15 + 5);
    }
}

void drawUI() {
    setcolor(LIGHTCYAN);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    
    char scoreText[20];
    sprintf(scoreText, "Diem: %d", score);
    outtextxy(10, 10, scoreText); 
    
    char livesText[20];
    sprintf(livesText, "Mang: %d", player.lives);
    outtextxy(10, 40, livesText);
    
    setcolor(player.ultimateTimer >= ULTIMATE_COOLDOWN ? LIGHTCYAN : LIGHTGRAY);
    rectangle(10, 70, 110, 90);
    setfillstyle(SOLID_FILL, player.ultimateTimer >= ULTIMATE_COOLDOWN ? LIGHTCYAN : LIGHTGRAY);
    float barProgress = player.ultimateTimer < ULTIMATE_COOLDOWN ? player.ultimateTimer : ULTIMATE_COOLDOWN;
    bar(12, 72, 12 + (barProgress / ULTIMATE_COOLDOWN) * 96, 88);

    char levelText[30];
    sprintf(levelText, "Cap do: %d", difficultyLevel);
    outtextxy(10, 100, levelText);

    // Thong bao khu vuc an toan (Chuyen cap)
if (levelTransitionTimer > 0 && !gameOver) {
        levelTransitionTimer -= 0.02; 
        
        setcolor(YELLOW);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 4);
        char lvlMsg[50];
        
        if (difficultyLevel == 4) {
            sprintf(lvlMsg, "CAP DO CUOI CUNG!");
        } else {
            sprintf(lvlMsg, "CAP DO %d", difficultyLevel);
        }
        
        int textX = SCREEN_WIDTH / 2 - textwidth(lvlMsg) / 2;
        outtextxy(textX, SCREEN_HEIGHT / 2 - 50, lvlMsg); 
        
    }

if (gameOver) {
        setcolor(LIGHTRED);
        settextstyle(DEFAULT_FONT, HORIZ_DIR, 3);
        
        char gameOverMsg[] = "Ket Thuc!";
        int gameOverX = SCREEN_WIDTH / 2 - textwidth(gameOverMsg) / 2;
        outtextxy(gameOverX, SCREEN_HEIGHT / 2 - 30, gameOverMsg);
        
        char finalScore[30];
        sprintf(finalScore, "Diem Cuoi: %d", score);
        int scoreX = SCREEN_WIDTH / 2 - textwidth(finalScore) / 2;
        outtextxy(scoreX, SCREEN_HEIGHT / 2 + 10, finalScore);
        
        char retryMsg[] = "Nhan R de Choi Lai";
        int retryX = SCREEN_WIDTH / 2 - textwidth(retryMsg) / 2;
        outtextxy(retryX, SCREEN_HEIGHT / 2 + 50, retryMsg);
    }
}

// HAM CHINH (MAIN)
int main() {
    // Khoi tao do hoa voi kich thuoc man hinh chuan
    initwindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chien ham ngan ha");
    srand((unsigned int)time(NULL));

    initGame();
    
    showInstructions();

    int page = 0; // Dung cho Double Buffering (Chong nhay man hinh)

    while (1) {
        setactivepage(page); // Ve len trang an

        // Nhan R de choi lai khi Game Over
        if (gameOver && (GetAsyncKeyState('R') & 0x8000)) {
            mciSendString("stop laser_sound", NULL, 0, NULL); 
            isLaserSoundPlaying = false;
            initGame();
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        // --- CHI CAP NHAT TOA DO / LOGIC KHI GAME CHUA OVER ---
        if (!gameOver) {
            updateStars();
            updatePlayer();
            updateCompanions();
            updateBullets();
            spawnEnemy();
            updateEnemies();
            updatePowerUps();
            updateParticles();
            checkCollisions();
        }

        // --- HAM VE DO HOA CHAY LIEN TUC (Giu hinh anh dong bang khi chet) ---
        drawBackground(); 
        drawParticles();
        drawBullets();
        drawEnemies();
        drawPowerUps();
        
        // Neu da chet thi an may bay di
        if (!gameOver) {
            drawPlayer();
            drawCompanions();
            drawLaser();
        }
        
        drawUI();

        setvisualpage(page); // Hien thi trang vua ve xong
        page = 1 - page;     // Lat trang

        delay(20);
    }

    closegraph();
    return 0;
}
