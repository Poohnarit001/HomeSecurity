#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

constexpr uint8_t I2CADDR = 0x20;
constexpr uint8_t RIGHT_WINDOW_SERVO_PIN = 4;
constexpr uint8_t LEFT_WINDOW_SERVO_PIN = 5;
constexpr uint8_t DOOR_SERVO_PIN = 6;
constexpr uint8_t SOUND_PIN = 7;
constexpr uint8_t SENSOR_PIN = 8;
constexpr uint8_t RST_PIN = 9;
constexpr uint8_t SS_PIN = 10;
constexpr uint8_t SMOKE_PIN = 14;

MFRC522 rfid(SS_PIN, RST_PIN); 

Servo doorServo, leftWindowServo, rightWindowServo;
bool doorOpen = false;
bool leftWindowOpen = false;
bool rightWindowOpen = false;

const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 7, 6, 5, 4 };
byte colPins[COLS] = { 3, 2, 1, 0 };
Keypad_I2C keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);

LiquidCrystal_I2C lcd(0x27, 16, 2);

String inputNumber = "";
bool C_pressed = false;

const String correctPassword = "1234";
String tag = "";
unsigned long startTime = 0;
unsigned long lastTime = 0;
unsigned long doorCloseTime = 0;

tmElements_t tm;
int Hour, Minute, Second;
int Year, Month, Day;

enum Mode {
  SHOW_TIME,
  ENTER_PASSWORD
};
Mode currentMode = SHOW_TIME;

void print2digit(int val) {
  if (val < 10 && val >= 0) lcd.print(0);
  lcd.print(val);
}

void showDateTime() {
  if (RTC.read(tm)) {
    Day = tm.Day;
    Month = tm.Month;
    Year = tmYearToCalendar(tm.Year);
    Hour = tm.Hour;
    Minute = tm.Minute;
    Second = tm.Second;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("D/M/Y=");
    print2digit(Day); lcd.print("/"); print2digit(Month); lcd.print("/"); lcd.print(Year);
    
    lcd.setCursor(0, 1);
    lcd.print("Time = ");
    print2digit(Hour); lcd.print(":"); print2digit(Minute); lcd.print(":"); print2digit(Second);
  }
}

void openDoor() {
  doorServo.write(100);
  doorOpen = true;
}

void closeDoor() {
  doorServo.write(185);
  doorOpen = false;
  doorCloseTime = millis();
}

void openLeftWindow() {
  leftWindowServo.write(100);
  leftWindowOpen = true;
}

void closeLeftWindow() {
  leftWindowServo.write(185);
  leftWindowOpen = false;
}

void openRightWindow() {
  rightWindowServo.write(15);
  rightWindowOpen = true;
}

void closeRightWindow() {
  rightWindowServo.write(100);
  rightWindowOpen = false;
}

void unlockDoor() {
  openDoor();
  delay(7000);
  wdt_reset();
  closeDoor();
}

void resetRFID() {
    rfid.PCD_Init();  // ทำการรีเซ็ตและเริ่มต้นโมดูล RFID ใหม่
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  keypad.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press A, B or C");

  SPI.begin();
  rfid.PCD_Init();

  doorServo.attach(DOOR_SERVO_PIN);
  leftWindowServo.attach(LEFT_WINDOW_SERVO_PIN);
  rightWindowServo.attach(RIGHT_WINDOW_SERVO_PIN);

  doorServo.write(185);  
  leftWindowServo.write(185);  
  rightWindowServo.write(100);  

  pinMode(SENSOR_PIN, INPUT);
  pinMode(SOUND_PIN, OUTPUT);
  pinMode(SMOKE_PIN, INPUT);
  digitalWrite(SOUND_PIN, LOW);

  wdt_enable(WDTO_8S);
}

void loop() {

  wdt_reset();
  resetRFID();
  
  char key = keypad.getKey();
  if (key) {
      digitalWrite(SOUND_PIN, HIGH); 
      delay(100); 
      digitalWrite(SOUND_PIN, LOW);

      if (key == 'A') {
          currentMode = SHOW_TIME;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Showing Time:");
          delay(1000);
      } else if (key == 'B') {
          currentMode = ENTER_PASSWORD;
          inputNumber = "";
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Enter Password:");
      } else if (currentMode == ENTER_PASSWORD && isdigit(key)) {
          inputNumber += key;
          String maskedInput = "";
          for (int i = 0; i < inputNumber.length(); i++) {
              maskedInput += '*';  
          }

          lcd.setCursor(0, 1);
          lcd.print(maskedInput); 

          if (inputNumber.length() == 4) {
              delay(500);
              lcd.clear();
              if (inputNumber == correctPassword) {
                  lcd.setCursor(0, 0);
                  lcd.print("PASSWORD CORRECT");
                  unlockDoor();
              } else {
                  lcd.setCursor(0, 0);
                  lcd.print("PASSWORD INCORRECT");
              }
              delay(1000);
              currentMode = SHOW_TIME;
              inputNumber = "";
          }
      } else if (key == '*') {
          inputNumber = "";
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(currentMode == ENTER_PASSWORD ? "Enter Password:" : "Press A, B or C");
      }
  }

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');

    if (command == "OPEN_DOOR" && !doorOpen) openDoor();
    else if (command == "CLOSE_DOOR" && doorOpen) closeDoor();
    else if (command == "OPEN_LEFT_WINDOW" && !leftWindowOpen) openLeftWindow();
    else if (command == "CLOSE_LEFT_WINDOW" && leftWindowOpen) closeLeftWindow();
    else if (command == "OPEN_RIGHT_WINDOW" && !rightWindowOpen) openRightWindow();
    else if (command == "CLOSE_RIGHT_WINDOW" && rightWindowOpen) closeRightWindow();
  }

  int smokeValue = analogRead(SMOKE_PIN);
  Serial.print("Smoke = ");
  Serial.println(smokeValue);

  if (smokeValue > 300) {
    digitalWrite(SOUND_PIN, HIGH); 
    delay(500); 
    digitalWrite(SOUND_PIN, LOW); 
    delay(500);
  }

  unsigned long currentTime = millis();
  int sensorValue = digitalRead(SENSOR_PIN);

  if (sensorValue == HIGH && !doorOpen && (currentTime - doorCloseTime > 1000) && (currentTime - startTime > 5000 || currentMode == ENTER_PASSWORD)) {
    Serial.println("Alert");
    digitalWrite(SOUND_PIN, HIGH); 
    delay(100); 
    digitalWrite(SOUND_PIN, LOW); 
    delay(100);
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String tag = "";  
      for (byte i = 0; i < rfid.uid.size; i++) {
        tag += String(rfid.uid.uidByte[i]);
      }
      
      if (tag == "51130166149") {  // ตรวจสอบ UID ที่ถูกต้อง
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PASSWORD CORRECT");
        unlockDoor(); 
        startTime = millis();  
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PASSWORD INCORRECT");
        startTime = millis();
      }

      rfid.PICC_HaltA();  // เพิ่มฟังก์ชันเพื่อหยุดการทำงานของ RFID หลังจากการอ่านเสร็จสิ้น
      rfid.PCD_StopCrypto1();  // ปิดการทำงานของการเข้ารหัสเพื่อรีเซ็ต

      delay(1000);  // เพิ่มการหน่วงเวลาเพื่อให้ระบบพร้อมก่อนการอ่านครั้งถัดไป
  }

  if (currentMode == SHOW_TIME && currentTime - lastTime > 1000) {
    showDateTime();
    lastTime = currentTime;
  }

  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
}
