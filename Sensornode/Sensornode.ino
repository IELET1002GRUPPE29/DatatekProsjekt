#include <Servo.h>


Servo myservo;  // create servo object to control a servo
int pos = 0;    // variable to store the servo position
int servoPin = 33;
bool ledstate = 1;
int ltid  = 0; //DDDDDDDDDD
int terskel_teller = 0;
int terskel_temp = 28;
int terskel_gass = 100;
int terskel_lux = 500;
#include <Wire.h>

#include <Adafruit_PWMServoDriver.h>
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  125 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  575 // this is the 'maximum' pulse length count (out of 4096)

//******BUZZER*******
#define LEDC_CHANNEL_0  0
#define LEDC_TIMER_13_BIT 13
#define LEDC_BASE_FREQ 5000
#define LED_PIN 5
int freq = 2000;
int channel = 0;
int resolution = 8;


//^^^^^^^^BUZZER^^^^^^^^


/*
  Sensornode
  Describe what it does in layman's terms.  Refer to the components
  attached to the various pins.
  The circuit:
    list the components attached to each input
    list the components attached to each output
  Datatek prosjekt
  Av Michael Berg
*/


// ===============================================================================================================
//          INKLUDERING AV BIBLIOTEK OG DEFINISJON AV OBJEKTER

#define BLYNK_PRINT Serial
#include <analogWrite.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "Adafruit_VL6180X.h"
WidgetTerminal terminal(V5);
WidgetLED alarmled(V14);
BlynkTimer timer;
Adafruit_VL6180X vl = Adafruit_VL6180X(); //BRUKER 0x29 I2C adresse

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          GLOBALE VARIABLER

unsigned long alarmtid = 0;    //Definerer tid for millis funksjon
int alarmlengde = 30000; //30s alarm (Varer like lenge som lengde mellom max/min målinger, kan dermed gi kontinuerlig alarm)
int alarmos = 0;
unsigned long tid_nu = 0;
unsigned long tid = 0;    //Definerer tid for millis funksjon
int period = 30000;        //Definerer periode (Tid mellom hver gang koden skal kjøre)
float aRead = 0;              //Analog avlesning (0-4095)
float R = 0;                  //Termistor resistans som skal utregnes
float b = 3950;               //Termistor verdi
float R_0 = 10000;            //10k resistans i spenningsdeler med 10k NTC termistor
float T_0 = 20 + 273.15;      //Start temperatur [°C]
float temp = 0;               //Temperatur [°C]
int gass = 0;                 //Analog avlesning for gass høyere = mer "ugass"
float lux = 0;
int selectedreading = 1;      //Select sensor reading, 1 (LYS) by default
const int numReadings = 50;        //Lager en 50 lang array
int relevantnumReadings = 10;     //Det er 2-50 elementer som brukes (relevante)
float avlesningerTemp[numReadings];      //Lager en array med lengde 50 som kan holde floats m
float avlesningerGass[numReadings];      //Lager en array med lengde 50 som kan holde floats m
float avlesningerLux[numReadings];      //Lager en array med lengde 50 som kan holde floats m
int readIndex = 0;                //Indeksen til nåværende avlesning
float total = 0;                  //Total for å finne gjennomsnitt
float average = 0;                //Gjennomsnittet
bool en_boolsk_verdi_for_utregning = 0;

const int gassPin = 32;
const int tempPin = 35;
const int ledPin = 23;


float maxverdiTemp = 0;
float maxverdiLux = 0;
float maxverdiGass = 0;
float minverdiTemp = 10000; //Setter høyt slik at vi får lavere..
float minverdiLux = 10000;
float minverdiGass = 10000;

char auth[] = "thi6MWmSU17ZP4nTzTsTdojm2wV5hJ2x";
char ssid[] = "PBM";
char pass[] = "pbmeiendom";
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================


// ===============================================================================================================
//          SETUP

