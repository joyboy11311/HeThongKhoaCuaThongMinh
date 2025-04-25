#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define BUZZER_PIN 12
#define IR_SENSOR_PIN 5

// LED RGB (dương chung)
#define LED_RED_PIN 15
#define LED_GREEN_PIN 16

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 27, 26, 25};
byte colPins[COLS] = {33, 32, 4, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Trạng thái
enum State { LOGIN, CONFIRM_CHANGE, CHANGE_PASS };
State state = LOGIN;

String password = "1234";
String inputPassword = "";
bool lcdOn = false;
int attempts = 0;
bool lockout = false;
unsigned long lastBlinkTime = 0;
bool redLedState = false;
bool authenticated = false; // ✅ Thêm biến toàn cục

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  setLedColor("red");
  lcd.print("Waiting...");
}

void loop() {
  bool detected = digitalRead(IR_SENSOR_PIN) == LOW;

  if (detected && !lcdOn) {
    lcdOn = true;
    lcd.backlight();
    lcd.clear();
    lcd.print("Enter password:");
    inputPassword = "";
    state = LOGIN;
    if (!lockout) setLedColor("green");
  } 
  else if (!detected && lcdOn) {
    lcdOn = false;
    lcd.noBacklight();
    if (!lockout) setLedColor("red");
  }

  // Nếu bị khóa, LED đỏ nhấp nháy
  if (lockout) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 500) {
      lastBlinkTime = currentTime;
      redLedState = !redLedState;
      digitalWrite(LED_RED_PIN, redLedState ? LOW : HIGH); // nhấp nháy
    }
    digitalWrite(LED_GREEN_PIN, HIGH); // Tắt LED xanh
  }

  if (!lcdOn) return;

  char key = keypad.getKey();
  if (key) {
    switch (state) {
  case LOGIN:
  if (key == '#') {
    if (inputPassword.length() > 0) {
      inputPassword.remove(inputPassword.length() - 1);
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      for (int i = 0; i < inputPassword.length(); i++) lcd.print("*");
    }
  }
  else if (key >= '0' && key <= '9') {
    inputPassword += key;
    lcd.setCursor(0, 1);
    for (int i = 0; i < inputPassword.length(); i++) lcd.print("*");

    if (inputPassword.length() == password.length()) {
      if (inputPassword == password) {
        authenticated = true;
        noTone(BUZZER_PIN);
        lockout = false;

        for (int i = 0; i < 6; i++) {
          digitalWrite(LED_GREEN_PIN, i % 2 == 0 ? LOW : HIGH);
          delay(150);
        }
        setLedColor("green");

        lcd.clear();
        lcd.print("Access granted!");
        delay(2000);
        lcd.clear();
        lcd.print("*: Change pass");
        lcd.setCursor(0,1);
        lcd.print("#: Continue");
        state = CONFIRM_CHANGE;
        inputPassword = "";
        attempts = 0;
      } else {
        attempts++;
        lcd.clear();
        lcd.print("Wrong password");

        if (attempts >= 5) {
          lcd.setCursor(0, 1);
          lcd.print("ALARM!");
          tone(BUZZER_PIN, 1000);
          lockout = true;
          setLedColor("off");
        }

        delay(2000);
        lcd.clear();
        lcd.print("Enter password:");
        inputPassword = "";
      }
    }
  }
  break;


  case CONFIRM_CHANGE:
    if (key == '*') {
      inputPassword = "";
      lcd.clear();
      lcd.print("New password:");
      state = CHANGE_PASS;
    } else if (key == '#') {
      lcd.clear();
      lcd.print("Enter password:");
      inputPassword = "";
      state = LOGIN;
    }
    break;

  case CHANGE_PASS:
    if (key == '#') {
      if (inputPassword.length() > 0) {
        inputPassword.remove(inputPassword.length() - 1);
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        for (int i = 0; i < inputPassword.length(); i++) lcd.print("*");
      }
    }
    else if (key >= '0' && key <= '9') {
      inputPassword += key;
      lcd.setCursor(0, 1);
      for (int i = 0; i < inputPassword.length(); i++) lcd.print("*");

      if (inputPassword.length() >= 4) {
        password = inputPassword;
        lcd.clear();
        lcd.print("Pass changed!");
        delay(2000);
        lcd.clear();
        lcd.print("Enter password:");
        inputPassword = "";
        state = LOGIN;
        authenticated = false; // ✅ Reset sau khi đổi
      }
    }
    break;
    }
  }
}

// Điều khiển LED dương chung: LOW = bật, HIGH = tắt
void setLedColor(String color) {
  if (color == "red") {
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
  } else if (color == "green") {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
  } else if (color == "off") {
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, HIGH);
  }
}
