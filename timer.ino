#include <LiquidCrystal.h>

#define WITH_SHIELD_BUTTONS

/**
 * Pins configuration
 */
const byte
  LCD_RS = 8, LCD_EN = 9,
  LCD_DB4 = 4, LCD_DB5 = 5, LCD_DB6 = 6, LCD_DB7 = 7,
  LCD_BACKLIGHT = 10,
  BUZZER = 11,
  BUTTON_START = 13,
  BUTTON_STOP = 12;

/**
 * Options
 */
const byte COUNT_DOWN_NUMBER = 5;
const unsigned long AWAKE_TIMEOUT = 1111;

/**
 * Timer
 */
unsigned long markTime, elapsedTime;

void startTimer() {
  elapsedTime = 0;
  markTime = millis();
}
void readTime() {
  elapsedTime = millis() - markTime;
}

/**
 * Buttons
 */
struct Buttons {
  bool startPressed;
  bool stopPressed;
};
Buttons buttons = { false, false };

void setupButtons() {
  pinMode(BUTTON_START, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);
}
void readButtons() {
  buttons.startPressed = digitalRead(BUTTON_START) == LOW;
  buttons.stopPressed = digitalRead(BUTTON_STOP) == LOW;

  #ifdef WITH_SHIELD_BUTTONS
  const int adc_key = analogRead(0);
  buttons.startPressed = (adc_key > 250 && adc_key < 450);
  buttons.stopPressed = (adc_key > 50 && adc_key < 250);
  #endif
}

/**
 * Display
 */
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);

void setupDisplay() {
  lcd.begin(16, 2);
  pinMode(LCD_BACKLIGHT, INPUT); // analogWrite(LCD_BACKLIGHT, 100);
  lcd.setCursor(4, 0);
  lcd.print(F("R E A D Y"));
}

const char ON = '*', OFF = ' ';
void displayButtons(const bool start, const bool stop) {
  lcd.setCursor(0, 0);
  lcd.print(stop ? ON : OFF);
  lcd.setCursor(0, 1);
  lcd.print(start ? ON : OFF);
}

void displayTimeOnLine(const unsigned long time, const byte line) {
  const unsigned int dseconds = time / 1000;
  const unsigned int dcseconds = (time % 1000) / 10;
  lcd.setCursor(10, line);
  if (dseconds < 100) lcd.print(OFF);
  else lcd.print(dseconds / 100);
  lcd.setCursor(11, line);
  if (dseconds < 10) lcd.print(OFF);
  else lcd.print((dseconds % 100) / 10);
  lcd.setCursor(12, line);
  lcd.print(dseconds % 10);
  lcd.setCursor(13, line);
  lcd.print('.');
  lcd.setCursor(14, line);
  lcd.print(dcseconds < 10 ? 0 : dcseconds / 10);
  lcd.setCursor(15, line);
  lcd.print(dcseconds % 10);
}
unsigned long lastDisplayedTime;
void displayTime(const unsigned long elapsed) {
  displayTimeOnLine(elapsed, 1);
}
void displayDelay(const unsigned long delayed) {
  displayTimeOnLine(delayed, 0);
}

void displayCountDown(const byte seconds) {
  lcd.setCursor((2 * seconds) + 2, 0);
  lcd.print(OFF);
}

void displayFalseStart(const unsigned long remaining) {
  displayTimeOnLine(remaining, 0);
  lcd.setCursor(4, 0);
  lcd.print(F("FALSE  -"));
  lcd.setCursor(4, 1);
  lcd.print(F("START"));
}

void displayFinal(const unsigned long elapsed) {
  lcd.setCursor(4, 0);
  lcd.print(F("start:"));
  lcd.setCursor(6, 1);
  lcd.print(F("TOP:"));
  displayTimeOnLine(elapsed, 1);
}

/**
 * Sound
 */
enum SoundDuration { SHORT, LONG, VERY_LONG };
void sound(const int duration) {
  tone(BUZZER, 1000, (
    duration == SHORT ? 500 : (duration == LONG ? 1000 : 2000)
  ));
}

/**
 * State
 */
enum State { READY, AWAKE, COUNTING_DOWN, RUNNING, DONE };
State state;

void onReady() {
  if (buttons.startPressed) {
    startTimer();
    state = AWAKE;
  }
}

void onAwake() {
  if (buttons.startPressed) {
    if (elapsedTime > AWAKE_TIMEOUT) {
      startTimer();
      state = COUNTING_DOWN;
    }
  } else {
    state = READY;
  }
}

unsigned long lastCountDownSeconds = COUNT_DOWN_NUMBER + 1;
void onCountingDown() {
  if (buttons.startPressed) {
    unsigned int seconds = elapsedTime / 1000;
    if (seconds == lastCountDownSeconds) return;
    displayCountDown(seconds);
    lastCountDownSeconds = seconds;
    if (seconds == (COUNT_DOWN_NUMBER - 1) || seconds == (COUNT_DOWN_NUMBER - 2)) {
      sound(LONG);
    } else if (seconds == COUNT_DOWN_NUMBER) {
      startTimer();
      state = RUNNING;
      sound(SHORT);
    }
  } else {
    onFalseStart();
  }
}

void onFalseStart() {
  displayFalseStart((COUNT_DOWN_NUMBER * 1000) - elapsedTime);
  sound(VERY_LONG);
  state = DONE;
}

void onRunning() {
  displayTime(elapsedTime);
  if (buttons.stopPressed) {
    state = DONE;
    displayFinal(elapsedTime);
  } else if (buttons.startPressed) {
    displayDelay(elapsedTime);
  }
}

/** */
void setup() {
  setupDisplay();
  setupButtons();
  state = READY;
}

/** */
void loop() {
  if (state == AWAKE || state == COUNTING_DOWN || state == RUNNING) {
    readTime();
  }

  readButtons();
  displayButtons(buttons.startPressed, buttons.stopPressed);

  if (state == READY) onReady();
  else if (state == AWAKE) onAwake();
  else if (state == COUNTING_DOWN) onCountingDown();
  else if (state == RUNNING) onRunning();

  delay(state == READY || state == DONE ? 333 : 33);
}