void setup() {
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
  Serial.begin(9600);
  //myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin);//, 250, 2300); // attaches the servo on pin 15 to the servo object
  //Fyller array med 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    avlesningerTemp[thisReading] = 0;
}

  board1.begin();
  board1.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

  
  pinMode(tempPin, INPUT);
  pinMode(gassPin, INPUT);
  pinMode(ledPin, OUTPUT);
  //Vent for Serial kommunikasjon
 
  while (!Serial) {
    delay(1);
  }
  
  //Vent til I2C kommunikasjon er startet mellom ESP32 og VL6180x
  if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  
  Blynk.begin(auth, ssid, pass, IPAddress(91, 192, 221, 40), 8080);
  timer.setInterval(50L, myTimerEvent);
}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          FUNKSJONER

void max_and_min(float temperatur, float gassniva, float lux) {
  if (temperatur > maxverdiTemp) {
    maxverdiTemp = temperatur;
  }
  if (temperatur < minverdiTemp) {
    minverdiTemp = temperatur;
  }
  if (gassniva > maxverdiGass) {
    maxverdiGass = gassniva;
  }
  if (gassniva < minverdiGass) {
    minverdiGass = gassniva;
  }
  if (lux > maxverdiLux) {
    maxverdiLux = lux;
  }
  if (lux < minverdiLux) {
    minverdiLux = lux;
  }
};

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

BLYNK_WRITE(V3) {
  switch (param.asInt()) {
    case 1: {
        selectedreading = 1;
        terminal.clear();
        Blynk.setProperty(V4, "label", "Temperatur");
        Blynk.setProperty(V4, "suffix", "Temp");
        Blynk.setProperty(V4, "min", 0);
        Blynk.setProperty(V4, "max", 50);
        Blynk.setProperty(V4, "/pin/", "C");
        break;
      }
    case 2: {
        selectedreading = 2;
        terminal.clear();
        Blynk.setProperty(V4, "label", "Gass");
        Blynk.setProperty(V4, "Name", "Gass");
        Blynk.setProperty(V4, "min", 0);
        Blynk.setProperty(V4, "max", 4095);
        break;
      }
    case 3: {
        selectedreading = 3;
        terminal.clear();
        Blynk.setProperty(V4, "name", "lux");
        Blynk.setProperty(V4, "label", "Lux");
        Blynk.setProperty(V4, "min", 0);
        Blynk.setProperty(V4, "max", 4095);
        break;
      }
  }
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

//Slider for numreadings
BLYNK_WRITE(V7) {
  relevantnumReadings = param.asInt();
  en_boolsk_verdi_for_utregning = 0;
  total = 0;
  //Serial.println(relevantnumReadings);
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


float gjennomsnittArray(float * array, int len) {
  total = 0;
  for (int i = 0; i < len; i++)
  {
    total += float(array[i]);
  }
  //average = total / float(relevantnumReadings);
  //Blynk.virtualWrite(V6, average);
  //Serial.println(average);
  //Serial.println(relevantnumReadings);
  return (float(total) / float(len));
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


void myTimerEvent()
{
  aRead = analogRead(tempPin);                             //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                   //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(gassPin);
  lux = vl.readLux(VL6180X_ALS_GAIN_5);
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, gass);
  Blynk.virtualWrite(V2, lux);

  /*
    Serial.print(millis());
    Serial.print(" Temperatur: ");
    Serial.print(temp);
    Serial.print(" Gass: ");
    Serial.print(gass);
    Serial.print(" lux: ");
    Serial.print(lux);
  
  */
  
  if (selectedreading == 1) {
    Blynk.virtualWrite(V4, temp);
    String printstring = "The temperature is: " + String(temp) + "°C\n";
    terminal.print(printstring);
    if (en_boolsk_verdi_for_utregning == 1) {
      Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerTemp, relevantnumReadings));
    }
  }
  
  if (selectedreading == 2) {
    Blynk.virtualWrite(V4, gass);
    String printstring = "The analog gas reading is: " + String(gass) + "\n";
    terminal.print(printstring);
    if (en_boolsk_verdi_for_utregning == 1) {
      //gjennomsnittArray(avlesningerGass,relevantnumReadings);
      Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerGass, relevantnumReadings));
    }
  }
  
  if (selectedreading == 3) {
    Blynk.virtualWrite(V4, lux);
    String printstring = "The lux measurement is: " + String(lux) + "\n";
    terminal.print(printstring);
    if (en_boolsk_verdi_for_utregning == 1) {
      Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerLux, relevantnumReadings));
    }
  }
  
  
  // read from the sensor:
  avlesningerTemp[readIndex] = float(temp);
  avlesningerLux[readIndex] = float(lux);
  avlesningerGass[readIndex] = float(gass);
  max_and_min(temp, gass, lux);
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  
  Serial.println(readIndex);
  
  // Dersom vi er på enden av det relevante spektrumet av arrayen..
  
  if (readIndex >= relevantnumReadings) {
    // Start om igjen..
    en_boolsk_verdi_for_utregning = 1;
    readIndex = 0;
  }






  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // ===============================================================================================================
}

  int angleToPulse(int ang){
   int pulse = map(ang,0, 180, SERVOMIN,SERVOMAX);// map angle of 0 to 180 to Servo min and Servo max 
   Serial.print("Angle: ");Serial.print(ang);
   Serial.print(" pulse: ");Serial.println(pulse);
   return pulse;
}

