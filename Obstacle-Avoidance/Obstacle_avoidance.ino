//Bibliotek//
#include <Wire.h> 
#include <Zumo32U4.h>
#include "TurnSensor.h"

// Zumo komponenter //

Zumo32U4LCD lcd;
Zumo32U4ProximitySensors proxSensors;  //Brukes for å avgjøre avstanden til objekter///
Zumo32U4ButtonA buttonA; //start knapp//
Zumo32U4Encoders encoders;
Zumo32U4ButtonC buttonC; //Kalibreringsknapp///
Zumo32U4Motors motors;
LSM303 lsm303; // Accelerometer brukes for at dersom zumoen kolliderer skal den endre retning //
L3G gyro; // Avgjør hvor mye Zumoen skal rotere // 

// variabler til retningsfunkjsoner//
enum Status {
  paused,
  forward_left,
  forward_right,
  left_scanning,
  right_scanning,
  reverseing,
  advancing
};

//Konstante variabler//

const int Max_speed = 250;
const int rotation_speed = 200;
const int acceleration = 2;
float distance = 0;
String batteryMessure = "";

Status status_value = paused;
int current_speed = 0;

// Funksjoner //

// Roter til venstre //
void Left_turn_fun(int degrees) {
  turnSensorReset();
  motors.setSpeeds(-rotation_speed, rotation_speed);
  int rotation = 0;
  do {
    delay(1);
    turnSensorUpdate();
    rotation = (((int32_t)turnAngle >> 16) * 360) >> 16;
  } while (rotation < degrees);
  motors.setSpeeds(0, 0);
}

// Roter til høyere //
void Right_turn_fun(int degrees) {
  turnSensorReset();
  motors.setSpeeds(rotation_speed, -rotation_speed);
  int rotation = 0;
  do {
    delay(1);
    turnSensorUpdate();
    rotation = (((int32_t)turnAngle >> 16) * 360) >> 16;
  } while (rotation > -degrees);
  motors.setSpeeds(0, 0);
}


// Gå fremover //
void forward_fun() {
  
  turnSensorUpdate();
  int rotation = (((int32_t)turnAngle >> 16) * 360) >> 16;

  // kjør fremover og samtidig juster moterfarten for å holde retningen //
  motors.setSpeeds(current_speed + (rotation * 5), current_speed - (rotation * 5));
}

// Scann til venstre //
void scanLeft() {
  motors.setSpeeds(-current_speed, current_speed);
}

// Scann til høyre //
void scanRight() {
  motors.setSpeeds(current_speed, -current_speed);
}

// Len mot venstre //
void forward_left_fun() {
  motors.setSpeeds(current_speed / 2, current_speed);
}

// Len mot høyre //
void forward_right_fun() {
  motors.setSpeeds(current_speed, current_speed / 2);
}

//Display som skal i void-setup//
void lcd_startup(){
  lcd.clear();                                                                                            
  lcd.gotoXY(0,0);                                                                                        
  lcd.print("Press C ");                                                                                      
  lcd.gotoXY(0,1);                                                                                        
  lcd.print("to calib");
  delay(2000);   
  buttonC.waitForButton();                                                                                
}

// batterioversikt //
//fulllandet Zumo viser 6V //
void battery(){

  float batteryLevel = readBatteryMillivolts() / 1000;
  
  if (batteryLevel >=5) {
    batteryMessure = "Good";
    }
  if (batteryLevel >=3 && batteryLevel <= 4 ) {
    batteryMessure = "Ok  ";
    }
  if (batteryLevel < 3) {
    batteryMessure = "Low ";
    }
    
    lcd.print("Battery");
    lcd.gotoXY(0, 1);
    lcd.print("%-> ");
    lcd.print(batteryMessure);
    delay(3000); 
  }

//Kalibrerings funksjon//
void calibrateSensors()
{
  battery();
  lcd_startup();
  lcd.gotoXY(0,0);                                                                                        
  lcd.print("calib- ");
  lcd.gotoXY(0,1);   
  lcd.print("rating  ");
  
  // Rotering mens kalibrering //
  delay(3000);
  for(int i = 0; i < 120; i++){
    if (i > 30 && i <= 90){
      delay(10);
      lcd.gotoXY(0,0);                                                                                        
      lcd.print("calib-  ");
      lcd.gotoXY(0,1);   
      lcd.print("rating  ");
      motors.setSpeeds(-200, 200);
    } else {
      delay(10);
      lcd.gotoXY(0,0);                                                                                        
      lcd.print("calib-");
      lcd.gotoXY(0,1);   
      lcd.print("rating  ");
      motors.setSpeeds(200, -200);
    }
  }
  motors.setSpeeds(0, 0);
}


// setup //
void setup() {

  battery();

  //Kalibrering
  calibrateSensors();
  Serial.begin(9600);
     
  // Proximity sensors
  proxSensors.initThreeSensors();

  // Accelerometer
  Wire.begin();
  lsm303.init();
  lsm303.enableDefault();

  // Gyrometer kalibrering //
  turnSensorSetup();
  delay(500);
  turnSensorReset();

  lcd.clear();
  lcd.gotoXY(0,0);                                                                                        
  lcd.print("Ready ");
  lcd.gotoXY(0,1);   
  lcd.print("to go!");
  delay(1000);
  lcd.clear();
  lcd.gotoXY(0,0);                                                                                        
  lcd.print("Press A ");
  lcd.gotoXY(0,1);   
  lcd.print("To Run!"); 
  delay(2000);
}

