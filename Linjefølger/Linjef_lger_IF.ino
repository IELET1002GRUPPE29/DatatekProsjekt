//Inlcuding the nessesary librarys and defining classes
#include <Zumo32U4.h>                                                     //Main zumo library
Zumo32U4LineSensors lineSensor;                                           //Zumo IR sensor libary (Linefollower)
Zumo32U4LCD lcd;                                                          //Zumo LCD screen library
Zumo32U4Motors motors;                                                    //Zumo motor library
Zumo32U4ButtonA buttonA;


//Arrays
unsigned int lineSensorValues[5];                                         //Array for storting lineSensor data, [x], x for number of sensors

//Defining global constants
const int motor_speeds = 100;                                             //Sets motor_speed globally
int maxSpeed = 200;                                                       //Limits the max speed of the motors
int last_off_center_position = 0;
int pos;

//Millis() varibles
unsigned long time_now;
unsigned long timerStart;
const int terskel = 2000;

void lcd_startup() {
  lcd.clear();                                                            //Clears lcd screen
  lcd.gotoXY(0, 0);                                                       //Sets cursor to 0,0
  lcd.print("A to");                                                      //Writes text
  lcd.gotoXY(0, 1);                                                       //Sets crisor to 1,0
  lcd.print("calibr8");                                                   //Prints text
}

void calibrate_IR() {                                                     //Function that makes car spin and calibrates the IR sensors
  delay(1000);                                                            //Delay, so that the car doesn't begin calibrating right away
  motors.setSpeeds(motor_speeds, -motor_speeds);                          //Makes car spin
  time_now = millis();                                                    //Takes timestamp
  for (int i = 0; i < 180; i++) {                                         //Makes car calibrate in a  cool way
    if (i > 30 && i < 120) {
      motors.setSpeeds(motor_speeds, -motor_speeds);
    }
    else {
      motors.setSpeeds(-motor_speeds, motor_speeds);
    }
    lineSensor.calibrate();
  }
  motors.setSpeeds(0, 0);
}

void print_position(int x) {                                              //Function that prints position on lcd
  lcd.clear();                                                            //Clears lcd
  lcd.gotoXY(0, 0);                                                       //Places cursor in top left corner
  lcd.print(lineSensorValues[0]);                                                           //Prints the position data
  lcd.gotoXY(4, 0);
  lcd.print(lineSensorValues[1]);
  lcd.gotoXY(0, 1);
  lcd.print(lineSensorValues[2]);
  lcd.gotoXY(4, 1);
  lcd.print(lineSensorValues[3]);
  delay(50);
  
}

void check_for_line(){
  
    motors.setSpeeds(motor_speeds,motor_speeds);
    timerStart = millis();
    while(millis() < timerStart + 2000){                //Kjør frem i 2 sek og sjekk for linje
      pos = lineSensor.readLine(lineSensorValues);
      print_position(pos);
      if (lineSensorValues[0] > 50 || lineSensorValues[1]>50 || lineSensorValues[2] > 50 || lineSensorValues[3] > 50 || lineSensorValues[4] > 50){
        return;                         //hvis linje er funnet, retunrer til OG programm
      } 
    }
    motors.setSpeeds(-motor_speeds,-motor_speeds); //Hvis linje ikke er funnet, rygg tilbake til utgangspunktet.
    delay(1200);
    motors.setSpeeds(motor_speeds,-motor_speeds);
    delay(1200);
    motors.setSpeeds(0, 0);
    delay(1000);
    motors.setSpeeds(motor_speeds,motor_speeds);
    timerStart = millis();
    while(millis() < timerStart + 2000){                //Kjør frem i 2 sek og sjekk for linje
      pos = lineSensor.readLine(lineSensorValues);
      print_position(pos);
      if (lineSensorValues[0] > 50 || lineSensorValues[1]>50 || lineSensorValues[2] > 50 || lineSensorValues[3] > 50 || lineSensorValues[4] > 50){
        return;                         //hvis linje er funnet, retunrer til OG programm
      } 
    }
    motors.setSpeeds(-motor_speeds,-motor_speeds); //Hvis linje ikke er funnet, rygg tilbake til utgangspunktet.
    delay(2000);
    motors.setSpeeds(motor_speeds,-motor_speeds);
    delay(2400);
    motors.setSpeeds(0, 0);
    delay(1000);
    motors.setSpeeds(motor_speeds,motor_speeds);
    timerStart = millis();
    while(millis() < timerStart + 2000){                //Kjør frem i 2 sek og sjekk for linje
      pos = lineSensor.readLine(lineSensorValues);
      print_position(pos);
      if (lineSensorValues[0] > 50 || lineSensorValues[1]>50 || lineSensorValues[2] > 50 || lineSensorValues[3] > 50 || lineSensorValues[4] > 50){
        return;                         //hvis linje er funnet, retunrer til OG programm
      } 
    }
     motors.setSpeeds(-motor_speeds,-motor_speeds); //Hvis linje ikke er funnet, rygg tilbake til utgangspunktet.
    delay(2000);
    motors.setSpeeds(-motor_speeds,motor_speeds);
    delay(1200);
    motors.setSpeeds(0, 0);
    delay(1000);
    motors.setSpeeds(motor_speeds,motor_speeds);
    timerStart = millis();
    while(millis() < timerStart + 2000){                //Kjør frem i 2 sek og sjekk for linje
      pos = lineSensor.readLine(lineSensorValues);
      print_position(pos);
      if (lineSensorValues[0] > 50 || lineSensorValues[1]>50 || lineSensorValues[2] > 50 || lineSensorValues[3] > 50 || lineSensorValues[4] > 50){
        return;                         //hvis linje er funnet, retunrer til OG programm
      } 
    }
    
}


void setup() {
  lineSensor.initFiveSensors();                                           //Initiates the IR-sensors
  lcd_startup();                                                          //Runs LCD startup screen
  buttonA.waitForButton();                                                //Waits for button A to be pressed to intialize calibation
  calibrate_IR();                                                         //Runs the calibrate function
  lcd.clear();

}

void loop() {
  pos = lineSensor.readLine(lineSensorValues);                   //Gets position data from the array?
  print_position(pos);
  if (lineSensorValues[0] > 0 || lineSensorValues[1]>0 || lineSensorValues[2] > 0 || lineSensorValues[3] > 0 || lineSensorValues[4] > 0) {
    if (pos <= 1500) {
      motors.setSpeeds(10, motor_speeds);
    }
    else if (pos >= 2500) {
      motors.setSpeeds(motor_speeds, 10);
    }
    else {
      motors.setSpeeds(motor_speeds, motor_speeds);
    }
  }
  if (lineSensorValues[1] == 0 && lineSensorValues[2] == 0 && lineSensorValues[3] == 0) {
    motors.setSpeeds(0, 0);
    delay(1000);
    check_for_line();
  }
}