// ===============================================================================================================
//          SETUP
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================
// ===============================================================================================================
//          SETUP
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================



void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);
  ledcWrite(channel, duty);
};


//int statssss = 0;
void loop() {
  Blynk.run();
  timer.run();
  tid_nu = millis();

  
  //myservo.write(180);
  if (alarmos == 1) {
    
    //LEDFLASH
    
    if (tid_nu > 1000 + ltid){// && tid_nu <2000 + ltid && statssss == 0) {  //Dersom det har gått 500ms siden sist tid
      ledstate = !ledstate;     //Skift status
            Serial.println("ALARM 2");
            
      //alarmled.setValue(ledstate*255);
      digitalWrite(ledPin, ledstate);   //Lys
      //ledcWriteTone(channel, freq * !ledstate);
analogWrite(5, 100*ledstate);    
board1.setPWM(0, 0, angleToPulse(180*ledstate) );
//statssss = 1;



    }
    /*
    if (tid_nu > 2000 + ltid && tid_nu < 3000 + ltid && statssss == 1) {  //Dersom det har gått 500ms siden sist tid

                Serial.println("ALARM 3");
                myservo.write(180*ledstate);
                statssss = 2;

                
    }
    if (tid_nu > 3000 + ltid && statssss == 2){                
      statssss = 0;
      ltid = tid_nu;                  //Definer ny tid
}


*/
    };
  if (tid_nu > alarmlengde + alarmtid) {
    alarmos = 0; // Skru av alarm

  };
  if(alarmos == 0){
        digitalWrite(ledPin, LOW);
analogWrite(5, 0);    
    //ledcWriteTone(channel, 0);
    // myservo.write(180);
    }
  
  
  if (tid_nu > tid + 20000) {

    
    Serial.println("max");
    // PRINT DEM FØRST
    // Serial.print("maxverdiTemp: " );
    // Serial.println(maxverdiTemp);
  
    if (maxverdiTemp > terskel_temp) {
      terskel_teller++;
    }
    if (maxverdiLux > terskel_lux) {
      terskel_teller++;
    }
    if (maxverdiGass > terskel_gass) {
      terskel_teller++;
    }
    
    
    Serial.println(terskel_teller + 100);
    
    if (terskel_teller >= 2) {
      alarmos = 1;
      alarmtid = tid_nu;
      Serial.println("ALARM");
      //Blynk.notify("ALARM");
    
    }
    
    
    
    Blynk.virtualWrite(V8, maxverdiTemp);
    Blynk.virtualWrite(V9, minverdiTemp);
    Blynk.virtualWrite(V10, maxverdiLux);
    Blynk.virtualWrite(V11, minverdiLux);
    Blynk.virtualWrite(V12, maxverdiGass);
    Blynk.virtualWrite(V13, minverdiGass);
    
    // SÅ NULLSTILL DEM
    maxverdiTemp = 0;
    maxverdiLux = 0;
    maxverdiGass = 0;
    minverdiTemp = 10000;   //settes til en høy verdi
    minverdiLux = 10000;
    minverdiGass = 10000;
    terskel_teller = 0; //Nullstill terskelteller
    
    tid = tid_nu;
  }
}
