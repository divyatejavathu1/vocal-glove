// ================================================================
// ESP32 BLIND ASSIST GLOVE
// BLUETOOTH VOICE COMMAND VERSION
// ABSOLUTE FLEX DIFFERENCE
// ================================================================

#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

// ================================================================
// BLUETOOTH
// ================================================================

BluetoothSerial SerialBT;
#define BLUETOOTH_NAME "BLIND_ASSIST_GLOVE"

// ================================================================
// FLEX SENSOR PINS
// ================================================================

#define FLEX1_PIN 34
#define FLEX2_PIN 35
#define FLEX3_PIN 32
#define FLEX4_PIN 33

// ================================================================
// MPU6050
// ================================================================

#define MPU_ADDR 0x68
#define MPU_SDA 21
#define MPU_SCL 22

#define MPU_THRESHOLD 5000

// ================================================================
// FLEX CALIBRATION
// ================================================================

#define CALIBRATION_READINGS 10
#define FLEX_THRESHOLD_DIFFERENCE 100
#define FLEX_STABLE_TIME 100

// ================================================================
// MPU DIRECTION STABILITY
// ================================================================

#define AXIS_STABLE_TIME 200

// ================================================================
// FLEX VARIABLES
// ================================================================

int flexValue[4] = {0, 0, 0, 0};

float flexReference[4] = {0, 0, 0, 0};

// Stores the absolute difference for each sensor
float flexDifference[4] = {0, 0, 0, 0};

// Stores the absolute difference threshold
float flexTriggerLevel[4] = {0, 0, 0, 0};

bool flexState[4] = {false, false, false, false};

bool previousFlexState[4] = {false, false, false, false};

unsigned long flexChangeStart[4] = {0, 0, 0, 0};

// ================================================================
// MPU VARIABLES
// ================================================================

int16_t accelX = 0;
int16_t accelY = 0;
int16_t accelZ = 0;

// ================================================================
// DIRECTION ENUM
// ================================================================

enum Direction {
  DIR_STABLE,
  DIR_X_POSITIVE,
  DIR_X_NEGATIVE,
  DIR_Y_POSITIVE,
  DIR_Y_NEGATIVE
};

// ================================================================
// DIRECTION VARIABLES
// ================================================================

Direction currentDirection = DIR_STABLE;
Direction detectedDirection = DIR_STABLE;

unsigned long directionChangeTime = 0;

// ================================================================
// BLUETOOTH COMMAND LOCK
// ================================================================

bool commandSending = false;
unsigned long commandTime = 0;

#define COMMAND_LOCK_TIME 1000

// ================================================================
// INITIALIZE MPU6050
// ================================================================

bool initializeMPU() {
  Wire.begin(MPU_SDA, MPU_SCL);
  delay(100);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);

  byte error = Wire.endTransmission(true);

  if (error != 0) {
    Serial.print("MPU6050 ERROR = ");
    Serial.println(error);
    return false;
  }

  // Accelerometer range = +/-2g
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission(true);

  Serial.println("MPU6050 READY");

  return true;
}

// ================================================================
// READ MPU6050
// ================================================================

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  uint8_t count = Wire.requestFrom(
    MPU_ADDR,
    (uint8_t)6,
    (bool)true
  );

  if (count == 6) {
    accelX = (int16_t)(
      ((uint16_t)Wire.read() << 8) | Wire.read()
    );

    accelY = (int16_t)(
      ((uint16_t)Wire.read() << 8) | Wire.read()
    );

    accelZ = (int16_t)(
      ((uint16_t)Wire.read() << 8) | Wire.read()
    );
  }
}

// ================================================================
// READ ALL FLEX SENSORS
// ================================================================

void readFlexSensors() {
  flexValue[0] = analogRead(FLEX1_PIN);
  flexValue[1] = analogRead(FLEX2_PIN);
  flexValue[2] = analogRead(FLEX3_PIN);
  flexValue[3] = analogRead(FLEX4_PIN);

  // ABSOLUTE DIFFERENCE FROM REFERENCE
  for (int i = 0; i < 4; i++) {
    flexDifference[i] = abs(
      (float)flexValue[i] - flexReference[i]
    );
  }
}

