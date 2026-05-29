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
bool isPaused = false;
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
void shootCompanionBullet();
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
        outtextxy(SCREEN_WIDTH / 2 - 250, 400, "- Nhat cac vat pham de nang cap vu khi");
        outtextxy(SCREEN_WIDTH / 2 - 250, 440, "- Nhan Space de dung");

        if ((clock() / 500) % 2 == 0) {
            setcolor(LIGHTGREEN);
            outtextxy(SCREEN_WIDTH / 2 - 175, 480, ">> NHAN ENTER DE BAT DAU <<");
        }

        setvisualpage(page);
        page = 1 - page;

        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            break; 
        }
        delay(20);
    }
}

void playExplosionSound() {
    static clock_t lastExplosionTime = 0;
    clock_t now = clock();
    
    if ((now - lastExplosionTime) * 1000 / CLOCKS_PER_SEC >= 100) {
        mciSendString("seek vuno_sound to start", NULL, 0, NULL);
        mciSendString("play vuno_sound", NULL, 0, NULL);
        lastExplosionTime = now;
    }
}

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

    for (int i = 0; i < MAX_COMPANIONS; i++) {
        companions[i].active = false;
        companions[i].radius = 10;
        companions[i].lastAngle = -PI / 2;
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
        bullets[i].highDamage = false;
        bullets[i].isEnemy = false;
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

    score = 0;
    gameOver = false;
    bool isPaused = false;
    lastShotTime = 0;
    currentShootDelay = SHOOT_DELAY;
    currentPlayerSpeed = PLAYER_SPEED;
    currentBulletSize = 5;
    difficultyLevel = 0;
    bossActive = false;
    postBossDifficulty = 0;
    levelTransitionTimer = 0;
    nextBossScore = 500;

    mciSendString("close bgm", NULL, 0, NULL); 
    mciSendString("open \"nhacnen.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
    mciSendString("close laser_sound", NULL, 0, NULL); 
    mciSendString("open \"laser.wav\" type mpegvideo alias laser_sound", NULL, 0, NULL);
    
    mciSendString("close vuno_sound", NULL, 0, NULL); 
    mciSendString("open \"vuno.wav\" type waveaudio alias vuno_sound", NULL, 0, NULL);
}

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
    player.lastAngle = -PI / 2;  
    player.y = SCREEN_HEIGHT - player.radius - 20; 
    player.dy = 0;

    player.dx = 0; 
    if (GetAsyncKeyState(0x41) & 0x8000) player.dx -= currentPlayerSpeed; 
    if (GetAsyncKeyState(0x44) & 0x8000) player.dx += currentPlayerSpeed; 

    // [Phep bien doi Affine - Tinh tien] Phi thuyen nguoi choi: Truot qua trai/phai (player.dx).
    player.x += player.dx; 

    if (player.x < player.radius) player.x = player.radius;
    if (player.x > SCREEN_WIDTH - player.radius) player.x = SCREEN_WIDTH - player.radius;

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        clock_t now = clock();
        if ((now - lastShotTime) * 1000 / CLOCKS_PER_SEC >= currentShootDelay) {
            
            if (player.laserTimer <= 0) {
                shootBullet(); 
            }

            if (player.companionBoostTimer > 0) {
                shootCompanionBullet(); 
            }

            lastShotTime = now;
        }
    }

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

    if (ismouseclick(WM_RBUTTONDOWN)) {
        clearmouseclick(WM_RBUTTONDOWN);
        if (player.ultimateTimer >= ULTIMATE_COOLDOWN) {
            player.laserTimer = 1; 
            triggerUltimate();
        }
    }
    
    if (player.invincibilityTimer > 0) player.invincibilityTimer -= 0.02; 
    
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
            // [Phep bien doi Affine - Tinh tien] Toan bo dan: Ban thang len tren hoac cheo xuong duoi.
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

            // [Phep bien doi Affine - Tinh tien] Quai 10 (Dodger) tinh tien de ne dan
            if (enemies[i].type == 10) { 
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
                                bullets[k].isEnemy = true;
                                break;
                            }
                        }
                    }
                    enemies[i].specialTimer = 0;
                }
            }

            // [Phep bien doi Affine - Tinh tien] Tat ca quai vat: Tinh tien vi tri
            enemies[i].x += enemies[i].dx;
            enemies[i].y += enemies[i].dy;
            
            if (enemies[i].x < enemies[i].radius) enemies[i].x = enemies[i].radius;
            if (enemies[i].x > SCREEN_WIDTH - enemies[i].radius) enemies[i].x = SCREEN_WIDTH - enemies[i].radius;
            if (enemies[i].y < enemies[i].radius) enemies[i].y = enemies[i].radius;
            
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
                // [Phep bien doi Affine - Co gian] Hieu ung vu no: Vong song xung kich mo rong ban kinh
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

