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
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <BlynkSimpleEsp32.h>
#include "Adafruit_VL6180X.h"

WidgetTerminal terminal(V5);
WidgetLED alarmled(V14);
BlynkTimer timer;
Adafruit_VL6180X vl = Adafruit_VL6180X(); //BRUKER 0x29 I2C adresse
Adafruit_PWMServoDriver board1 = Adafruit_PWMServoDriver(0x40); //0x40 I2C adresse på samme I2C linje som vl6180x


#define SERVOMIN  125 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  575 // this is the 'maximum' pulse length count (out of 4096)

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          GLOBALE VARIABLER
//Timere
unsigned long alarmtid = 0;     //Definerer tid for millis funksjon
int alarmlengde = 30000;        //30s alarm (Varer like lenge som lengde mellom max/min målinger, kan dermed gi kontinuerlig alarm)
unsigned long tid_nu = 0;       //Tidsvariabel som oppdateres med millis()
unsigned long tid_prev = 0;          //Definerer tid for millis funksjon
unsigned long test_timer = 0;             //Timer for servotesting gjennom Blynk
unsigned long forrige_alarmtid  = 0;      //Tidsteller som brukes for å initiere alarmendringer (av/på med lys/buzzer/servo)
int test_dur = 1000;                      //Tid mellom servosveip
int maks_min_tid = 30000;                 //Tid for hver maks/min avlesning

//Enable, teller og status verdier
bool alarmos = 0;                //Alarmstatusverdi
bool en_boolsk_verdi_for_utregning = 0;   //Verdi som bestemmer når man kan regne ut gjennomsnittet
bool alarmstate = 1;                      //Alarmstatus 0 eller 1, bestemmer om ting skal være av eller på
bool servotest = 0;                       //Verdi som endres hvert sekund når servoen skal testes, brukes til å bestemme 180 eller 0 grader
bool pos = 0;                              //Servoposisjonsvariabel
int servo_end_state = 0;                  //Bestemmer sluttposisjon til servo etter alarm
int test_buttonState = 0;                 //Knappestatus for servotestknapp på Blynk
int selectedreading = 1;                  //Select sensor reading, 1 (LYS) by default
int readIndex = 0;                        //Indeksen til nåværende avlesning

//Utregningsvariabler
float aRead = 0;                //Analog avlesning (0-4095)
float R = 0;                    //Termistor resistans som skal utregnes
float b = 3950;                 //Termistor verdi
float R_0 = 10000;              //10k resistans i spenningsdeler med 10k NTC termistor
float T_0 = 20 + 273.15;        //Start temperatur [°C]
float temp = 0;                 //Temperatur [°C]
int gass = 0;                   //Analog avlesning for gass høyere = mer "ugass"
float lux = 0;                  //Luxverdi (lys) fra VL6180x sensor
const int numReadings = 50;               //Lager en 50 lang array
int relevantnumReadings = 10;             //Det er 2-50 elementer som brukes (relevante)
float avlesningerTemp[numReadings];       //Lager en array med lengde 50 som kan holde floats m
float avlesningerGass[numReadings];       //Lager en array med lengde 50 som kan holde floats m
float avlesningerLux[numReadings];        //Lager en array med lengde 50 som kan holde floats m
float total = 0;                          //Total for å finne gjennomsnitt
float average = 0;                        //Gjennomsnittet

//Terskelverdier
int terskel_teller = 0;                   //Teller for antall sensorer som overgår maks verdien
int terskel_temp = 28;                    //Maksverdi for temperatur
int terskel_gass = 100;                   //Maksverdi for gass
int terskel_lux = 500;                    //Maksverdi for lux



//Pinverdier
const int gassPin = 32;                   //Gass sensor er oppkoblet til GPIO32
const int tempPin = 35;                   //Termistor er oppkoblet til GPIO35
const int ledPin = 23;                    //Led er oppkoblet til GPIO23




//Verdier som brukes for å lagre maks og min verdier
float maxverdiTemp = 0;               //Setter maks-verdiene til 0 slik at vi vil få høyere verdier inn og overskriver
float maxverdiLux = 0;
float maxverdiGass = 0;
float minverdiTemp = 10000;           //Setter høyt slik at vi får lavere..
float minverdiLux = 10000;
float minverdiGass = 10000;


//Nettverk og Blynk
char auth[] = "thi6MWmSU17ZP4nTzTsTdojm2wV5hJ2x";   //Blynk authentication code
char ssid[] = "PBM";                                //Nettverk
char pass[] = "pbmeiendom";                         //Passord
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================
// ===============================================================================================================
//          SETUP

