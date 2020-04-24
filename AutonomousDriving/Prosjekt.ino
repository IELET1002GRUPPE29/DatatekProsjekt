/////////////////////////////////////////////////////
// Libraries
/////////////////////////////////////////////////////

#include <Zumo32U4.h>
#include "TurnSensor.h"

L3G gyro;
Zumo32U4LCD lcd;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4ButtonC buttonC;
Zumo32U4Encoders encoders;

/////////////////////////////////////////////////////
// Constants and variables
/////////////////////////////////////////////////////

const int defaultSpeed = 100;
int32_t angle = 0;
int mode_index = 0;

/////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////

// Turns gyro-value into degrees
int32_t getAngle() {
  // turnAngle is a variable defined in TurnSensor.cpp
  return (((int32_t)turnAngle >> 16) * 360) >> 16;
}

// Calibrates Gyro
void calibrateGyro() {
  turnSensorSetup();
  delay(500);
  turnSensorReset();
  lcd.clear();
}

// Select mode
void selectMode() {
  char modes[5][8] = {"Square", "Circle", "Forward", "Jiggle"};

  // Update the display
  lcd.gotoXY(0, 0);
  lcd.print("Select:");
  lcd.gotoXY(0, 1);
  lcd.print(modes[mode_index]);

  // "Start" button
  if (buttonB.isPressed()) {
    lcd.clear();

    for (int i = 3; i > 0; i--) {
      lcd.gotoXY(0, 0);
      lcd.print("Starting");
      lcd.gotoXY(0, 1);
      lcd.print("in ");
      lcd.print(i);
      delay(1000);
      lcd.clear();
    }

    switch (mode_index) {
      case 0:
        calibrateGyro();
        driveSquare();
        break;
      case 1:
        driveCircle();
        break;
      case 2:
        calibrateGyro();
        driveForward();
        break;
      case 3:
        driveJiggle();
        break;
    }
  } else {
    // Scroll through the different options
    if (buttonA.getSingleDebouncedPress() && mode_index != 0) {
      mode_index -= 1;
      lcd.clear();
    }

    if (buttonC.getSingleDebouncedPress() && mode_index != 3) {
      mode_index += 1;
      lcd.clear();
    }
  }
}

// Drive in a square
void driveSquare() {
  int angleRotation[] = { -90, -180, 90, -1};

  for (int i = 0; i < 4; i++) {
    motors.setSpeeds(defaultSpeed, defaultSpeed);
    delay(2000);

    while (angle != angleRotation[i]) {

      // Read the sensors
      turnSensorUpdate();
      int32_t angle = getAngle();
      motors.setSpeeds(defaultSpeed, 0);

      // Update the display
      lcd.gotoXY(0, 0);
      lcd.print(angle);
      lcd.print(" ");

      if (angle == angleRotation[i]) {
        break;
      }
    }
  }
  motors.setSpeeds(0, 0);
}

// Drive in a circle
void driveCircle() {
  int motorEncoderA;
  int motorEncoderB;

  const int radius = 18;
  const int carLength = 9;
  const int oneRotation = 910;
  const float oneRotationLength = 12; // cm

  // Calculates the inner and outer circle to stop when the wheels
  // have driven that distance
  float innerCircle = radius * 2 * 3.14;
  float outerCircle = (radius + carLength) * 2 * 3.14;

  int innerRotation = (innerCircle / oneRotationLength) * oneRotation;
  int outerRotation = (outerCircle / oneRotationLength) * oneRotation;

  int outerSpeed = defaultSpeed;
  int innerSpeed = (int)defaultSpeed * (innerCircle / outerCircle) * 0.79;

  // Drive when the distance is not met
  while (motorEncoderA < outerRotation || motorEncoderB < innerRotation) {
    motorEncoderA = encoders.getCountsLeft();
    motorEncoderB = encoders.getCountsRight();

    motors.setSpeeds(outerSpeed, innerSpeed);

    // Stop when the distance is driven
    if (motorEncoderA >= outerRotation || motorEncoderB >= innerRotation) {
      motors.setSpeeds(0, 0);
      motorEncoderA = encoders.getCountsAndResetLeft();
      motorEncoderB = encoders.getCountsAndResetRight();
      break;
    }
  }
}

// Drive zigzag
void driveJiggle() {
  const int objects = 3;
  const int radius = 18;
  const int carLength = 9;
  const int oneRotation = 910;
  const float oneRotationLength = 12; // cm

  // Takes the input of how many objects that are in front of the robot,
  // and drives in half-circles to dodge them, with the same concept as
  // driveCircle function
  for (int i = 0; i < objects; i++) {
    int motorEncoderA;
    int motorEncoderB;

    float innerCircle;
    float outerCircle;
    int outerSpeed;
    int innerSpeed;

    if (i % 2 == 0) {
      innerCircle = radius * 3.14;
      outerCircle = (radius + carLength) * 3.14;

      outerSpeed = defaultSpeed;
      innerSpeed = (int)defaultSpeed * (innerCircle / outerCircle) * 0.79;

    } else if (i % 2 != 0) {
      innerCircle = (radius + carLength) * 3.14;
      outerCircle = radius * 3.14;

      outerSpeed = (int)defaultSpeed * (outerCircle / innerCircle) * 0.86;
      innerSpeed = defaultSpeed;
    }

    int innerRotation = (innerCircle / oneRotationLength) * oneRotation;
    int outerRotation = (outerCircle / oneRotationLength) * oneRotation;

    while (motorEncoderA < outerRotation || motorEncoderB < innerRotation) {
      motorEncoderA = encoders.getCountsLeft();
      motorEncoderB = encoders.getCountsRight();

      Serial.println(motorEncoderA);
      Serial.println(outerRotation);
      Serial.println(motorEncoderB);
      Serial.println(innerRotation);
      Serial.println(" ");

      motors.setSpeeds(outerSpeed, innerSpeed);

      if (motorEncoderA >= outerRotation || motorEncoderB >= innerRotation) {
        motors.setSpeeds(0, 0);
        motorEncoderA = encoders.getCountsAndResetLeft();
        motorEncoderB = encoders.getCountsAndResetRight();
        break;
      }
    }
  }
}

// drives forward, turns 180 degrees, and drives back
void driveForward() {
  const int turnAngle = -180;

  motors.setSpeeds(defaultSpeed, defaultSpeed);
  delay(3000);

  while (angle != -180) {
    turnSensorUpdate();
    int32_t angle = getAngle();
    motors.setSpeeds(defaultSpeed, -defaultSpeed);

    // Update the display
    lcd.gotoXY(0, 0);
    lcd.print(angle);
    lcd.print(" ");

    if (angle == -180) {
      break;
    }
  }

  motors.setSpeeds(defaultSpeed, defaultSpeed);
  delay(3000);
  motors.setSpeeds(0, 0);

}

/////////////////////////////////////////////////////
// Setup and loop
/////////////////////////////////////////////////////

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  selectMode();
}
