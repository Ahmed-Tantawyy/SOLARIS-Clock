#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define SCLK 9
#define MOSI 10
#define RST 8
#define DC 6
#define CS 7
#define BL 21
#define BEEP 20
#define A 5
#define B 4
#define C 3
#define D 2

// The offsets for the screen
class LCD : public Adafruit_ST7789 {
public:
  LCD() : Adafruit_ST7789(CS, DC, MOSI, SCLK, RST) {}
  void offset() { _colstart = _colstart2 = 82; _rowstart = _rowstart2 = 18; }
} tft;

// The time that the clock opens on
int mode, set, h = 7, m, s, ah = 7, am;
long timer = 300;
bool alarmOn, ringing, timerRunning, watchRunning;
unsigned long watch, watchStart, lastSecond, lastTimer, lastDraw;
int lastAlarm = -1;

// When the switch is pressed
bool press(int pin) {
  static bool old[22];
  static unsigned long changed[22];
  bool down = !digitalRead(pin);
  if (down != old[pin] && millis() - changed[pin] > 25) {
    old[pin] = down;
    changed[pin] = millis();
    return down;
  }
  return false;
}

void number(int n) { if (n < 10) tft.print('0'); tft.print(n); }

void time(long x, bool tenth = false) {
  tft.setTextSize(4); tft.setCursor(32, 26);
  number(x / 60); tft.print(':'); number(x % 60);
  if (tenth) { tft.setTextSize(2); tft.print('.'); tft.print((watch % 1000) / 100); }
}

  // The clock 4 modes on the screen
void draw() {
  const char *title[] = {"CLOCK", "ALARM", "TIMER", "STOPWATCH"};
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(2); tft.setCursor(5, 3); tft.print(title[mode]);
  tft.drawFastHLine(0, 21, tft.width(), ST77XX_YELLOW);
  tft.setTextColor(ST77XX_WHITE);

  if (mode == 0) {
    time(h * 60L + m); tft.setTextSize(1); tft.setCursor(212, 51); number(s); tft.setCursor(5, 65);
    tft.print(set == 1 ? "Set hour" : set == 2 ? "Set minute" : "SW4: set");
  } else if (mode == 1) {
    time(ah * 60L + am); tft.setTextSize(1); tft.setCursor(5, 65);
    tft.setTextColor(alarmOn ? ST77XX_GREEN : ST77XX_RED); tft.print(alarmOn ? "Alarm ON" : "Alarm OFF");
  } else if (mode == 2) {
    time(timer); tft.setTextSize(1); tft.setCursor(5, 65);
    tft.setTextColor(timerRunning ? ST77XX_GREEN : ST77XX_WHITE);
    tft.print(timerRunning ? "Running" : "Paused - SW2/SW3 set");
  } else {
    time((watch + (watchRunning ? millis() - watchStart : 0)) / 1000, true);
    tft.setTextSize(1); tft.setCursor(5, 65); tft.setTextColor(watchRunning ? ST77XX_GREEN : ST77XX_WHITE);
    tft.print(watchRunning ? "Running" : "Paused - SW3 reset");
  }
  if (ringing) {
    tft.fillRect(165, 3, 115, 18, ST77XX_RED); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(1);
    tft.setCursor(170, 8); tft.print("WAKE UP! SW4");
  }
}

void change(int x) {
  if (mode == 0 && set == 1) h = (h + x + 24) % 24;
  if (mode == 0 && set == 2) m = (m + x + 60) % 60;
  if (mode == 1 && set == 1) ah = (ah + x + 24) % 24;
  if (mode == 1 && set == 2) am = (am + x + 60) % 60;
  if (mode == 1 && set == 3) alarmOn = !alarmOn;
  if (mode == 2 && !timerRunning) timer = max(0L, timer + x * 60L);
}

  // The swiches functions
void buttons() {
  if (press(A)) { mode = (mode + 1) % 4; set = 0; ringing = false; noTone(BEEP); }
  if (press(D)) {
    if (ringing) { ringing = false; noTone(BEEP); }
    else if (mode < 2) set = (set + 1) % (mode ? 4 : 3);
    else if (mode == 2) { timerRunning = !timerRunning; lastTimer = millis(); }
    else { if (watchRunning) watch += millis() - watchStart; else watchStart = millis(); watchRunning = !watchRunning; }
  }
  if (press(B)) change(1);
  if (press(C)) { if (mode == 3 && !watchRunning) watch = 0; else change(-1); }
}

// When something is pressed or changed
void update() {
  if (millis() - lastSecond >= 1000) {
    lastSecond += 1000;
    if (++s == 60) { s = 0; if (++m == 60) { m = 0; h = (h + 1) % 24; } }
  }
  int now = h * 60 + m;
  if (alarmOn && h == ah && m == am && s == 0 && lastAlarm != now) ringing = true, lastAlarm = now;
  if (timerRunning && millis() - lastTimer >= 1000) {
    lastTimer += 1000;
    if (timer > 0) timer--;
    if (!timer) timerRunning = false, ringing = true;
  }
  if (ringing && (millis() / 400) % 2 == 0) tone(BEEP, 1800); else noTone(BEEP);
}

void setup() {
  pinMode(BL, OUTPUT); digitalWrite(BL, LOW);
  for (int pin : {A, B, C, D}) pinMode(pin, INPUT_PULLUP);
  pinMode(BEEP, OUTPUT);
  tft.init(76, 284); tft.offset(); tft.setRotation(1); tft.invertDisplay(false);
  lastSecond = lastTimer = millis();
  draw();
}

void loop() {
  buttons(); update();
  unsigned long gap = mode == 3 && watchRunning ? 100 : 1000;
  if (millis() - lastDraw >= gap || ringing) { draw(); lastDraw = millis(); }
  delay(5);
}