void setup() {

  Serial.begin(9600);

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

void maks_min(float temperatur, float gassniva, float lux) {
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

BLYNK_WRITE(V15) {
  test_buttonState = param.asInt();
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

//Slider for numreadings
BLYNK_WRITE(V7) {
  relevantnumReadings = param.asInt();
  en_boolsk_verdi_for_utregning = 0;
  total = 0;
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


float gjennomsnittArray(float * array, int len) {
  total = 0;
  for (int i = 0; i < len; i++)
  {
    total += float(array[i]);
  }

  return (float(total) / float(len));
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


void myTimerEvent()   //Kalles hvert x sek
{
  aRead = analogRead(tempPin);                             //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                   //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(gassPin);
  lux = vl.readLux(VL6180X_ALS_GAIN_5);
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, gass);
  Blynk.virtualWrite(V2, lux);


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


  // Hent inn verdier:

  avlesningerTemp[readIndex] = float(temp);
  avlesningerLux[readIndex] = float(lux);
  avlesningerGass[readIndex] = float(gass);
  maks_min(temp, gass, lux);
  //Iterer til neste posisjon i arrayet:
  readIndex = readIndex + 1;

  Serial.println(readIndex);

  // Dersom vi er på enden av det relevante spektrumet av arrayen..

  if (readIndex >= relevantnumReadings) {
    // Start om igjen..
    en_boolsk_verdi_for_utregning = 1;
    readIndex = 0;
  }
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&


//COURTESY OF ROBOJAX
int angleToPulse(int ang) {
  int pulse = map(ang, 0, 180, SERVOMIN, SERVOMAX); // map angle of 0 to 180 to Servo min and Servo max
  return pulse;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================
// ===============================================================================================================



void loop() {
  Blynk.run();
  timer.run();
  tid_nu = millis();

  if (test_buttonState == 1 && tid_nu > test_timer + test_dur) {    //Dersom knappen holdes ned og det har gått nok tid siden test_timer
    servotest = !servotest;                                         //Omdefiner 0->1 1->0 
    board1.setPWM(0, 0, angleToPulse(180 * servotest) );            //Send instruks til servo på brett 0 og kanal 0 til å være 180 eller 0 grader
    test_timer = tid_nu;                                            //Ny tid
  }

  if (alarmos == 1) {                                             //Dersom alarmverdi er 1
    if (tid_nu > 1000 + forrige_alarmtid) {                       //Dersom det har gått ett sekund siden sist alarmtid
      alarmstate = !alarmstate;     //Skift status                //Omdefiner 0->1 1->0
      alarmled.setValue(alarmstate * 255);                        //Blynk LED blink
      digitalWrite(ledPin, alarmstate);                           //Fysisk LED blink 
      analogWrite(5, 100 * alarmstate);                           //Buzzer buzz
      board1.setPWM(0, 0, angleToPulse(180 * alarmstate) );       //Servo swipe
      forrige_alarmtid = tid_nu;                                  //Ny tid

    }

    if (tid_nu > alarmlengde + alarmtid) {                        //Dersom det har gått "alarmlengde" tid
      servo_end_state = !servo_end_state;                         //Bytt endeposisjon for servo 0->1 1->0
      alarmled.setValue(0);                                       //Blynk LED av
      digitalWrite(ledPin, LOW);                                  //Fysisk LED av
      analogWrite(5, 0);                                          //Buzzer av
      board1.setPWM(0, 0, angleToPulse(180 * servo_end_state) );  //Servo settes til 180 eller 0 grader
      alarmos = 0;                                                //Sett alarmverdi til 0 (av) 
    }

  }



  if (tid_nu > tid_prev + maks_min_tid) {   //Dersom det har gått maks_min_tid (30s) siden sist tid_prev
    if (maxverdiTemp > terskel_temp) {      //Dersom maxverdiTemp er større enn terskel_temp
      terskel_teller++;                     //Legg til en på terskel telleren 
    }
    if (maxverdiLux > terskel_lux) {        //Samme som ovenfor
      terskel_teller++;
    }
    if (maxverdiGass > terskel_gass) {
      terskel_teller++;
    }

    if (terskel_teller >= 2) {      //Dersom to sensorer gir verdier som er over terskelverdiene
      alarmos = 1;                  //Sett alarmverdi til 1 (på)
      alarmtid = tid_nu;            //Ny alarmtid
      //Blynk.notify("ALARM");
    }


    //Skriv verdier til blynk
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

    tid_prev = tid_nu;    //Sett ny tid
  }
}