void checkCollisions() {
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

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            
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

    if (difficultyLevel > oldLevel) {
        levelTransitionTimer = 3.0; 
    }

    if (!bossActive && levelTransitionTimer <= 0 && score >= nextBossScore) {
        spawnBoss();
        nextBossScore += 500; 
    }
}

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
}

void shootCompanionBullet() {
    PlaySound(TEXT("ban.wav"), NULL, SND_FILENAME | SND_ASYNC);

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
    if (dy > dx) {
        int temp = dx; dx = dy; dy = temp;
        isSwap = 1;
    }
    
    int d = 2 * dy - dx;
    int incE = 2 * dy;
    int incNE = 2 * (dy - dx);
    
    int x = x1, y = y1;
    putpixel(x, y, color);
    
    for (int i = 1; i <= dx; i++) {
        if (d < 0) {
            d += incE;
            if (isSwap) y += sy; 
            else x += sx;
        } else {
            d += incNE;
            x += sx;
            y += sy;
        }
        putpixel(x, y, color);
    }
}

void recursiveBoundaryFill(int x, int y, int fill_color, int boundary_color) {
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
    int d = 3 - 2 * r; 

    while (y >= x) {
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

void drawKochLine(float x1, float y1, float x2, float y2, int iter, int color) {
    if (iter == 0) {
        // [Ap dung thuat toan Bresenham Line] Ve cac net thang tao nen duong cong Fractal
        bresenhamLine((int)x1, (int)y1, (int)x2, (int)y2, color);
    } else {
        float dx = (x2 - x1) / 3.0f;
        float dy = (y2 - y1) / 3.0f;

        float p1x = x1 + dx;
        float p1y = y1 + dy;

        float p2x = x1 + 2 * dx;
        float p2y = y1 + 2 * dy;

        // [Phep bien doi Affine - Quay] Xoay goc Pi/3 tinh toa do dinh tam giac Koch
        float px = p1x + dx * cos(PI / 3) + dy * sin(PI / 3);
        float py = p1y - dx * sin(PI / 3) + dy * cos(PI / 3);
        
        drawKochLine(x1, y1, p1x, p1y, iter - 1, color);
        drawKochLine(p1x, p1y, px, py, iter - 1, color);
        drawKochLine(px, py, p2x, p2y, iter - 1, color);
        drawKochLine(p2x, p2y, x2, y2, iter - 1, color);
    }
}

void drawKochSnowflake(int x, int y, int radius, int iter, float angle, int color) {
    // [Phep bien doi Affine - Quay] Tinh toan 3 dinh cua tam giac deu duoc xoay theo goc 'angle'
    float p1x = x + radius * cos(angle - PI / 2);
    float p1y = y + radius * sin(angle - PI / 2);
    float p2x = x + radius * cos(angle + PI / 6);
    float p2y = y + radius * sin(angle + PI / 6);
    float p3x = x + radius * cos(angle + 5 * PI / 6);
    float p3y = y + radius * sin(angle + 5 * PI / 6);

    // [Hinh hoc Fractal] Ap dung thuat toan ve duong cong Koch
    drawKochLine(p1x, p1y, p2x, p2y, iter, color);
    drawKochLine(p2x, p2y, p3x, p3y, iter, color);
    drawKochLine(p3x, p3y, p1x, p1y, iter, color);
}

void drawPlayer() {
    if (player.invincibilityTimer > 0) {
        if ((int)(player.invincibilityTimer * 15) % 2 == 0) {
            if (player.shieldTimer > 0) {
                // [Ap dung thuat toan Midpoint Circle] Ve vong tron la chan
                midpointCircle(player.x, player.y, player.radius + 10, LIGHTBLUE);
            }
            return; 
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

    // [Ap dung thuat toan Midpoint Line] Ve cac duong van ranh xanh dam
    midpointLine(x - (int)(r * 0.4), y - (int)(r * 0.1), x - (int)(r * 1.4), y + (int)(r * 0.3), BLUE);
    midpointLine(x - (int)(r * 0.5), y + (int)(r * 0.1), x - (int)(r * 1.3), y + (int)(r * 0.4), BLUE);
    midpointLine(x - (int)(r * 0.6), y + (int)(r * 0.3), x - (int)(r * 1.2), y + (int)(r * 0.5), BLUE);
    midpointLine(x + (int)(r * 0.4), y - (int)(r * 0.1), x + (int)(r * 1.4), y + (int)(r * 0.3), BLUE);
    midpointLine(x + (int)(r * 0.5), y + (int)(r * 0.1), x + (int)(r * 1.3), y + (int)(r * 0.4), BLUE);
    midpointLine(x + (int)(r * 0.6), y + (int)(r * 0.3), x + (int)(r * 1.2), y + (int)(r * 0.5), BLUE);
    midpointLine(x - (int)(r * 0.2), y - (int)(r * 1.2), x + (int)(r * 0.2), y - (int)(r * 1.2), BLUE);
    midpointLine(x - (int)(r * 0.1), y - (int)(r * 1.0), x + (int)(r * 0.1), y - (int)(r * 1.0), BLUE);
    midpointLine(x - (int)(r * 0.1), y - (int)(r * 0.5), x + (int)(r * 0.1), y - (int)(r * 0.5), BLUE);

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
    
    // [Ap dung thuat toan Boundary Fill] To mau xanh lap day khoang chua vu khi
    recursiveBoundaryFill(x - (int)(r * 0.75), y + (int)(r * 0.6), LIGHTCYAN, LINE_COLOR);

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

    if (player.shieldTimer > 0) {
        // [Ap dung thuat toan Midpoint Circle] Ve vong tron la chan
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
                // [Ap dung thuat toan Bresenham Circle] Ve dan cua dich
                int enemyBulletRadius = 5;
                setcolor(WHITE); 
                setfillstyle(SOLID_FILL, LIGHTRED); 
                fillellipse(bx, by, enemyBulletRadius, enemyBulletRadius);
                circle(bx, by, enemyBulletRadius);
            } 
            else {
                // [Phep bien doi Affine - Co gian] He so co gian phinh to vien dan (currentBulletSize * 1.5)
                int radius = player.bulletSizeTimer > 0 ? currentBulletSize * 1.5 : currentBulletSize;

                if (player.currentFruitType == 1) { 
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
        // [Phep bien doi Affine - Quay] Tinh goc quay (angle) huong ve phi thuyen nguoi choi
        float angle = atan2(
            player.y - enemies[i].y,
            player.x - enemies[i].x
        );

        switch (enemies[i].type) {

        case 1: 
        {
            int cx = enemies[i].x;
            int cy = enemies[i].y;
            int s = size;

            setcolor(LIGHTMAGENTA);
            line(cx, cy, cx - (int)(s * 1.2), cy - (int)(s * 0.8));
            line(cx, cy, cx + (int)(s * 1.2), cy - (int)(s * 0.8));
            line(cx, cy, cx - (int)(s * 0.8), cy + (int)(s * 1.2));
            line(cx, cy, cx + (int)(s * 0.8), cy + (int)(s * 1.2));

            setcolor(WHITE);
            setfillstyle(SOLID_FILL, DARKGRAY);
            int alien_body[] = {
                cx, cy - s,          
                cx + s, cy,          
                cx, cy + s,          
                cx - s, cy,          
                cx, cy - s           
            };
            fillpoly(5, alien_body);

            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, (int)(s * 0.6), (int)(s * 0.6));

            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, (int)(s * 0.15), (int)(s * 0.4));
            
            setcolor(WHITE);
            putpixel(cx - 2, cy - 2, WHITE);
            putpixel(cx - 3, cy - 2, WHITE);

            break;
        }

        case 2: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTGREEN);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // [Phep bien doi Affine - Quay] Quai vat quay theo muc tieu
            int pts[] = {
                cx + (int)(s * 1.5 * cos(angle)), cy + (int)(s * 1.5 * sin(angle)),
                cx + (int)(s * cos(angle + 2.5)), cy + (int)(s * sin(angle + 2.5)),
                cx + (int)(s * 0.2 * cos(angle)), cy + (int)(s * 0.2 * sin(angle)), 
                cx + (int)(s * cos(angle - 2.5)), cy + (int)(s * sin(angle - 2.5)),
                cx + (int)(s * 1.5 * cos(angle)), cy + (int)(s * 1.5 * sin(angle))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, GREEN);
            fillellipse(cx + (int)(s*0.3*cos(angle)), cy + (int)(s*0.3*sin(angle)), s/3, s/3);
            break;
        }

        case 3: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            int pts[14];
            for (int j = 0; j < 6; j++) {
                pts[j * 2] = cx + s * cos(j * PI / 3 + PI/6);
                pts[j * 2 + 1] = cy + s * sin(j * PI / 3 + PI/6);
            }
            pts[12] = pts[0]; pts[13] = pts[1];
            fillpoly(7, pts);
            
            setcolor(LIGHTGRAY);
            line(cx - s, cy, cx + s, cy);
            line(cx, cy - s, cx, cy + s);

            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, s/2, s/2);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s/4, s/4);
            break;
        }

        case 4: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTMAGENTA);
            
            // [Phep bien doi Affine - Quay] Quai vat quay theo muc tieu
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
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);
            fillellipse(cx, cy, s*0.7, s*0.7);
            break;
        }

        case 5: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            setcolor(CYAN);
            for(int j = 0; j < 4; j++) {
                float a = angle + j * PI/2;
                line(cx + (int)(s*0.8*cos(a)), cy + (int)(s*0.8*sin(a)), 
                     cx + (int)(s*1.5*cos(a)), cy + (int)(s*1.5*sin(a)));
            }
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);
            int eye[] = {cx, cy-s, cx+s, cy, cx, cy+s, cx-s, cy, cx, cy-s};
            fillpoly(5, eye);
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, CYAN);
            fillellipse(cx + (int)(s*0.2*cos(angle)), cy + (int)(s*0.2*sin(angle)), s/3, s/3);
            break;
        }

        case 6: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(RED);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            fillellipse(cx, cy, s, s);
            
            setcolor(LIGHTRED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx - s/2, cy - s/3, s/3, s/3);
            fillellipse(cx + s/3, cy + s/2, s/4, s/4);
            fillellipse(cx + s/2, cy - s/4, s/3, s/3);
            fillellipse(cx - s/3, cy + s/3, s/4, s/4);
            break;
        }

        case 7: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTBLUE);
            setfillstyle(SOLID_FILL, BLUE);
            
            // [Phep bien doi Affine - Quay] Quai vat xoay vong lap quanh tam de cat
            int pts[18];
            for (int j = 0; j < 8; j++) {
                float a = enemies[i].zigzagTimer + j * PI / 4;
                float r = (j % 2 == 0) ? s * 1.4 : s * 0.5; 
                pts[j * 2] = cx + r * cos(a);
                pts[j * 2 + 1] = cy + r * sin(a);
            }
            pts[16] = pts[0]; pts[17] = pts[1];
            fillpoly(9, pts);
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTGRAY);
            fillellipse(cx, cy, s*0.3, s*0.3);
            break;
        }

        case 8: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTGRAY);
            setfillstyle(SOLID_FILL, DARKGRAY);
            
            // [Phep bien doi Affine - Quay] Quai vat quay theo muc tieu
            int pts[] = {
                cx + (int)(s*1.8 * cos(angle + PI/2)), cy + (int)(s*1.8 * sin(angle + PI/2)), 
                cx + (int)(s*0.5 * cos(angle)), cy + (int)(s*0.5 * sin(angle)),               
                cx + (int)(s*1.8 * cos(angle - PI/2)), cy + (int)(s*1.8 * sin(angle - PI/2)), 
                cx - (int)(s*0.5 * cos(angle)), cy - (int)(s*0.5 * sin(angle)),               
                cx + (int)(s*1.8 * cos(angle + PI/2)), cy + (int)(s*1.8 * sin(angle + PI/2))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            setcolor(RED);
            fillellipse(cx + (int)(s*0.5*cos(angle+0.5)), cy + (int)(s*0.5*sin(angle+0.5)), 2, 2);
            fillellipse(cx + (int)(s*0.5*cos(angle-0.5)), cy + (int)(s*0.5*sin(angle-0.5)), 2, 2);
            break;
        }

        case 9: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            setcolor(RED);
            setfillstyle(SOLID_FILL, LIGHTRED);
            int flame[] = {
                cx, cy, 
                cx - (int)(s*1.5 * cos(angle-0.3)), cy - (int)(s*1.5 * sin(angle-0.3)),
                cx - (int)(s*2.0 * cos(angle)), cy - (int)(s*2.0 * sin(angle)), 
                cx - (int)(s*1.5 * cos(angle+0.3)), cy - (int)(s*1.5 * sin(angle+0.3)),
                cx, cy
            };
            fillpoly(5, flame);

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

        case 10: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, CYAN);
            
            int pts[] = {
                cx + (int)(s * cos(angle)), cy + (int)(s * sin(angle)),
                cx + (int)(s * 1.2 * cos(angle + 2.0)), cy + (int)(s * 1.2 * sin(angle + 2.0)),
                cx, cy, 
                cx + (int)(s * 1.2 * cos(angle - 2.0)), cy + (int)(s * 1.2 * sin(angle - 2.0)),
                cx + (int)(s * cos(angle)), cy + (int)(s * sin(angle))
            };
            fillpoly(5, pts);
            drawpoly(5, pts);
            
            setcolor(LIGHTCYAN);
            circle(cx + (int)(s*0.5*cos(angle+2.0)), cy + (int)(s*0.5*sin(angle+2.0)), 2);
            circle(cx + (int)(s*0.5*cos(angle-2.0)), cy + (int)(s*0.5*sin(angle-2.0)), 2);
            break;
        }

        case 11: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(LIGHTMAGENTA);
            setfillstyle(SOLID_FILL, MAGENTA);
            
            int pts[] = {
                cx + (int)(s*1.2 * cos(angle-0.5)), cy + (int)(s*1.2 * sin(angle-0.5)),
                cx + (int)(s*1.2 * cos(angle+0.5)), cy + (int)(s*1.2 * sin(angle+0.5)),
                cx - (int)(s*0.8 * cos(angle-0.8)), cy - (int)(s*0.8 * sin(angle-0.8)),
                cx - (int)(s*0.8 * cos(angle+0.8)), cy - (int)(s*0.8 * sin(angle+0.8)),
                cx + (int)(s*1.2 * cos(angle-0.5)), cy + (int)(s*1.2 * sin(angle-0.5))
            };
            fillpoly(5, pts);

            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTMAGENTA);
            fillellipse(cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle)), s*0.4, s*0.4);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx + (int)(s*1.2 * cos(angle)), cy + (int)(s*1.2 * sin(angle)), 3, 3);
            break;
        }

        case 12: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            setcolor(LIGHTGREEN);
            for(int j=0; j<6; j++) {
                float a = angle + j * PI/3 + sin(cx + cy)*0.5; 
                // [Ap dung thuat toan Bresenham Line giap tiep bang Line MACRO] - Xu ly xuc tu
                line(cx, cy, cx + (int)(s*1.2 * cos(a)), cy + (int)(s*1.2 * sin(a)));
            }
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, GREEN);
            fillellipse(cx, cy, (int)(s*0.7), (int)(s*0.7));
            
            setcolor(BLACK);
            setfillstyle(SOLID_FILL, BLACK);
            fillellipse(cx + (int)(s*0.3 * cos(angle)), cy + (int)(s*0.3 * sin(angle)), 2, 2);
            break;
        }

        case 13: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            // [Hinh hoc Fractal]
            // [Phep bien doi Affine - Quay] Quai vat xoay vong lap quanh tam (enemies[i].zigzagTimer)
            drawKochSnowflake(cx, cy, s + 5, 2, enemies[i].zigzagTimer, LIGHTBLUE);
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx, cy, 3, 3);
            break;
        }

        case 14: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, RED);
            
            int body[] = {
                cx + (int)(s * cos(angle - 1.5)), cy + (int)(s * sin(angle - 1.5)),
                cx + (int)(s * cos(angle + 1.5)), cy + (int)(s * sin(angle + 1.5)),
                cx - (int)(s * 1.5 * cos(angle - 0.5)), cy - (int)(s * 1.5 * sin(angle - 0.5)),
                cx - (int)(s * 1.5 * cos(angle + 0.5)), cy - (int)(s * 1.5 * sin(angle + 0.5)),
                cx + (int)(s * cos(angle - 1.5)), cy + (int)(s * sin(angle - 1.5))
            };
            fillpoly(5, body);

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

        case 15: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            setcolor(CYAN);
            for(int j=0; j<8; j++) {
                float a = enemies[i].zigzagTimer * 0.5 + j * PI/4;
                line(cx, cy, cx + (int)(s * 1.5 * cos(a)), cy + (int)(s * 1.5 * sin(a)));
            }

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

        case 16: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            setcolor(LIGHTRED);
            for (int r = s; r < s * 1.5; r += 5) {
                // [Ap dung thuat toan Bresenham Circle] Ve vong hao quang buc xa
                if (rand()%2 == 0) bresenhamCircle(cx, cy, r, LIGHTRED);
            }
            
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s, s);
            
            setcolor(RED);
            setfillstyle(SOLID_FILL, RED);
            fillellipse(cx, cy, s/4, s*0.8);
            
            float t = enemies[i].specialTimer * 3.0; 
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, RED);
            for(int j=0; j<4; j++) {
                float a = t + j * PI/2;
                int px = cx + (int)(s * 1.6 * cos(a));
                int py = cy + (int)(s * 1.6 * sin(a));
                fillellipse(px, py, 12, 12);
                line(cx, cy, px, py); 
            }
            break;
        }

        case 17: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, MAGENTA);
            
            int pts[14];
            for (int j = 0; j < 6; j++) {
                pts[j*2] = cx + s * cos(j * PI / 3 + PI/6);
                pts[j*2+1] = cy + s * sin(j * PI / 3 + PI/6);
            }
            pts[12] = pts[0]; pts[13] = pts[1];
            fillpoly(7, pts);
            
            // [Ap dung thuat toan Midpoint Line] Ve luoi Grid 3D cua boss khoi lap phuong
            setcolor(LIGHTMAGENTA);
            midpointLine(cx, cy, pts[0], pts[1], LIGHTMAGENTA);
            midpointLine(cx, cy, pts[4], pts[5], LIGHTMAGENTA);
            midpointLine(cx, cy, pts[8], pts[9], LIGHTMAGENTA);
            
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, YELLOW);
            fillellipse(cx, cy, s/3, s/3);
            
            circle(cx, cy, s/1.5);
            break;
        }

        case 18: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
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

            setcolor(WHITE);
            setfillstyle(SOLID_FILL, LIGHTCYAN);
            for (int j = 0; j < 6; j++) {
                int px = cx + (int)(s * 0.6 * cos(j * PI / 3));
                int py = cy + (int)(s * 0.6 * sin(j * PI / 3));
                fillellipse(px, py, s/4, s/4);
            }
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, WHITE);
            fillellipse(cx, cy, s/3, s/3);
            break;
        }

        case 19: 
        {
            int cx = enemies[i].x, cy = enemies[i].y, s = size;
            
            // [Phep bien doi Affine - Quay] Quai vat xoay vong lap quanh tam (zigzagTimer)
            float rot = enemies[i].zigzagTimer * 2.0; 
            
            setcolor(YELLOW);
            setfillstyle(SOLID_FILL, RED);
            for(int j = 0; j < 5; j++) {
                float a = rot + j * 2 * PI / 5;
                int arm[] = {
                    cx, cy,
                    cx + (int)(s * 0.5 * cos(a - 0.5)), cy + (int)(s * 0.5 * sin(a - 0.5)),
                    cx + (int)(s * 1.5 * cos(a + 0.5)), cy + (int)(s * 1.5 * sin(a + 0.5)), 
                    cx + (int)(s * 0.5 * cos(a + 1.0)), cy + (int)(s * 0.5 * sin(a + 1.0)),
                    cx, cy
                };
                fillpoly(5, arm);
            }
            
            setcolor(WHITE);
            setfillstyle(SOLID_FILL, BLACK);
            fillellipse(cx, cy, s*0.6, s*0.6);
            
            // [Ap dung thuat toan Midpoint Circle / BGI Circle] Ve loi mat troi
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
                setcolor(particles[i].life > 6 ? WHITE : LIGHTCYAN); 
                circle(particles[i].x, particles[i].y, (int)particles[i].size);
                circle(particles[i].x, particles[i].y, (int)particles[i].size - 1); 
            } else {
                int col = particles[i].color;
                
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

int main() {
    initwindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Chien ham ngan ha");
    srand((unsigned int)time(NULL));

    initGame();
    showInstructions();

    int page = 0; 
    bool spacePressed = false; 

    while (1) {
        setactivepage(page); 
        cleardevice();       

        if (gameOver && (GetAsyncKeyState('R') & 0x8000)) {
            mciSendString("stop laser_sound", NULL, 0, NULL); 
            isLaserSoundPlaying = false;
            initGame();
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        if (!gameOver) {
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                if (!spacePressed) {
                    isPaused = !isPaused; 
                    spacePressed = true;  
                }
            } else {
                spacePressed = false; 
            }
        }

        if (!gameOver && !isPaused) {
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

        drawBackground(); 
        drawParticles();
        drawBullets();
        drawEnemies();
        drawPowerUps();
        
        if (!gameOver) {
            drawPlayer();
            drawCompanions();
            drawLaser();
        }
        
        drawUI();

        if (isPaused && !gameOver) {
            setcolor(YELLOW);
            settextstyle(DEFAULT_FONT, HORIZ_DIR, 4);
            char pauseTitle[] = "TAM DUNG";
            outtextxy(SCREEN_WIDTH / 2 - textwidth(pauseTitle) / 2, SCREEN_HEIGHT / 2 - 40, pauseTitle);

            setcolor(WHITE);
            settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
            char resumeMsg[] = "Nhan Space de tiep tuc";
            outtextxy(SCREEN_WIDTH / 2 - textwidth(resumeMsg) / 2, SCREEN_HEIGHT / 2 + 10, resumeMsg);
        }

        setvisualpage(page); 
        page = 1 - page;     

        delay(20);
    }

    closegraph();
    return 0;
}
