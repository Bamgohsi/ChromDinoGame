#include <wiringPi.h>
#include <lcd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

//#define sw1 8
#define sw2 15
//#define sw3 16
#define LCD_RS 25
#define LCD_E 24
#define LCD_D0 29
#define LCD_D1 28
#define LCD_D2 27
#define LCD_D3 26
#define LCD_D4 23
#define LCD_D5 22
#define LCD_D6 21
#define LCD_D7 7

int lcd;
int s = 0;

volatile int jumpFlag = 0;
volatile int frameTick = 0;
int monfr = 85;
int jumpfr = 0;
int delaytick = 3;
int score = 0;
char scoreStr[10];
int gamestate = 0;
int mainstate = 0;

uint8_t crt_a[8] = {0x0A,0x1B,0x15,0x0E,0x15,0x04,0x0E,0x0A};
uint8_t crt_b[8] = {0x0A,0x1B,0x15,0x0E,0x15,0x04,0x0E,0x11};
uint8_t customMon1[8] = {0x00,0x00,0x0A,0x0A,0x15,0x0A,0x0E,0x0A};
uint8_t monster_a[5][8] = {};
uint8_t monster_b[5][8] = {};

uint8_t shifted_a[9][8] = {};
uint8_t shifted_b[9][8] = {};

void monster_set() {
        for(int i = 0; i < 6; i++) {
                for(int j = 0; j < 8; j++) {
                        monster_a[i][j] = customMon1[j] >> i;
                        monster_b[i][j] = customMon1[j] << (5-i);
                }
        }
   for (int i = 0; i < 9; i++) {
      for (int j = 0; j < 8; j++) {
         if(i == 0) shifted_a[0][j] = 0;
         if (j >= 8 - i)   shifted_a[i][j] = crt_a[j - (8 - i)];
         else      shifted_b[i][j] = crt_a[j + i];
      }
   }
}
void setclear() {
   lcdClear(lcd);
   jumpFlag = 0;
   monfr = 85;
   delaytick = 3;
   mainstate = 0;
   score = 0;
}
void lcdspclear(int a, int b) {
   lcdPosition(lcd, a, b);
   lcdPutchar(lcd, ' ');
}
void jump() {
   lcdspclear(2, 0);
   if(jumpfr < 9) {
      lcdCharDef(lcd, 0, shifted_a[jumpfr]);
          lcdCharDef(lcd, 1, shifted_b[jumpfr]);
   }
   else if(jumpfr > 14){
      lcdCharDef(lcd, 0, shifted_a[22-jumpfr]);
      lcdCharDef(lcd, 1, shifted_b[22-jumpfr]);
      if(jumpfr == 22) {
         jumpFlag = 0;
         return;
      }
   }
   lcdPosition(lcd, 2, 1);
       lcdPutchar(lcd, 1);
       lcdPosition(lcd, 2, 0);
   lcdPutchar(lcd, 0);
   jumpfr++;
}
void slice_x() {
   if(monfr) {
      lcdCharDef(lcd, 2, monster_a[monfr%5]);
      lcdCharDef(lcd, 3, monster_b[monfr%5]);
      lcdPosition(lcd, monfr/5 - 1, 1);
      lcdPutchar(lcd, 2);
      lcdPosition(lcd, monfr/5, 1);
      lcdPutchar(lcd, 3);
      if(monfr % 5 == 0) {
         lcdspclear(monfr/5+1, 1);
         lcdspclear(monfr/5, 1);
      }
      monfr--;
   }
}
void normalst() {
   lcdCharDef(lcd, 0, crt_a);
   lcdCharDef(lcd, 1, crt_b);
   lcdPosition(lcd, 2, 1);

   if(s % 2 == 0) {
      lcdPutchar(lcd, 0);
   }
   else {
      lcdPutchar(lcd, 1);
   }
}
void updatePlayer() {
   if((frameTick % (20+delaytick) == 0) && !jumpFlag) {
      normalst();
      s++;
   }
   if(jumpFlag && (frameTick % (1+delaytick) == 0)) {
      jump();
   }
}
void updateMonster() {
   if(frameTick % (1+delaytick) == 0) slice_x();
   if(monfr == 0) {
      lcdspclear(0, 1);
      lcdspclear(1, 1);
      monfr = 85;
   }
}
void updateScore() {
   if(frameTick % (10+delaytick) == 0) {
      sprintf(scoreStr, "%d", score);
      lcdPosition(lcd, 11, 0);
      lcdPuts(lcd, scoreStr);
      score++;
   }
   if(score == 50) delaytick = 1;
   if(score == 102) delaytick = 0;
}

void buttonPressed() {
   static unsigned long previousMillis = 0;
   unsigned long currentMillis = millis();
   if (score < 4) return;
   if(currentMillis - previousMillis >= 180 * delaytick + 800) {
      jumpfr = 0;
      jumpFlag = 1;
      previousMillis = currentMillis;
   }
}
void timerHandler(int signum) {
   frameTick++;
}
void setupFrameTimer() {
   struct sigaction sa;
   struct itimerval timer;

   sa.sa_handler = &timerHandler;
   sa.sa_flags = SA_RESTART;
   sigaction(SIGALRM, &sa, NULL);

   timer.it_value.tv_sec = 0;
   timer.it_value.tv_usec = 16666;
   timer.it_interval.tv_sec = 0;
   timer.it_interval.tv_usec = 16666;

   setitimer(ITIMER_REAL, &timer, NULL);
}
void gamemainhome() {
   static unsigned long previousMillis = 0;
   unsigned long currentMillis = millis();
   int waitStart;
   if(currentMillis - previousMillis >= 500) {
      if (mainstate++ % 2 == 0) lcdClear(lcd);
      else lcdPuts(lcd, " Press   Button    to  Start    ");
      previousMillis = currentMillis;
      }
   if(digitalRead(sw2) == HIGH) {
      gamestate = 1;
      setclear();
      waitStart = frameTick;
      while(frameTick - waitStart < 50) delay(1);
   }
}
void gameover() {
   const char *gameover = "Game Over";
   lcdClear(lcd);
   lcdPosition(lcd, 3, 0);
   int i = 0;
   int lastFrame = -1;
   while(gameover[i] != '\0') {
      if(frameTick % 10 == 0 && frameTick != lastFrame) {
         lastFrame = frameTick;
         lcdPutchar(lcd, gameover[i++]);
      }
   }
   int waitStart = frameTick;
   while(frameTick - waitStart < 70) {
      delay(1);
   }
}
void checkCollision() {
   if((monfr/5 == 2) && (jumpfr < 3 || jumpfr > 20)) {
      gamestate = 0;
      gameover();
   }
}

int main() {
   wiringPiSetup();
   setupFrameTimer();
   lcd = lcdInit(2, 16, 8,
      LCD_RS, LCD_E,
      LCD_D0, LCD_D1, LCD_D2, LCD_D3,
      LCD_D4, LCD_D5, LCD_D6, LCD_D7);
   //pinMode(sw1, INPUT);
   pinMode(sw2, INPUT);
   //pinMode(sw3, INPUT);
   pullUpDnControl(sw2, PUD_UP);
   wiringPiISR(sw2, INT_EDGE_FALLING, &buttonPressed);

   monster_set();
   static unsigned int lastTick = 0;
   while(TRUE) {
      while (gamestate) {
         if(frameTick > lastTick) {
            lastTick = frameTick;
            updatePlayer();
            updateMonster();
            checkCollision();
            updateScore();
         }
      }
      gamemainhome();
   }
}
