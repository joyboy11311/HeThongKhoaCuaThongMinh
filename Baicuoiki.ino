#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define BUZZER_PIN 12

// Keypad setup
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

// IR sensors
#define IR1_PIN 5

// State enum
enum State { LOGIN, CONFIRM_CHANGE, CHANGE_PASS };
State state = LOGIN;

String password = "1234";
String inputPassword = "";
bool accessGranted = false;
int attempts = 0;
bool lcdOn = false;

String getMaskedString(int len) {
  String str = "";
  for (int i = 0; i < len; i++) str += '*';
  return str;
}

void setup() {
  Serial.begin(9600);
  pinMode(IR1_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.print("Waiting...");
}

void loop() {
  // Cảm biến bật LCD
  bool detected = digitalRead(IR1_PIN) == LOW;
  
  if (detected && !lcdOn) {
    lcdOn = true;
    lcd.backlight();
    lcd.clear();
    lcd.print("Enter password:");
    inputPassword = "";
    state = LOGIN;
  } 
  else if (!detected && lcdOn) {
    lcdOn = false;
    lcd.noBacklight();
  }

  if (!lcdOn) return; // LCD đang tắt thì bỏ qua phần còn lại

  char key = keypad.getKey();
  if (key) {
    switch (state) {
      case LOGIN:
        if (key == '#') {
          lcd.clear();
          lcd.print("Change password?");
          lcd.setCursor(0, 1);
          lcd.print("*:Yes  #:No");
          state = CONFIRM_CHANGE;
        } 
        else if (key == 'B') {
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
              lcd.clear();
              lcd.print("Access granted!");
              accessGranted = true;
              delay(2000);
              lcd.clear();
              lcd.print("Enter password:");
              accessGranted = false;
            } else {
              attempts++;
              lcd.clear();
              lcd.print("Wrong password");
              if (attempts >= 5) {
                digitalWrite(BUZZER_PIN, HIGH);
                delay(3000);
                digitalWrite(BUZZER_PIN, LOW);
              }
              delay(2000);
              lcd.clear();
              lcd.print("Enter password:");
            }
            inputPassword = "";
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
        if (key == 'B') {
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
          }
        }
        break;
    }
  }
}