// ================================================================
// FLEX SENSOR CALIBRATION
// ================================================================

void calibrateFlexSensors() {
  Serial.println();
  Serial.println("================================================");
  Serial.println("FLEX SENSOR CALIBRATION");
  Serial.println("KEEP ALL FINGERS STRAIGHT / NORMAL");
  Serial.println("DO NOT MOVE THE GLOVE");
  Serial.println("10 READINGS WILL BE TAKEN");
  Serial.println("================================================");

  delay(2000);

  long sum[4] = {0, 0, 0, 0};

  for (int sample = 0; sample < CALIBRATION_READINGS; sample++) {
    readFlexSensors();

    sum[0] += flexValue[0];
    sum[1] += flexValue[1];
    sum[2] += flexValue[2];
    sum[3] += flexValue[3];

    Serial.print("Reading ");
    Serial.print(sample + 1);
    Serial.print("/10");

    Serial.print("   F1=");
    Serial.print(flexValue[0]);

    Serial.print("   F2=");
    Serial.print(flexValue[1]);

    Serial.print("   F3=");
    Serial.print(flexValue[2]);

    Serial.print("   F4=");
    Serial.println(flexValue[3]);

    delay(100);
  }

  // Calculate reference and absolute-difference threshold
  for (int i = 0; i < 4; i++) {
    flexReference[i] =
      (float)sum[i] / CALIBRATION_READINGS;

    flexTriggerLevel[i] =
      FLEX_THRESHOLD_DIFFERENCE;

    flexDifference[i] = 0;
  }

  Serial.println();
  Serial.println("================================================");
  Serial.println("CALIBRATION COMPLETE");
  Serial.println("================================================");

  for (int i = 0; i < 4; i++) {
    Serial.print("FLEX ");
    Serial.print(i + 1);

    Serial.print("  REFERENCE = ");
    Serial.print(flexReference[i], 1);

    Serial.print("  ABS DIFFERENCE THRESHOLD = ");
    Serial.println(flexTriggerLevel[i], 1);
  }

  Serial.println("================================================");

  // Initial state: NOT BENT
  for (int i = 0; i < 4; i++) {
    flexState[i] = false;
    previousFlexState[i] = false;
    flexChangeStart[i] = 0;
  }

  delay(1000);
}

// ================================================================
// CALCULATE FLEX STATE USING ABSOLUTE DIFFERENCE
// ================================================================

bool calculateFlexState(
  int sensorValue,
  float referenceValue
) {
  float difference = abs(
    (float)sensorValue - referenceValue
  );

  return difference > FLEX_THRESHOLD_DIFFERENCE;
}

// ================================================================
// UPDATE FLEX STATES
// ================================================================

void updateFlexStates() {
  for (int i = 0; i < 4; i++) {
    bool newState = calculateFlexState(
      flexValue[i],
      flexReference[i]
    );

    if (newState != flexState[i]) {
      if (flexChangeStart[i] == 0) {
        flexChangeStart[i] = millis();
      }

      if (millis() - flexChangeStart[i] >= FLEX_STABLE_TIME) {
        flexState[i] = newState;
        flexChangeStart[i] = 0;
      }
    } else {
      flexChangeStart[i] = 0;
    }
  }
}

// ================================================================
// DETERMINE MPU DIRECTION
// ================================================================

Direction determineDirection() {
  if (accelX > MPU_THRESHOLD) {
    return DIR_X_POSITIVE;
  }

  if (accelX < -MPU_THRESHOLD) {
    return DIR_X_NEGATIVE;
  }

  if (accelY > MPU_THRESHOLD) {
    return DIR_Y_POSITIVE;
  }

  if (accelY < -MPU_THRESHOLD) {
    return DIR_Y_NEGATIVE;
  }

  return DIR_STABLE;
}

// ================================================================
// UPDATE MPU DIRECTION
// ================================================================

