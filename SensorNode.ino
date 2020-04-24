#include <Config.h>
#include <EasyBuzzer.h>
#include <ESP32Servo.h>

//BLYNKSHIT
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <analogWrite.h>

WidgetTerminal terminal(V1);
WidgetLED alarmLed (V20);
Servo myservo;

//WiFi and Token
char auth[] = "ytmzJE1PS9tqaITeeakoHxAfq39z0482";
char ssid[] = "ELIAS-XPS 8798";
char pass[] = "esp-32-web";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Global variables
bool calculate_average_variable = false;                        //if true, begin calcualting averages
int measureSensor = 0;                                          //1 = deistance, 2 temp, 3 light shit
int PointsPerAverage = 10;

//gpio pins for the phisycal components of the alarm
const byte led_gpio = 23; 
const byte buzz_gpio = 21;

//HC-SR04 distance measuring sensor
const int trigPin = 4;
const int echoPin = 5;
long duration;
int distance;

//Average values and arrays
const int basicArrayLength = 50;                                          //the length of each data array
float tempArray[basicArrayLength];                                        //array for the temp readings
float distanceArray[basicArrayLength];                                    //array for the distance readings
float lightArray[basicArrayLength];                                       //array for the thermistor readings
int readIndex = 0;                                                        //the index (runnig count) we write data to in the arrays

//MAX_MIN VALUES
float maxTemp = 0;                                                        //Setter maks-verdiene til 0 slik at vi vil få høyere verdier inn og overskriver
float maxLight = 0;
float maxDist = 0;

//millis timer constatns 
unsigned long time_now;
unsigned long prev_time;
const int time_threshold = 10000;                                         //timer constant for each time to send max values to blynk

//max values for each sensor to trigger the count of thresholdbreaks
const float temp_threshold = 25;
const float dist_threshold = 25;
const float light_threshold = 1500;
int threshold_breaker_counter = 0;

//global function variavbes
int total;
bool servo_left = false;
unsigned long alarmtime;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS


//Function that gets value from photoresisor
float photo() {
  const int photoPin = 32;
  float sensor_value = analogRead(photoPin);
  return sensor_value;
}
//Function that gets temp form thermistor
float thermistor() {
  const int room_temp = 22;
  int R1 = 10000;           // the resistor from voltage divider
  float Bt = 3950.0;
  const int thermoPin = 35;
  float sensor_value = analogRead(thermoPin);                   //reading the value from GPIO35
  float Vout = sensor_value * (3.3 / 4095.0);                   //converts from analog to voltage
  float Rt = R1 * Vout / (3.3 - Vout);                          //the resistance of thermistor from the voltage divider
  float a1 = 1 / (270.15 + room_temp);
  float b1 = 1 / Bt;
  float c1 = log(Rt / 10000.0);                                 //third parameter, 10000 ohms are Ro at zero degrees celsius
  float y1 = a1 + b1 * c1;                                      //the result of equation
  float T = 1 / y1;                                             //temperature in kelvin
  float Tc = T - 273.15;                                        //temperature in celsius
  return Tc;
}
//Function that gets distance from HC-SR04
float distance_func() {                                                 //Found at: https://www.instructables.com/id/Pocket-Size-Ultrasonic-Measuring-Tool-With-ESP32/

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor

  return distance;
}

//Function that calculates the average of values in arrays based on the position of sliders in the Blynk app
float averageArray(float * array, int sliderValue) {                        //got from Michael Berg
  total = 0;
  for (int i = 0; i < sliderValue; i++)
  {
    total += float(array[i]);
  }

  return (float(total) / float(sliderValue));
}

//Function that checks max values
void max_min(float temp, float light, float dist) {                         //shoutout to Michael
  if (temp > maxTemp) {
    maxTemp = temp;
  } 
  if (light > maxLight) {
    maxLight = light;
  }
  if (dist > maxDist) {
    maxDist = dist;
  }
}

//Function that sets of the physical alarm component of the alarm system
void alarm(){
  //local millis timer variables
  unsigned long inside_time_now;
  int terskel = 500;

  ledcWrite(0, 100);                                            //Buzzer on
  ledcWrite(1, 240);                                            //Led on
  alarmLed.on();                                                //Led blynk app on
  inside_time_now = millis();
  while (millis() < inside_time_now + terskel) {}               //wait terksel time
  ledcWrite(0, 0);                                              //Buzzer off
  ledcWrite(1, 0);                                              //led off
  alarmLed.off();                                               //Led blynk app off
  inside_time_now = millis();
  while (millis() < inside_time_now + terskel) {}
}

void servo_alarm(){
  
  Serial.println("alarm");
  alarmtime = millis();
  while (millis() < alarmtime + 500) {
    for (int pos = 0; pos <= 180; pos += 1) {
      myservo.write(pos);
      time_now = millis();
      while (millis() < time_now + 10) {}
    }

    for (int pos = 180; pos >= 0; pos -= 10) {
      myservo.write(pos);
      time_now = millis();
      while (millis() < time_now + 10) {}
    }
  }
  if (servo_left == true) {
    myservo.write(180);
  }
  if (servo_left == false) {
    myservo.write(0);
  }
  servo_left = !servo_left;
}


BlynkTimer timer;


void setup() {
  Serial.begin(9600);
  myservo.attach(33);                                                   //attaches servo to gpio pin 33
  pinMode(trigPin, OUTPUT);                                             // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);                                              // Sets the echoPin as an Input
  ledcAttachPin(buzz_gpio, 0);                                          //Attaches buzzer gpio pin to pwm channel 0
  ledcAttachPin(led_gpio, 1);                                           //Attaches led gpio pin to pwm channel 
  ledcSetup(0, 4000, 8);                                                //Configures the pwm channels: channel, Freq, bitlength
  ledcSetup(1, 4000, 8);
  Blynk.begin(auth, ssid, pass, IPAddress(91, 192, 221, 40), 8080);     //blynk to ntnu blynk
  timer.setInterval(100L, myTimerEvent);                                  
}

