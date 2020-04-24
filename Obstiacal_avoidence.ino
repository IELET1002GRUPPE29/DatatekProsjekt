//Bibliotek//
#include <Wire.h> // Used for Accelerometer
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
enum State {
  pause_state,
  forward_left_state,
  forward_right_state,
  scan_left_state,
  scan_right_state,
  reverse_state,
  forward_state
};

//Konstante variabler//

const uint16_t motorSpeed = 350;
const uint16_t turnSpeed = 200;
const int acceleration = 2;
float distance = 0;
String batteryMessure = "";

State state = pause_state;
int curSpeed = 0;

// Funksjoner //

// Roter til venstre //
void turnLeft(int degrees) {
  turnSensorReset();
  motors.setSpeeds(-turnSpeed, turnSpeed);
  int angle = 0;
  do {
    delay(1);
    turnSensorUpdate();
    angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    lcd.gotoXY(0, 0);
    lcd.print(angle);
    lcd.print(" ");
  } while (angle < degrees);
  motors.setSpeeds(0, 0);
}

// Roter til høyere //
void turnRight(int degrees) {
  turnSensorReset();
  motors.setSpeeds(turnSpeed, -turnSpeed);
  int angle = 0;
  do {
    delay(1);
    turnSensorUpdate();
    angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    lcd.gotoXY(0, 0);
    lcd.print(angle);
    lcd.print(" ");
  } while (angle > -degrees);
  motors.setSpeeds(0, 0);
}

// Stop //
void stop() {
  motors.setSpeeds(0, 0);
}

// Gå fremover //
void forward() {
  
  turnSensorUpdate();
  int angle = (((int32_t)turnAngle >> 16) * 360) >> 16;

  // kjør fremover og samtidig juster moterfarten for å holde retningen //
  motors.setSpeeds(curSpeed + (angle * 5), curSpeed - (angle * 5));
}

// reversering //
void reverse() {
  motors.setSpeeds(-motorSpeed, -motorSpeed);
}

// Scann til venstre //
void scanLeft() {
  motors.setSpeeds(-curSpeed, curSpeed);
}

// Scann til høyre //
void scanRight() {
  motors.setSpeeds(curSpeed, -curSpeed);
}

// Len mot venstre //
void forwardLeft() {
  motors.setSpeeds(curSpeed / 2, curSpeed);
}

// Len mot høyre //
void forwardRight() {
  motors.setSpeeds(curSpeed, curSpeed / 2);
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

  uint16_t batteryLevel = readBatteryMillivolts() / 1000;
  
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

  int left_sensor = proxSensors.countsLeftWithLeftLeds();
  int centerLeftSensor = proxSensors.countsFrontWithLeftLeds();
  int centerRightSensor = proxSensors.countsFrontWithRightLeds();
  int right_sensor = proxSensors.countsRightWithRightLeds();

  // Accelerometer, sjekker om det blir brå stopp/kollisjon//
  lsm303.read();
  int16_t x = lsm303.a.x;
  int16_t y = lsm303.a.y;
  int32_t magnitudeSquared = (int32_t)x * x + (int32_t)y * y;

  // Change states, hjernen til Zumoen //
  if (state == pause_state) {
    if (buttonPress) {
      state = forward_state;
      turnSensorReset();
    }
  }
  else if (buttonPress) {
    state = pause_state;
  }
  else if (magnitudeSquared > 250000000) {
    state = reverse_state;
  }
  else if (state == scan_left_state) {
    if (centerLeftSensor < 4 && centerRightSensor < 4) {
      state = forward_state;
      turnSensorReset();
    }
  }
  else if (state == scan_right_state) {
    if (centerLeftSensor < 4 && centerRightSensor < 4) {
      state = forward_state;
      turnSensorReset();
    }
  }
  else if (state == forward_state) {
    if (centerLeftSensor >= 5 && centerRightSensor >= 5) {
      if (centerLeftSensor < centerRightSensor) {
        state = scan_left_state;
      } else {
        state = scan_right_state;
      }
    }
    else if (centerLeftSensor >= 2 && centerRightSensor >= 2) {
      if (centerLeftSensor < centerRightSensor) {
        state = forward_left_state;
      } else {
        state = forward_right_state;
      }
    }
  }
  else if (state == forward_left_state || state == forward_right_state) {
    if (centerLeftSensor < 2 && centerRightSensor < 2) {
      state = forward_state;
      turnSensorReset();
    }
    if (centerLeftSensor >= 5 && centerRightSensor >= 5) {
      if (centerLeftSensor < centerRightSensor) {
        state = scan_left_state;
      } else {
        state = scan_right_state;
      }
    }
  }

  // Justering av motorfart //

  if (state != pause_state && curSpeed < motorSpeed) {
    curSpeed += acceleration;
  }

  if (state == pause_state) {
    stop();
    curSpeed = 0;
  }
  else if (state == forward_state)
    forward();
  else if (state == forward_left_state)
    forwardLeft();
  else if (state == forward_right_state)
    forwardRight();
  else if (state == scan_left_state)
    scanLeft();
  else if (state == scan_right_state)
    scanRight();
  else if (state == reverse_state) {
    lcd.gotoXY(0, 0);
    lcd.print("Reverse!");
    reverse();
    delay(250);
    turnLeft(150);
    curSpeed = 0;
    delay(200);
    state = forward_state;
    lcd.clear();
    turnSensorReset();
  }

  //Display mens kjøring//
  lcd.gotoXY(0, 0);
  if (state == pause_state)
    lcd.print("Traveled");
    else if (state == forward_state)
    lcd.print("Freeway!");
  else if (state == forward_left_state)
    lcd.print("<-------");
  else if (state == forward_right_state)
    lcd.print("------->");
  else if (state == scan_left_state)
    lcd.print("<--L?-->");
  else if (state == scan_right_state)
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