void updateDirection() {
  Direction newDirection = determineDirection();

  if (newDirection != detectedDirection) {
    detectedDirection = newDirection;
    directionChangeTime = millis();
  }

  if (millis() - directionChangeTime >= AXIS_STABLE_TIME) {
    if (currentDirection != detectedDirection) {
      currentDirection = detectedDirection;

      Serial.print("DIRECTION = ");

      switch (currentDirection) {
        case DIR_X_POSITIVE:
          Serial.println("X > +5000");
          break;

        case DIR_X_NEGATIVE:
          Serial.println("X < -5000");
          break;

        case DIR_Y_POSITIVE:
          Serial.println("Y > +5000");
          break;

        case DIR_Y_NEGATIVE:
          Serial.println("Y < -5000 - NO VOICE");
          break;

        case DIR_STABLE:
          Serial.println("STABLE - NO VOICE");
          break;
      }
    }
  }
}

// ================================================================
// GET VOICE COMMAND
// ================================================================

String getVoiceCommand(
  Direction direction,
  int flexIndex
) {
  // X POSITIVE
  if (direction == DIR_X_POSITIVE) {
    if (flexIndex == 0) return "I NEED FOOD";
    if (flexIndex == 1) return "I NEED WATER";
    if (flexIndex == 2) return "I NEED HELP";
    if (flexIndex == 3) return "I NEED MEDICINE";
  }

  // X NEGATIVE
  if (direction == DIR_X_NEGATIVE) {
    if (flexIndex == 0) return "THANK YOU";
    if (flexIndex == 1) return "GOOD MORNING";
    if (flexIndex == 2) return "SORRY";
    if (flexIndex == 3) return "MY ADDRESS";
  }

  // Y POSITIVE
  if (direction == DIR_Y_POSITIVE) {
    if (flexIndex == 0) return "HI";
    if (flexIndex == 1) return "GOOD";
    if (flexIndex == 2) return "BAD";
    if (flexIndex == 3) return "GOOD BYE";
  }

  return "";
}

// ================================================================
// SEND VOICE COMMAND TO MOBILE
// ================================================================

void sendVoiceCommand(String command) {
  if (command.length() == 0) {
    return;
  }

  if (commandSending) {
    if (millis() - commandTime < COMMAND_LOCK_TIME) {
      return;
    }

    commandSending = false;
  }

  if (SerialBT.hasClient()) {
    SerialBT.println(command);
    SerialBT.flush();

    Serial.print("BLUETOOTH COMMAND SENT: ");
    Serial.println(command);
  } else {
    Serial.println("MOBILE NOT CONNECTED");

    Serial.print("COMMAND: ");
    Serial.println(command);
  }

  commandSending = true;
  commandTime = millis();
}

// ================================================================
// SEND WELCOME
// ================================================================

void sendWelcome() {
  Serial.println();
  Serial.println("SENDING WELCOME COMMAND");

  if (SerialBT.hasClient()) {
    SerialBT.println("WELCOME");
    SerialBT.flush();

    Serial.println("WELCOME SENT TO MOBILE");
  } else {
    Serial.println("MOBILE NOT CONNECTED");
    Serial.println("WELCOME WILL NOT BE SPOKEN");
  }

  delay(500);
}

// ================================================================
// PROCESS FLEX GESTURES
// ================================================================

void processFlexGestures() {
  // STABLE = NO VOICE
  if (currentDirection == DIR_STABLE) {
    for (int i = 0; i < 4; i++) {
      previousFlexState[i] = flexState[i];
    }

    return;
  }

  // Y NEGATIVE = NO VOICE
  if (currentDirection == DIR_Y_NEGATIVE) {
    for (int i = 0; i < 4; i++) {
      previousFlexState[i] = flexState[i];
    }

    return;
  }

  for (int i = 0; i < 4; i++) {
    // ONLY NOT BENT -> BENT
    if (flexState[i] && !previousFlexState[i]) {
      String command = getVoiceCommand(
        currentDirection,
        i
      );

      if (command.length() > 0) {
        Serial.println();
        Serial.println("==============================================");

        Serial.print("FLEX SENSOR = ");
        Serial.println(i + 1);

        Serial.print("CURRENT VALUE = ");
        Serial.println(flexValue[i]);

        Serial.print("REFERENCE = ");
        Serial.println(flexReference[i], 1);

        Serial.print("ABSOLUTE DIFFERENCE = ");
        Serial.println(flexDifference[i], 1);

        Serial.print("TRIGGER DIFFERENCE = ");
        Serial.println(FLEX_THRESHOLD_DIFFERENCE);

        Serial.print("VOICE COMMAND = ");
        Serial.println(command);

        Serial.println("==============================================");

        sendVoiceCommand(command);
      }
    }

    // Save state
    previousFlexState[i] = flexState[i];
  }
}

