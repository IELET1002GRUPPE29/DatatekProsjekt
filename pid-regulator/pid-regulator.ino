// Skrevet av Johan Vu

/////////////////////////////////////////////////////
// Libraries
/////////////////////////////////////////////////////

#include <Zumo32U4.h>
#include "TurnSensor.h"
#define NUM_SENSORS 5

Zumo32U4LineSensors lineSensors;
Zumo32U4ButtonA buttonA;
Zumo32U4Motors motors;
Zumo32U4LCD lcd;
L3G gyro;

/////////////////////////////////////////////////////
// Constants and variables
/////////////////////////////////////////////////////
bool possibleTurnAround = false;            // Detects a dead end 
bool possibleTurnLeft = false;              // Mu
bool turn = false;                          // Must make a turn soon
const int maxSpeed = 180;                   // Max speed for the Zumo
const int turnSpeed = 100;                  // Turning speed 
const int detectTurn = 800;                 // Error value limit when a right angle is met
int lastError = 0;                          // Last error value
int angle = 0;                              // Current degrees
int error;                                  // Current error
int position;                               // Position of the Zumo 
unsigned int lineSensorValues[NUM_SENSORS]; // Array of the 5 sensors in Zumo

/////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////

// Converting sensor values to degrees
int getAngle() {
  // turnAngle is a variable defined in TurnSensor.cpp
  return (((int)turnAngle >> 16) * 360) >> 16;
}

// Calibrate Sensors
void calibrateSensors() {
  lcd.clear();

  // Wait 1 second and then begin automatic sensor calibration
  // by rotating in place to sweep the sensors over the line
  delay(1000);
  for(int i = 0; i < 120; i++)
  {
    if (i > 30 && i <= 90)
    {
      motors.setSpeeds(-200, 200);
    }
    else
    {
      motors.setSpeeds(200, -200);
    }

    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
}

// Calibrates Gyro
void calibrateGyro() {
  turnSensorSetup();
  
  delay(500);
  turnSensorReset();
  lcd.clear();
}

// Adjusting speed with PID-regulator
void PID_drive() {
    // Calculates the speed differnece with PD. Currently using a proportional
    // constant of 1/2 and a derivative constant of 6.
    int speedDifference = error / 2 + 6 * (error - lastError);

    // Saves the current error to use as a recursion
    lastError = error;
    
    // Calculates left and right speed
    int leftSpeed = (int)maxSpeed + speedDifference;
    int rightSpeed = (int)maxSpeed - speedDifference;

    // Constrains the left and right speed
    leftSpeed = constrain(leftSpeed, 0, (int)maxSpeed);
    rightSpeed = constrain(rightSpeed, 0, (int)maxSpeed);

    // Sets the calculated left and right speeds to the motor
    motors.setSpeeds(leftSpeed, rightSpeed);
}

void setup() {
  // put your setup code here, to run once:
   Serial.begin(9600);

   // Initialising five sensors
   lineSensors.initFiveSensors();

    // Wait for button A to be pressed and released, shows it in LCD screen
    lcd.clear();
    lcd.print("Press A");
    lcd.gotoXY(0, 1);
    lcd.print("to calib");
    buttonA.waitForButton();

    // Calibrates sensors
    calibrateSensors();
    calibrateGyro();

    // Wati for button A to be pressed again to run line follower
    lcd.clear();
    lcd.print("Press A");
    lcd.gotoXY(0, 1);
    lcd.print("to go!");
    buttonA.waitForButton();
}

/////////////////////////////////////////////////////
// Setup and loop
/////////////////////////////////////////////////////

void loop() {
  // Gets the current position from the sensorvalues 0 - 4000, where 0 is right and 4000 is left
  position = lineSensors.readLine(lineSensorValues);

  // Error margin in center, which means that negative value is too much right and vise versa
  error = position - 2000;

  // If theres only small error (not a right angle met)
  if (error < detectTurn && error > -detectTurn) {
    PID_drive();

    // If the three sensors in the middle are on the black tape, then alert that a 180-deg turn
    // is soon to come
    if (lineSensorValues[1] == 1000 && lineSensorValues[2] == 1000 && lineSensorValues[3] == 1000){
      possibleTurnAround = true;
    }

    // If the furthest left sensor detects any black tape, alert it
    if (lineSensorValues[0] >= 50) {
      possibleTurnLeft = true;
    }

  // If a right angle is met
  } else if (error >= detectTurn || error <= -detectTurn) {
 
    // A turn is soon to be made
    turn = true;

    // Checks if the robot can turn right at first
    if (lineSensorValues[4] >= 50 && turn == true) { // If the furthest right sensor triggers
      turnSensorReset();

      // Keep turning until 90 degrees is met
      while(angle > -90) {
        motors.setSpeeds(100, -56);   // Turns clockwise
        turnSensorUpdate();           
        int angle = getAngle();   // Gets the current angle

        // If Zumo has turned 90 degrees 
        if (angle <= -90) {
          // Reset booleans and break out of loop
          turn = false;
          possibleTurnLeft = false;
          break;
        }
      }   
  } else {
      // If theres no right turn  to be made, keep driving forward
      motors.setSpeeds(100, 100);
  
      // If the road continues but its a dead end, take a 180-deg turn
      if (possibleTurnAround == true && turn == true) {
        turnSensorReset();

        // Drives forward a bit to have some space
        motors.setSpeeds(100, 100);
        delay(500);

        // Keep turning until a full u-turn
        while(angle > -180) {
          motors.setSpeeds(100, -100);   // Turns clockwise
          turnSensorUpdate();     
          int angle = getAngle();    // Gets current angle   

          // If 180 degrees is met  
          if (angle <= -180) {
            // Reset booleans and break out of loop
            possibleTurnAround = false;
            break;
          }
        }

        // If theres no right turn, forward, or 180-deg turn, take a left turn
      } else if(error == -2000 && turn == true && possibleTurnLeft == true) {
            turnSensorReset();

            // Keep turning until a left turn has been made
            // 80 instead of 90 because of calibration error
            while(angle < 80) {
              motors.setSpeeds(-54, 100);   // Turns anti clockwise
              turnSensorUpdate();
              int angle = getAngle();   // Gets current angle

              // If the Zumo made a left turn
              if (angle >= 80) {
                // Reset boleans and break out of loop
                turn = false;
                possibleTurnLeft = false;
                break;
              }
            }
        }
    }
  }
}