//Gets slider values from the Blynk app
int ppaSlider;                                                          //Slider value
BLYNK_WRITE(V3) {
  PointsPerAverage = param.asInt();                                     //Gets slider value from blynk widget
  calculate_average_variable = false;                                   //if the slider is adjustet to change avgpoints befor arrays ar full, "ignore"
}


int test_button;                                                        //Servo test button
BLYNK_WRITE(V22){
  test_button = param.asInt();
  while(test_button == 1){
  unsigned long servo_test_time;
  int terskel = 10;
  for (int pos = 0; pos <= 180; pos += 1) {
    myservo.write(pos);
    servo_test_time = millis();
    while (millis() < servo_test_time + terskel) {}
    }
  for (int pos = 180; pos >= 0; pos -= 1) {
    myservo.write(pos);
    servo_test_time = millis();
    while (millis() < servo_test_time + terskel) {}
    }
    test_button = param.asInt();
  }
}

void test_servo() {
  
}





BLYNK_WRITE(V0) { //Skriv inn terminalshitt her og sett en variabel til true som gjør at data sendens til graf i enten loop eller timer
  switch (param.asInt()) {
    case 1: {
        terminal.clear();
        measureSensor = 1;

        Blynk.setProperty(V2, "label", "Distance");
        Blynk.setProperty(V2, "suffix", "Distance");
        Blynk.setProperty(V2, "min", 0);
        Blynk.setProperty(V2, "max", 200);
        Blynk.setProperty(V2, "/pin/", "cm");
        break;
      }
    case 2: {
        terminal.clear();
        measureSensor = 2;
        Blynk.setProperty(V2, "label", "Temperature");
        Blynk.setProperty(V2, "suffix", "Temp");
        Blynk.setProperty(V2, "min", 0);
        Blynk.setProperty(V2, "max", 30);
        Blynk.setProperty(V2, "/pin/", "C");
        break;
      }
    case 3: {
        terminal.clear();
        measureSensor = 3;
        Blynk.setProperty(V2, "label", "Light");
        Blynk.setProperty(V2, "suffix", "Volt");
        Blynk.setProperty(V2, "min", 0);
        Blynk.setProperty(V2, "max", 1200);
        Blynk.setProperty(V2, "/pin/", "V");
        break;
      }
  }
}


void myTimerEvent() {

  if (measureSensor == 1) {
    Blynk.virtualWrite(V2, distance_func());
    String printstring = "The distance is: " + String(distance_func()) + "cm\n";
    terminal.print(printstring);
    if (calculate_average_variable == true) {
      Blynk.virtualWrite(V4, averageArray(distanceArray, PointsPerAverage));
      Blynk.virtualWrite(V7, averageArray(distanceArray, PointsPerAverage));
    }
  }
  if (measureSensor == 2) {
    Blynk.virtualWrite(V2, thermistor());
    String printstring = "The temp is: " + String(thermistor()) + "C\n";
    terminal.print(printstring);
    if (calculate_average_variable == true) {
      Blynk.virtualWrite(V5, averageArray(tempArray, PointsPerAverage));
      Blynk.virtualWrite(V7, averageArray(tempArray, PointsPerAverage));
    }
  }
  if (measureSensor == 3) {
    Blynk.virtualWrite(V2, photo());
    String printstring = "The light is: " + String(photo()) + "V\n";
    terminal.print(printstring);
    if (calculate_average_variable == true) {
      Blynk.virtualWrite(V6, averageArray(lightArray, PointsPerAverage));
      Blynk.virtualWrite(V7, averageArray(lightArray, PointsPerAverage));
    }
  }

  tempArray[readIndex] = thermistor();
  distanceArray[readIndex] = distance_func();
  lightArray[readIndex] = photo();
  max_min(thermistor(), photo(), distance_func());

  readIndex = readIndex + 1;                          // går til neste posisjon i arrayet
  if (readIndex >= basicArrayLength) {
    readIndex = 0;
    calculate_average_variable = true;                //regner ut snitt først når hele arrayet er fullt
  }

  /*
    //PRINT ALL DATA TIL EGEN GRAF UANSETT
    Blynk.virtualWrite(V2,distance_func());
    Blynk.virtualWrite(V6,thermistor());
    Blynk.virtualWrite(V10,photo());

    Blynk.virtualWrite(V12,maxDist);
    Blynk.virtualWrite(V13,minDist);
    Blynk.virtualWrite(V14,maxTemp);
    Blynk.virtualWrite(V15,minTemp);
    Blynk.virtualWrite(V16,maxLight);
    Blynk.virtualWrite(V17,minLight);
  */
}

void loop() {
  Blynk.run();
  timer.run();
  EasyBuzzer.update();
  time_now = millis();

  if (time_now > prev_time + time_threshold) { //triggers hvis har gått 30 sek
    
    //Prints the values
    Blynk.virtualWrite(V12, maxDist);
    Blynk.virtualWrite(V14, maxTemp);
    Blynk.virtualWrite(V16, maxLight);
    prev_time = time_now;
    maxTemp = 0;
    maxLight = 0;
    maxDist = 0; 
  }
  if (maxDist > dist_threshold) {
      threshold_breaker_counter ++;
  }
  if (maxTemp > temp_threshold) {
    threshold_breaker_counter ++;
  }
  if (maxLight > light_threshold) {
    threshold_breaker_counter ++;
  }
  if (threshold_breaker_counter >= 2) {
    servo_alarm();
    alarm();
      
    threshold_breaker_counter = 0;
   
  }
}
