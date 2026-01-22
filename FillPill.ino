#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <RTClib.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RTC
RTC_DS3231 rtc;

// GSM module
SoftwareSerial gsm(7, 8);  // RX=7, TX=8

// Buttons & buzzer
int setButton = A0;
int lidButton = A1;
int buzzer = 9;

// LEDs
int greenLED = 10;
int redLED = 11;

// Timer variables
unsigned long countdownStart = 0;
unsigned long remainingTime = 0;
unsigned long timerSeconds = 0;

bool countdownActive = false;
bool alarmActive = false;
unsigned long alarmStartTime = 0;

void setup() {

  Wire.begin();
  rtc.begin();
  gsm.begin(9600);

  pinMode(setButton, INPUT_PULLUP);
  pinMode(lidButton, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("FILLPILL READY");
  delay(1500);

  lcd.clear();
  lcd.print("Set the timer..");
  lcd.setCursor(0, 1);
  lcd.print("1 click = 10 sec");
}

void loop() {

  // --------------------------------------------------------
  //   1) MULTI-CLICK SET BUTTON (each click = 10 sec)
  // --------------------------------------------------------
  if (!countdownActive && !alarmActive) {

    if (digitalRead(setButton) == LOW) {

      timerSeconds += 10;     // ✔ 1 click = 10 seconds
      remainingTime = timerSeconds;

      lcd.clear();
      lcd.print("Time Set:");
      lcd.setCursor(0, 1);
      lcd.print(timerSeconds);
      lcd.print(" sec");

      delay(300); // debounce

      // WAIT for more clicks
      unsigned long waitStart = millis();
      while (millis() - waitStart < 1200) {
        if (digitalRead(setButton) == LOW) {

          timerSeconds += 10;   // ✔ each extra click adds 10 seconds
          remainingTime = timerSeconds;

          lcd.clear();
          lcd.print("Time Set:");
          lcd.setCursor(0,1);
          lcd.print(timerSeconds);
          lcd.print(" sec");

          delay(300);
          waitStart = millis();  // reset wait timer
        }
      }

      // When user stops clicking → start countdown
      countdownActive = true;
      countdownStart = millis();

      lcd.clear();
      lcd.print("Countdown...");
      delay(500);
    }
  }

  // --------------------------------------------------------
  //   2) LIVE COUNTDOWN
  // --------------------------------------------------------
  if (countdownActive && !alarmActive) {

    unsigned long elapsed = (millis() - countdownStart) / 1000;
    long timeLeft = remainingTime - elapsed;

    if (timeLeft >= 0) {
      lcd.clear();
      lcd.print("Time Left:");
      lcd.setCursor(0, 1);
      lcd.print(timeLeft);
      lcd.print(" sec");
      delay(200);
    }

    // EARLY OPEN
    if (digitalRead(lidButton) == LOW) {

      digitalWrite(redLED, HIGH);

      lcd.clear();
      lcd.print("EARLY OPEN!");
      lcd.setCursor(0, 1);
      lcd.print("Wait 5 sec");

      tone(buzzer, 1500);
      delay(5000);
      noTone(buzzer);

      digitalWrite(redLED, LOW);

      if (elapsed >= remainingTime)
        remainingTime = timerSeconds;
      else
        remainingTime -= elapsed;

      countdownStart = millis();
      return;
    }

    // TIMER FINISHED → START ALARM
    if (timeLeft <= 0) {

      lcd.clear();
      lcd.print("TIME TO TAKE");
      lcd.setCursor(0, 1);
      lcd.print("MEDICINE!");

      alarmActive = true;
      alarmStartTime = millis();
      countdownActive = false;

      digitalWrite(greenLED, HIGH);  // GREEN LED ON
    }
  }

  // --------------------------------------------------------
  //   3) ALARM MODE (2-minute beep)
  // --------------------------------------------------------
  if (alarmActive) {

    tone(buzzer, 1000);
    delay(200);
    noTone(buzzer);
    delay(200);

    // ✔ CORRECT OPEN
    if (digitalRead(lidButton) == LOW) {

      lcd.clear();
      lcd.print("Medicine");
      lcd.setCursor(0, 1);
      lcd.print("Taken!");

      stopAlarm();
      restartTimer();

      digitalWrite(greenLED, LOW);
      delay(300);
      return;
    }

    // ❌ No open → SEND SMS
    if (millis() - alarmStartTime >= 30000) {

      lcd.clear();
      lcd.print("Sending SMS...");
      lcd.setCursor(0, 1);
      lcd.print("Not Taken!");

      sendSMS("Medicine NOT taken!");

      stopAlarm();
      restartTimer();
      return;
    }
  }
}

void stopAlarm() {
  noTone(buzzer);
  alarmActive = false;
  digitalWrite(greenLED, LOW);
}

void restartTimer() {
  countdownActive = true;
  countdownStart = millis();
  remainingTime = timerSeconds;

  lcd.clear();
  lcd.print("New Timer:");
  lcd.setCursor(0, 1);
  lcd.print(timerSeconds);
  lcd.print(" sec");
}

void sendSMS(String msg) {
  gsm.println("AT+CMGF=1");
  delay(300);

  gsm.println("AT+CMGS=\"+910000000000\""); // <-- YOUR NUMBER
  delay(300);

  gsm.println(msg);
  delay(200);

  gsm.write(26);  // CTRL+Z
  delay(500);

  lcd.clear();
  lcd.print("SMS SENT!");
}
