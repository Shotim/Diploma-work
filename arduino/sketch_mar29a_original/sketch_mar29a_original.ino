#include <Servo.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>

#define ROWS 4
#define COLUMNS 3

#define ECHO_PIN 10
#define TRIG_PIN 6

#define SOUND_PIN 4
#define SERVO_PIN 5

#define FINGER_PRINT_IN 8
#define FINGER_PRINT_OUT 9

SoftwareSerial fingerPrintSerial(9, 8);
Adafruit_Fingerprint fingerPrint = Adafruit_Fingerprint(&fingerPrintSerial);
uint8_t id;

char initialPassword[4] = "1111";
char initialSuperUserCode[8] = "14663246";

char inputPassword[4];
char inputSuperUserCode[8];
char inputBuffer;
int wrongTries = 0;
long duration = 500;
boolean isDoorOpened = false;

char keys[ROWS][COLUMNS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte columnPins[COLUMNS] = {11, 12, 13};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, columnPins, ROWS, COLUMNS );
LiquidCrystal_I2C lcd(0x20, 20, 4);
Servo servo;

void setup() {

  fingerPrint.begin(57600);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  servo.attach(SERVO_PIN);
  servo.write(0);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  if (fingerPrint.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
}
}

void loop() {
  do {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    delay(250);
  } while (duration < 500);
  duration = 500;
  mainAlgorithm();
}

void mainAlgorithm() {
  do {
    if (wrongTries == 3) {
      lcd.clear();
      lcd.print("Change password!");
      changePasswordIfCodeIsCorrect();
    } else {
      lcd.clear();
      lcd.print("Enter password");
      lcd.setCursor(0, 1);
      lcd.print("(");
      lcd.print(3 - wrongTries);
      lcd.print(" tries left)");
      inputBuffer = keypad.waitForKey();
      lcd.clear();
      lcd.setCursor(0, 0);
      if(inputBuffer == '#'){
        getFingerprintEnrollIfCodeIsCorrect();
      } else if (inputBuffer == '*') {
        changePasswordIfCodeIsCorrect();
      } else {
        if(FINGERPRINT_OK == getFingerprintID()){
          openDoor();
          wrongTries = 0;
        } else {
          wrongTries++;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong password");
          tone(SOUND_PIN, 2000, 1000);
          delay(1000);
        }
        enterPassword(inputBuffer);
        if (isPasswordCorrect()) {
          openDoor();
          wrongTries = 0;
        } else {
          wrongTries++;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong password");
          tone(SOUND_PIN, 2000, 1000);
          delay(1000);
        }
      }
    }
  } while (isDoorOpened);
  isDoorOpened = false;
}

void changePasswordIfCodeIsCorrect() {
  boolean isPasswordChanged = false;
  while (!isPasswordChanged) {
    enterSuperUserCode();
    if (isSuperUserCodeCorrect()) {
      lcd.clear();
      lcd.print("Enter old");
      lcd.setCursor(0,1);
      lcd.print("password:");
      inputBuffer = keypad.waitForKey();
      lcd.clear();
      lcd.setCursor(0, 0);
      enterPassword(inputBuffer);
      if (isPasswordCorrect()) {
        lcd.clear();
        lcd.print("Change password:");
        changePassword();
        wrongTries = 0;
        isPasswordChanged = true;
      } 
    } else {
      lcd.clear();
      lcd.print("Wrong code.");
      lcd.setCursor(0, 1);
      lcd.print("Try again!!!");
      delay(3000);
    }
  }

}

void openDoor() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Open.");
  servo.write(180);
  tone(SOUND_PIN, 400, 750);
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Close.");
  servo.write(0);
  tone(SOUND_PIN, 300, 700);
  delay(1000);
}

void enterPassword(char firstSymbol) {
  inputPassword[0] = firstSymbol;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("*");
  for (int counter = 1; counter < 4; counter++) {
    inputPassword[counter] = keypad.waitForKey();
    lcd.print("*");
  }
}

boolean isPasswordCorrect() {

  for (int counter = 0; counter < 4; counter++) {
    if (initialPassword[counter] != inputPassword[counter]) {
      return false;
    }
  }
  return true;
}

void enterSuperUserCode() {
  lcd.clear();
  lcd.print("Enter super-user");
  lcd.setCursor(0, 1);
  lcd.print("password!!!");
  for (int counter = 0; counter < 8; counter++) {
    inputSuperUserCode[counter] = keypad.waitForKey();
    if (counter == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    lcd.print("*");
  }
}

boolean isSuperUserCodeCorrect() {
  for (int counter = 0; counter < 8; counter++) {
    if (initialSuperUserCode[counter] != inputSuperUserCode[counter]) {
      return false;
    }
  }
  return true;
}

void changePassword() {
  for (int counter = 0; counter < 4; counter++) {
    initialPassword[counter] = keypad.waitForKey();
    if (counter == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    lcd.print("*");
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Password changed");
}

uint8_t getFingerprintEnrollIfCodeIsCorrect() {
  enterSuperUserCode();
  if(isSuperUserCodeCorrect()){
    return getFingerprintEnroll();
  } else {
    lcd.clear();
    lcd.print("Wrong code.");
    lcd.setCursor(0, 1);
    lcd.print("Try again!!!");
    delay(3000);
  }
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = fingerPrint.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = fingerPrint.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = fingerPrint.getImage();
  }
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = fingerPrint.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = fingerPrint.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!

  p = fingerPrint.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  p = fingerPrint.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

uint8_t getFingerprintID() {
  uint8_t p = fingerPrint.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = fingerPrint.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = fingerPrint.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(fingerPrint.fingerID);
  Serial.print(" with confidence of "); Serial.println(fingerPrint.confidence);

  return fingerPrint.fingerID;
}
