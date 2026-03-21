#include <graphics.h>
#include <conio.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

void drawBackground();
void drawPlayer(int x, int y);
void drawCompanion(int x, int y);
void drawDroneEnemy(int x, int y);
void drawBossEnemy(int x, int y);
void drawUI();

int main() {
	initwindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter - Demo Chuong 1");

    // 2. V? c?nh n?n (Không gian vu tr?)
    drawBackground();

    // 3. V? nhân v?t chính và máy bay h? tr?
    drawPlayer(400, 500);
    drawCompanion(330, 530); // Tàu h? tr? trái
    drawCompanion(470, 530); // Tàu h? tr? ph?i

    // 4. V? phe d?ch (Phác th?o c?nh ch?m trán)
    drawDroneEnemy(200, 150);
    drawDroneEnemy(600, 150);
    drawBossEnemy(400, 100);

    // 5. V? giao di?n thông tin (UI)
    drawUI();

    // 6. D?ng màn hình d? giáo viên ch?m di?m
    getch();

    // D?n d?p b? nh? d? h?a
    closegraph();
    return 0;
}

// Hàm v? n?n vu tr?
void drawBackground() {
    setbkcolor(BLACK);
    cleardevice();

    // V? m?t vài ngôi sao tinh (s? d?ng circle)
    setcolor(WHITE);
    setfillstyle(SOLID_FILL, WHITE);
    for (int i = 0; i < 50; i++) {
        int x = rand() % SCREEN_WIDTH;
        int y = rand() % SCREEN_HEIGHT;
        circle(x, y, 1);
    }
}

// Hàm v? tàu ngu?i choi (Phe Ta)
void drawPlayer(int x, int y) {
    int r = 15;

    // Ph?i màu cho tàu chính (setcolor, setfillstyle, floodfill)
    setcolor(LIGHTCYAN);
    setfillstyle(SOLID_FILL, CYAN);

    // V? thân tàu b?ng rectangle
    rectangle(x - r / 3, y - r, x + r / 3, y + r);
    floodfill(x, y, LIGHTCYAN);

    // V? mui tàu b?ng da giác (k?t h?p các du?ng th?ng)
    int nose[6] = {
        x, y - r - 10,
        x - r / 3, y - r,
        x + r / 3, y - r
    };
    fillpoly(3, nose);

    // V? cánh tàu
    setfillstyle(SOLID_FILL, LIGHTCYAN);
    bar(x - r - 12, y - 5, x + r + 12, y + 5);

    // V? duôi h?a ti?n
    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, RED);
    // S? d?ng line d? v? tia l?a
    line(x, y + r, x, y + r + 15);
    line(x - 3, y + r, x - 5, y + r + 10);
    line(x + 3, y + r, x + 5, y + r + 10);
}

// Hàm v? máy bay h? tr?
void drawCompanion(int x, int y) {
    int r = 10;
    setcolor(LIGHTGREEN);
    setfillstyle(SOLID_FILL, GREEN);

    rectangle(x - r / 3, y - r, x + r / 3, y + r);
    floodfill(x, y, LIGHTGREEN);

    int nose[6] = {
        x, y - r - 6,
        x - r / 3, y - r,
        x + r / 3, y - r
    };
    fillpoly(3, nose);
    bar(x - r - 8, y - 4, x + r + 8, y + 4);
}

// Hàm v? k? d?ch lo?i nh? (Phe Ð?ch)
void drawDroneEnemy(int x, int y) {
    int size = 20;
    
    setcolor(LIGHTRED);
    setfillstyle(SOLID_FILL, DARKGRAY);
    
    // Yêu c?u rubric: S? d?ng ellipse
    ellipse(x, y, 0, 360, size, size / 2);
    floodfill(x, y, LIGHTRED);
    
    // Lõi nang lu?ng
    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, LIGHTRED);
    circle(x, y, 5);
    floodfill(x, y, YELLOW);
    
    // S? d?ng line d? v? súng hai bên
    setcolor(WHITE);
    line(x - size, y, x - size, y + 10);
    line(x + size, y, x + size, y + 10);
}

// Hàm v? Boss 
void drawBossEnemy(int x, int y) {
    int size = 50;
    
    setcolor(MAGENTA);
    setfillstyle(SOLID_FILL, LIGHTMAGENTA);
    rectangle(x - size, y - size/2, x + size, y + size/2);
    floodfill(x, y, MAGENTA);
    
    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, RED);
    circle(x, y, 15);
    floodfill(x, y, YELLOW);

    // Yêu c?u rubric: S? d?ng arc (V? khiên nang lu?ng d?ng vòng cung)
    setcolor(LIGHTCYAN);
    arc(x, y, 180, 360, size + 10); // Khiên vòng cung phía tru?c m?t Boss
    arc(x, y, 180, 360, size + 15);
}

// Hàm v? Giao di?n (UI)
void drawUI() {
    // 1. Tên Game (Font to, n?i b?t)
    setcolor(YELLOW);
    settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 4);
    outtextxy(SCREEN_WIDTH / 2 - 180, 20, (char*)"CHIEN HAM NGAN HA");

    // 2. B?ng di?m và Sinh m?nh (Góc trái)
    setcolor(LIGHTCYAN);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    outtextxy(10, 10, (char*)"Score: 00000");
    outtextxy(10, 40, (char*)"Lives: 3");

    // 3. Khung k? nang (Góc trái)
    setcolor(LIGHTGRAY);
    rectangle(10, 70, 110, 90);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    outtextxy(10, 100, (char*)"Ultimate: [Chuot Phai]");

    // 4. Hu?ng d?n choi co b?n (Góc ph?i)
    setcolor(LIGHTGREEN);
    outtextxy(SCREEN_WIDTH - 250, 80, (char*)"Dieu khien: A / D");
    outtextxy(SCREEN_WIDTH - 250, 110, (char*)"Ban: [Chuot Trai]");

    // 5. Thông báo ch? (Gi?a màn hình)
    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);
    outtextxy(SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT / 2, (char*)"BAM PHIM BAT KY DE BAT DAU...");
}
