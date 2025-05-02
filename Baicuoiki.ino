#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// Cấu hình chân
#define BUZZER_PIN     PE10
#define IR_SENSOR_PIN  PE12
#define IR_DOOR_SENSOR_PIN  D4  // Cảm biến đặt ở cửa

#define LED_RED_PIN    D3
#define LED_GREEN_PIN  D2

#define SERVO_PIN D5
Servo doorServo;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {PB3, PB5, PC7, PA15};
byte colPins[COLS] = {PB12, PB13, PB15, PC6};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Trạng thái hệ thống
enum State { LOGIN, CONFIRM_CHANGE, CHANGE_PASS };
State state = LOGIN;

String password = "1234";
String inputPassword = "";
bool lcdOn = false;
int attempts = 0;
bool lockout = false;
unsigned long lastBlinkTime = 0;
bool redLedState = false;
bool authenticated = false;
bool doorOpened = false;
unsigned long lastClearTime = 0;
const unsigned long AUTO_CLOSE_DELAY = 5000; // 5 giây

void setLedColor(String color);
void handleAutoClose();

// Setup
void setup() {
  Serial.begin(9600);

  Wire.begin();  // SDA = PB9, SCL = PB8
  lcd.init();
  lcd.backlight();

  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(IR_DOOR_SENSOR_PIN, INPUT);

  setLedColor("red");
  lcd.print("Waiting...");

  doorServo.attach(SERVO_PIN);
  doorServo.write(0);  // Đóng cửa ban đầu
}

// Loop
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
  } else if (!detected && lcdOn) {
    lcdOn = false;
    lcd.noBacklight();
    if (!lockout) setLedColor("red");
  }

  if (lockout) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 500) {
      lastBlinkTime = currentTime;
      redLedState = !redLedState;
      digitalWrite(LED_RED_PIN, redLedState ? LOW : HIGH);
    }
    digitalWrite(LED_GREEN_PIN, HIGH);
  }

  if (!lcdOn) {
    handleAutoClose();  // vẫn kiểm tra đóng cửa
    return;
  }

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
        } else if (key >= '0' && key <= '9') {
          inputPassword += key;
          lcd.setCursor(0, 1);
          for (int i = 0; i < inputPassword.length(); i++) lcd.print("*");

          if (inputPassword.length() == password.length()) {
            if (inputPassword == password) {
              authenticated = true;
              noTone(BUZZER_PIN);
              lockout = false;

              doorServo.write(90);    // Mở cửa
              doorOpened = true;
              lastClearTime = 0;
              lcd.clear();
              lcd.print("WELCOME! TKHOME");
              delay(2000);            // Hiển thị trong 2 giây


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
              lcd.setCursor(0, 1);
              lcd.print("#: Continue");
              state = CONFIRM_CHANGE;
              inputPassword = "";
              attempts = 0;
            } else {
              attempts++;
              lcd.clear();
              lcd.print("Wrong password");

              if (attempts >= 3) {
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
        } else if (key >= '0' && key <= '9') {
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
            authenticated = false;
          }
        }
        break;
    }
  }

  handleAutoClose();
}

// Điều khiển LED RGB dương chung
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

// Tự động đóng cửa sau khi không có người
void handleAutoClose() {
  if (doorOpened) {
    bool objectAtDoor = digitalRead(IR_DOOR_SENSOR_PIN) == LOW;

    if (!objectAtDoor && lastClearTime == 0) {
      lastClearTime = millis();
    }

    if (objectAtDoor) {
      lastClearTime = 0;
    }

    if (lastClearTime > 0 && millis() - lastClearTime >= AUTO_CLOSE_DELAY) {
      doorServo.write(0);     // Đóng cửa
      doorOpened = false;
      lastClearTime = 0;
      lcd.clear();
      lcd.print("Door is CLOSED");
      delay(2000);
      Serial.println("Door closed automatically");
    }
  }
}
