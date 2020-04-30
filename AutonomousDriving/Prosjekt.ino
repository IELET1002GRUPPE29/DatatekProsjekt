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

const int defaultSpeed = 100;  // Default speed
int32_t angle = 0;             // Current Degrees
int mode_index = 0;            // Index on which mode is selected

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
  char modes[5][8] = {"Square", "Circle", "Forward", "Jiggle"};  // List of options

  // Update the display on which mode is selected
  lcd.gotoXY(0, 0);
  lcd.print("Select:");           // Prints "Select"
  lcd.gotoXY(0, 1);
  lcd.print(modes[mode_index]);   // Prints the mode index into the list to show current mode

  // "Start" button to run program
  if (buttonB.isPressed()) {      // If button B is pressed
    lcd.clear();

    // Counter to place down your robot
    for (int i = 3; i > 0; i--) {
      lcd.gotoXY(0, 0);
      lcd.print("Starting");
      lcd.gotoXY(0, 1);
      lcd.print("in ");
      lcd.print(i);
      delay(1000);
      lcd.clear();
    }

    // Different options, depending on what number mode_index is
    switch (mode_index) {
      case 0:
        // Drive in a Square
        calibrateGyro();
        driveSquare();
        break;
      case 1:
        // Drive in a Circle
        driveCircle();
        break;
      case 2:
        // Drive forward and turn
        calibrateGyro();
        driveForward();
        break;
      case 3:
        // Drive zig zag
        driveJiggle();
        break;
    }
  } else {
    // Scroll through the different options

    // Goes left in the menu when A is pressed (decreasing the mode_index value)
    if (buttonA.getSingleDebouncedPress() && mode_index != 0) {
      mode_index -= 1;
      lcd.clear();
    }

    // Goes right in the menu when C is pressed (acending the mode_index value)
    if (buttonC.getSingleDebouncedPress() && mode_index != 3) {
      mode_index += 1;
      lcd.clear();
    }
  }
}

// Drive in a square
void driveSquare() {
  int angleRotation[] = { -90, -180, 90, -1}; // The diffrent turning points for the circle

  // Loops four times as its going in a square
  for (int i = 0; i < 4; i++) {
    motors.setSpeeds(defaultSpeed, defaultSpeed); // Drives forward in two seconds
    delay(2000);

    // Starts turning, and it will keep turning untill the wanted degrees are met
    while (angle != angleRotation[i]) {

      // Read the sensors
      turnSensorUpdate();
      int32_t angle = getAngle();        // Gets the current angle
      motors.setSpeeds(defaultSpeed, 0); // Set motors to turn right

      // Update the display
      lcd.gotoXY(0, 0);
      lcd.print(angle);
      lcd.print(" ");

      // If angle it met, then stop rotating
      if (angle == angleRotation[i]) {
        break;
      }
    }
  }

  // Stop when it is done driving in a square
  motors.setSpeeds(0, 0);
}

// Drive in a circle
void driveCircle() {

  // Declare motorEncoders
  int motorEncoderLeft;
  int motorEncoderRight;

  // Local variables in the function
  const int radius = 18;              // Radius of the wanted circle
  const int carLength = 9;            // Width of the Zumo
  const int oneRotation = 910;        // 910 encoder value to turn one time
  const float oneRotationLength = 12; // cm

  // Calculates the inner and outer circle to stop when the wheels
  // have driven that distance

  float innerCircle = radius * 2 * 3.14;               // Inner circumference
  float outerCircle = (radius + carLength) * 2 * 3.14; // Outer circumference

  // Inner and outer circumference into encoder values
  int innerRotation = (innerCircle / oneRotationLength) * oneRotation;
  int outerRotation = (outerCircle / oneRotationLength) * oneRotation;

  // Declearing Speed
  int outerSpeed = defaultSpeed;
  int innerSpeed = defaultSpeed * radius / (radius + carLength) * 0.75;

  // Drive when the distance is not met
  while (motorEncoderLeft < innerRotation || motorEncoderRight < outerRotation) {

    // Gets the current Encoder values
    motorEncoderLeft = encoders.getCountsLeft();
    motorEncoderRight = encoders.getCountsRight();

    //Sets the calculated speeds
    motors.setSpeeds(innerSpeed, outerSpeed);

    // Stop when the distance is driven
    if (motorEncoderLeft >= innerRotation || motorEncoderRight >= outerRotation) {
      motors.setSpeeds(0, 0);
      motorEncoderLeft = encoders.getCountsAndResetLeft();
      motorEncoderRight = encoders.getCountsAndResetRight();
      break;
    }
  }
}