// ================================================================
// PRINT SENSOR VALUES
// ================================================================

void printSensorValues() {
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint < 500) {
    return;
  }

  lastPrint = millis();

  Serial.print("X=");
  Serial.print(accelX);

  Serial.print(" Y=");
  Serial.print(accelY);

  Serial.print(" Z=");
  Serial.print(accelZ);

  for (int i = 0; i < 4; i++) {
    Serial.print(" | F");
    Serial.print(i + 1);

    Serial.print("=");
    Serial.print(flexValue[i]);

    Serial.print(" Diff=");
    Serial.print(flexDifference[i], 1);
  }

  Serial.println();
}

// ================================================================
// SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================================");
  Serial.println("ESP32 BLIND ASSIST GLOVE");
  Serial.println("BLUETOOTH VOICE COMMAND VERSION");
  Serial.println("ABSOLUTE FLEX DIFFERENCE");
  Serial.println("================================================");

  // FLEX SENSOR PINS
  pinMode(FLEX1_PIN, INPUT);
  pinMode(FLEX2_PIN, INPUT);
  pinMode(FLEX3_PIN, INPUT);
  pinMode(FLEX4_PIN, INPUT);

  // START BLUETOOTH
  Serial.println("STARTING BLUETOOTH...");

  if (SerialBT.begin(BLUETOOTH_NAME)) {
    Serial.println("BLUETOOTH STARTED");
    Serial.print("DEVICE NAME: ");
    Serial.println(BLUETOOTH_NAME);
  } else {
    Serial.println("BLUETOOTH START FAILED");
  }

  // MPU6050
  Serial.println("INITIALIZING MPU6050...");

  if (!initializeMPU()) {
    Serial.println("WARNING: MPU6050 NOT FOUND");
  }

  // FLEX CALIBRATION
  calibrateFlexSensors();

  // INITIAL MPU READING
  readMPU();

  // INITIAL DIRECTION
  currentDirection = DIR_STABLE;
  detectedDirection = DIR_STABLE;
  directionChangeTime = millis();

  // SEND WELCOME
  sendWelcome();

  // READY
  Serial.println();
  Serial.println("================================================");
  Serial.println("SYSTEM READY");
  Serial.println("================================================");

  Serial.println("BLUETOOTH DEVICE:");
  Serial.println(BLUETOOTH_NAME);

  Serial.println();
  Serial.println("FLEX REFERENCES:");

  for (int i = 0; i < 4; i++) {
    Serial.print("FLEX ");
    Serial.print(i + 1);

    Serial.print(" REF=");
    Serial.print(flexReference[i], 1);

    Serial.print(" ABS DIFFERENCE THRESHOLD=");
    Serial.println(FLEX_THRESHOLD_DIFFERENCE);
  }

  Serial.println();
  Serial.println("X > +5000 -> FOOD/WATER/HELP/MEDICINE");
  Serial.println("X < -5000 -> THANK YOU/GOOD MORNING/SORRY/ADDRESS");
  Serial.println("Y > +5000 -> HI/GOOD/BAD/GOOD BYE");
  Serial.println("Y < -5000 -> NO VOICE");
  Serial.println("STABLE -> NO VOICE");

  Serial.println();
  Serial.println("================================================");
  Serial.println("CONNECT MOBILE TO:");
  Serial.println("BLIND_ASSIST_GLOVE");
  Serial.println("================================================");
}

// ================================================================
// LOOP
// ================================================================

void loop() {
  // Read MPU
  readMPU();

  // Read flex sensors and calculate absolute differences
  readFlexSensors();

  // Update flex states
  updateFlexStates();

  // Update MPU direction
  updateDirection();

  // Process gestures
  processFlexGestures();

  // Serial monitor
  printSensorValues();

  delay(20);
}