// --- Main Loop ---
void loop() {

  // setter hovedfunksjonen i gang //
  bool buttonPress = buttonA.getSingleDebouncedPress();

  // Proximity sensors
  proxSensors.read();

  int left_sensor_prox = proxSensors.countsLeftWithLeftLeds();
  int center_left_sensor_prox = proxSensors.countsFrontWithLeftLeds();
  int center_right_sensor_prox = proxSensors.countsFrontWithRightLeds();
  int right_sensor_prox = proxSensors.countsRightWithRightLeds();

  // Accelerometer, sjekker om det blir brå stopp/kollisjon//
  lsm303.read();
  int16_t x = lsm303.a.x;
  int16_t y = lsm303.a.y;
  int32_t collisjon = (int32_t)x * x + (int32_t)y * y;

  // Change status_values, hjernen til Zumoen //
  if (status_value == paused) {
    if (buttonPress) {
      status_value = advancing;
      turnSensorReset();
    }
  }
  else if (buttonPress) {
    status_value = paused;
  }
  else if (collisjon > 250000000) {
    status_value = reverseing;
  }
  else if (status_value == left_scanning) {
    if (center_left_sensor_prox < 4 && center_right_sensor_prox < 4) {
      status_value = advancing;
      turnSensorReset();
    }
  }
  else if (status_value == right_scanning) {
    if (center_left_sensor_prox < 4 && center_right_sensor_prox < 4) {
      status_value = advancing;
      turnSensorReset();
    }
  }
  else if (status_value == advancing) {
    if (center_left_sensor_prox >= 5 && center_right_sensor_prox >= 5) {
      if (center_left_sensor_prox < center_right_sensor_prox) {
        status_value = left_scanning;
      } else {
        status_value = right_scanning;
      }
    }
    else if (center_left_sensor_prox >= 2 && center_right_sensor_prox >= 2) {
      if (center_left_sensor_prox < center_right_sensor_prox) {
        status_value = forward_left;
      } else {
        status_value = forward_right;
      }
    }
  }
  else if (status_value == forward_left || status_value == forward_right) {
    if (center_left_sensor_prox < 2 && center_right_sensor_prox < 2) {
      status_value = advancing;
      turnSensorReset();
    }
    if (center_left_sensor_prox >= 5 && center_right_sensor_prox >= 5) {
      if (center_left_sensor_prox < center_right_sensor_prox) {
        status_value = left_scanning;
      } else {
        status_value = right_scanning;
      }
    }
  }

  // Justering av motorfart //

  if (status_value != paused && current_speed < Max_speed) {
    current_speed += acceleration;
  }

  if (status_value == paused) {
     motors.setSpeeds(0, 0);
    current_speed = 0;
  }
  else if (status_value == advancing)
    forward_fun();
  else if (status_value == forward_left)
    forward_left_fun();
  else if (status_value == forward_right)
    forward_right_fun();
  else if (status_value == left_scanning)
    scanLeft();
  else if (status_value == right_scanning)
    scanRight();
  else if (status_value == reverseing) {
    lcd.gotoXY(0, 0);
    lcd.print("Reverse!");
    motors.setSpeeds(-Max_speed, -Max_speed);
    delay(250);
    Left_turn_fun(150);
    current_speed = 0;
    delay(200);
    status_value = advancing;
    lcd.clear();
    turnSensorReset();
  }

  //Display mens kjøring//
  lcd.gotoXY(0, 0);
  if (status_value == paused)
    lcd.print("Traveled");
    else if (status_value == advancing)
    lcd.print("Freeway!");
  else if (status_value == forward_left)
    lcd.print("<-------");
  else if (status_value == forward_right)
    lcd.print("------->");
  else if (status_value == left_scanning)
    lcd.print("<--L?-->");
  else if (status_value == right_scanning)
    lcd.print("<--R?-->");

// - Distanse måler med encoders - //
    int countsLeft;
    int countsRight;
  
  countsLeft = encoders.getCountsLeft();
  countsRight = encoders.getCountsRight();
  
// Målinger fra praksis bruk av Zumoen viser at en hel rotasjon utgjør en puls på 910, og en avstand på en rotasjon er 12 cm // 
  if ((countsLeft >= 890 && countsLeft <= 980) || (countsRight >= 890 && countsRight <= 980)){
    //legger inn en økning med 12 cm//
    distance += 0.12;
    //Reseter/tømmer variablene//
    countsLeft = encoders.getCountsAndResetLeft();
    countsRight = encoders.getCountsAndResetRight(); 
    }
     Serial.println(distance);
     Serial.println("\t");
     Serial.println(countsLeft);

  // Viser avstand kjørt på display//
  lcd.gotoXY(0, 1);
  lcd.print("D:");
  lcd.print(distance);
  lcd.print("M ");
  delay(2);
}