// Drive zigzag
void driveJiggle() {

  // Local variables
  const int objects = 3;              // How many objects to come
  const int radius = 18;              // Distance of the object to the Zumo
  const int carLength = 9;            // Width of the Zumo
  const int oneRotation = 910;        // 910 encoder value to turn one time
  const float oneRotationLength = 12; // cm

  // Takes the input of how many objects that are in front of the robot,
  // and drives in half-circles to dodge them, with the same concept as
  // driveCircle function
  for (int i = 0; i < objects; i++) {

    // Declare important variables
    int motorEncoderL;    // Encoder for left motor
    int motorEncoderR;    // Encoder for right motor
    float circleMotorL;   // Left circumference
    float circleMotorR;   // Right circumference
    int motorSpeedL;      // Speed for the left motor
    int motorSpeedR;      // Speed for the right motor

    // Checks if the current iterated number is an even number
    // to differenciate left and right half circles
    if (i % 2 == 0) {
      // Inner and outer circle circumference for left and right motor
      circleMotorR = radius * 3.14;
      circleMotorL = (radius + carLength) * 3.14;

      // Declaring speed for left and right motor
      motorSpeedL = defaultSpeed;
      motorSpeedR = defaultSpeed * radius / (radius + carLength) * 0.84;

      // Check if the iterated number is an odd
    } else if (i % 2 != 0) {
      // Inner and outer circle circumference for left and right motor
      circleMotorR = (radius + carLength) * 3.14;
      circleMotorL = radius * 3.14;

      // Delcaring speed for left and right motro
      motorSpeedL = defaultSpeed * radius / (radius + carLength) * 0.70;
      motorSpeedR = defaultSpeed;
    }

    // Calculates the encoder value of the wanted distance traveled
    motorEncoderR = (circleMotorR / oneRotationLength) * oneRotation;
    motorEncoderL = (circleMotorL / oneRotationLength) * oneRotation;

    // If the distance is not met, keep driving
    while (motorEncoderL < circleMotorL || motorEncoderR < circleMotorR) {
      // Gets the current encoder values
      motorEncoderL = encoders.getCountsLeft();
      motorEncoderR = encoders.getCountsRight();

      // Sets the current speed of left and right motor
      motors.setSpeeds(motorSpeedL, motorSpeedR);

      // If the distance is met, stop driving
      if (motorEncoderL >= circleMotorL || motorEncoderR >= circleMotorR) {
        motors.setSpeeds(0, 0);
        motorEncoderL = encoders.getCountsAndResetLeft();
        motorEncoderR = encoders.getCountsAndResetRight();
        break;
      }
    }
  }
}

// drives forward, turns 180 degrees, and drives back
void driveForward() {
  const int turnAngle = -180;

  // Drive forward in three seconds
  motors.setSpeeds(defaultSpeed, defaultSpeed);
  delay(3000);

  // If current angles arent 180 degrees, keep turning
  while (angle != -180) {
    turnSensorUpdate();
    int32_t angle = getAngle();                     // Gets the current angle
    motors.setSpeeds(defaultSpeed, -defaultSpeed);  // Set the motors to spin clockwise

    // Update the display
    lcd.gotoXY(0, 0);
    lcd.print(angle);
    lcd.print(" ");

    // if the robot has turned 180 degrees, stop
    if (angle == -180) {
      break;
    }
  }

  // Drive forward in three seconds, then stop
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
