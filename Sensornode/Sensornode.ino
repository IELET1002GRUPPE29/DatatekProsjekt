/*
  Sensornode
  Sensornoden er bygget rundt en ESP32, der det er blitt brukt en MQ2 Gass sensor, Vl6180x ToF sensor og en NTC
  termistor i spenningsdeler som sensorer. I tilleg er det blitt brukt; servo motor, buzzer, lysdiode, knapp og 
  LCD skjerm som brukergrensesnitt. Sensornoden er oppkoblet mot Blynk på NTNU.io sin sky, gjennom Blynk kan man
  få grafisk innsyn i sensoravlesningene og teste servo motoren.

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
#include <LiquidCrystal_I2C.h>

WidgetTerminal terminal(V5);
WidgetLED alarmled(V14);
BlynkTimer timer;
Adafruit_VL6180X vl = Adafruit_VL6180X(); //BRUKER 0x29 I2C adresse
Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver(0x40); //0x40 I2C adresse på samme I2C linje som vl6180x
LiquidCrystal_I2C lcd(0x27, 16, 2);       //I2C LCD Skjerm bruker 0x27 som addresse på samme I2C linje...


#define SERVOMIN  125 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  575 // this is the 'maximum' pulse length count (out of 4096)

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          GLOBALE VARIABLER

//Timere
unsigned long alarmtid = 0;               //Definerer tid for millis funksjon
int alarmlengde = 30000;                  //30s alarm (Varer like lenge som lengde mellom max/min målinger, kan dermed gi kontinuerlig alarm)
unsigned long tid_nu = 0;                 //Tidsvariabel som oppdateres med millis()
unsigned long tid_prev = 0;               //Definerer tid for millis funksjon
unsigned long test_timer = 0;             //Timer for servotesting gjennom Blynk
unsigned long forrige_alarmtid  = 0;      //Tidsteller som brukes for å initiere alarmendringer (av/på med lys/buzzer/servo)f
int test_dur = 1000;                      //Tid mellom servosveip
int maks_min_tid = 30000;                 //Tid for hver maks/min avlesning
unsigned long knapptid = 0;               //Tidsteller som brukes til å registrere sit knappetrykk for å debounce
int debouncetid = 100;                    //Tid som brukes for å debounce

//Enable, teller og status verdier
bool alarmos = 0;                         //Alarmstatusverdi
bool en_boolsk_verdi_for_utregning = 0;   //Verdi som bestemmer når man kan regne ut gjennomsnittet
bool alarmstate = 1;                      //Alarmstatus 0 eller 1, bestemmer om ting skal være av eller på
bool servotest = 0;                       //Verdi som endres hvert sekund når servoen skal testes, brukes til å bestemme 180 eller 0 grader
bool pos = 0;                             //Servoposisjonsvariabel
int servo_end_state = 0;                  //Bestemmer sluttposisjon til servo etter alarm
int test_buttonState = 0;                 //Knappestatus for servotestknapp på Blynk
int selectedreading = 1;                  //Select sensor reading, 1 (LYS) by default
int readIndex = 0;                        //Indeksen til nåværende avlesning
int SERVOSWIPE[] = {SERVOMIN, SERVOMAX};  //Servoswipe array der SERVOMIN er 0 grader og SERVO max er 180 grader
int knappteller = 0;                      //Teller for knappetrykk (3 statuser)
int knappstatus = 0;                      //Knappstatus for å vite om knappen er nedtrykt eller ikke

//Utregningsvariabler
float aRead = 0;                //Analog avlesning (0-4095)
float R = 0;                    //Termistor resistans som skal utregnes
float b = 3950;                 //Termistor verdi
float R_0 = 10000;              //10k resistans i spenningsdeler med 10k NTC termistor
float T_0 = 20 + 273.15;        //Start temperatur [°C]
float temp = 0;                 //Temperatur [°C]
int gass = 0;                   //Analog avlesning for gass høyere = mer "ugass"
float lux = 0;                  //Luxverdi (lys) fra VL6180x sensor
const int numReadings = 50;               //Maks antall avlesninger som kan inngå i arrayet
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
const int buzzerPin = 5;                  //Buzzer er oppkoblet til GPIO5

//Verdier som brukes for å lagre maks og min verdier
float maxverdiTemp = 0;               //Setter maks-verdiene til 0 slik at vi vil få høyere verdier inn og overskriver
float maxverdiLux = 0;
float maxverdiGass = 0;
float minverdiTemp = 10000;           //Setter høyt slik at vi får lavere..
float minverdiLux = 10000;
float minverdiGass = 10000;

//Nettverk og Blynk
char auth[] = "thi6MWmSU17ZP4nTzTsTdojm2wV5hJ2x";   //Blynk authentication code
char ssid[] = "PBM";                                //Nettverksnavn
char pass[] = "pbmeiendom";                         //Passord til nettverket
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================
// ===============================================================================================================
//          SETUP
void setup() {
  
  Serial.begin(9600);           //Starter seriekommunikasjon

  //Vent for Serial kommunikasjon
  while (!Serial) {
    delay(1);
  }

    //Definering av pimodes
  pinMode(tempPin, INPUT);    
  pinMode(gassPin, INPUT);    
  pinMode(ledPin, OUTPUT);   
  pinMode(buzzerPin, OUTPUT); 

  ledcAttachPin(buzzerPin, 0); //Sett buzzerPin på kanal 0
  ledcSetup(0, 4000, 8); //Kanal 0, PWM frekvens, 8-bit oppløsning

  //Initiering av I2C kommunikasjon

  lcd.begin();                                //Starter LCD
  lcd.print("Trykk BOOT BTN");                //Print tekst på lcd

  pca9685.begin();                             //Starter PCA9685
  pca9685.setPWMFreq(60);                      // Servo kjører på ca. 60hz
  
  //Vent til I2C kommunikasjon er startet mellom ESP32 og VL6180x
  while (! vl.begin()) {
    Serial.println("Fant ikke Vl6180x");
    delay (1);                                 
  }
  

  Blynk.begin(auth, ssid, pass, IPAddress(91, 192, 221, 40), 8080);   //Koble opp ESP32 til nettverket og skyen   
  timer.setInterval(50L, myTimerEvent);                               //Blynk timer fopr å kjøre myTimerEvent funksjon                         
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
//En funksjon for å bestemme hvilken sensorer som det skal leses fra og sender det til V4
BLYNK_WRITE(V3) {
  switch (param.asInt()) {                              //Switch som tar input fra virtuel pin no. 3. 
    case 1: {                                           //1 = temperatur
        selectedreading = 1;
        terminal.clear();
        Blynk.setProperty(V4, "label", "Temperatur");
        Blynk.setProperty(V4, "suffix", "Temp");
        Blynk.setProperty(V4, "min", 0);
        Blynk.setProperty(V4, "max", 50);
        Blynk.setProperty(V4, "/pin/", "C");
        break;
      }
    case 2: {                                           //2 = gass
        selectedreading = 2;
        terminal.clear();
        Blynk.setProperty(V4, "label", "Gass");
        Blynk.setProperty(V4, "Name", "Gass");
        Blynk.setProperty(V4, "min", 0);
        Blynk.setProperty(V4, "max", 4095);
        break;
      }
    case 3: {                                           //3 = lux
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
//Funksjon som finner gjennomsnittet av en array med gitt lengde
float gjennomsnittArray(float * array, int len) { 
  total = 0;                                            //Setter total til 0
  for (int i = 0; i < len; i++)                         //Itererer gjennom arrayet
  { 
    total += float(array[i]);                           //Legger til verdier til total
  }

  return (float(total) / float(len));                   //Returner gjennomsnittet
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//Blynk timer event
void myTimerEvent()   //Kalles hvert x sek, som definert i setup
{
  aRead = analogRead(tempPin);                                //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                           //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(gassPin);                                 //Henter inn sensoravlesninger
  lux = vl.readLux(VL6180X_ALS_GAIN_5);
  Blynk.virtualWrite(V0, temp);                               //Sendes til Blynk
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
  if (knappteller == 1) {
    lcd.clear();
    lcd.print("Temp: " + String(temp));
  }
  if (knappteller == 2) {
    lcd.clear();
    lcd.print("Gass: " + String(gass));
  }
  if (knappteller == 3) {
    lcd.clear();
    lcd.print("Lux: " + String(lux));
  }

}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================
// ===============================================================================================================



void loop() {
  Blynk.run();
  timer.run();
  tid_nu = millis();

  if (digitalRead(0) == LOW && knappstatus == 0) {    //Dersom knappen trykkes ned og "knappstatus" er 0

    if ((tid_nu - knapptid) > debouncetid) {     //Dersom det har gått debounce tid siden sist knapptid
      knappstatus = 1;                            //Sett knappstatus til 1
      knappteller++;                              //Legg til en på telleren
      knapptid = tid_nu;                          //Ny knappetid
    }
  }
  if (knappstatus == 1 && digitalRead(0) == HIGH) {   //Dersom knappstatus er 1 og knappen er ikke nedtrykt
    knappstatus = 0;                                  //Reset til 0                    
  }
  if (knappteller > 3) {                  //Dersom knappteller overstiger 3
    knappteller = 1;                      //Skal den resettes til 1
  }


  if (test_buttonState == 1 && tid_nu > test_timer + test_dur) {    //Dersom knappen holdes ned og det har gått nok tid siden test_timer
    servotest = !servotest;                                         //Omdefiner 0->1 1->0
    pca9685.setPWM(0, 0,SERVOSWIPE[servotest]);                     //Sett Servo PWM signal til min/max (0/180) grader
    test_timer = tid_nu;                                            //Ny tid
  }

  if (alarmos == 1) {                                             //Dersom alarmverdi er 1
    if (tid_nu > 1000 + forrige_alarmtid) {                       //Dersom det har gått ett sekund siden sist alarmtid
      alarmstate = !alarmstate;     //Skift status                //Omdefiner 0->1 1->0
      alarmled.setValue(alarmstate * 255);                        //Blynk LED blink
      digitalWrite(ledPin, alarmstate);                           //Fysisk LED blink
      ledcWrite(0, 100*!alarmstate);                              //Send PWM signal på buzzer
      pca9685.setPWM(0, 0,SERVOSWIPE[alarmstate]);                //Sett Servo PWM signal til min/max (0/180) grader
      forrige_alarmtid = tid_nu;                                  //Ny tid

    }

    if (tid_nu > alarmlengde + alarmtid) {                        //Dersom det har gått "alarmlengde" tid
      servo_end_state = !servo_end_state;                         //Bytt endeposisjon for servo 0->1 1->0
      alarmled.setValue(0);                                       //Blynk LED av
      digitalWrite(ledPin, LOW);                                  //Fysisk LED av
      ledcWrite(0, 0);                                            //Skru av buzzern
      pca9685.setPWM(0, 0,SERVOSWIPE[servo_end_state]);           //Sett Servo PWM signal til min/max (0/180) grader 
      alarmos = 0;                                                //Sett alarmverdi til 0 (av)
    }

  }



  if (tid_nu > tid_prev + maks_min_tid) {   //Dersom det har gått maks_min_tid (30s) siden sist tid_prev
    if (maxverdiTemp > terskel_temp) {      //Dersom maxverdiTemp er større enn terskel_temp
      terskel_teller++;                     //Legg til en på terskel telleren
    }
    if (maxverdiLux > terskel_lux) {        
      terskel_teller++;
    }
    if (maxverdiGass > terskel_gass) {
      terskel_teller++;
    }

    if (terskel_teller >= 2) {      //Dersom to sensorer gir verdier som er over terskelverdiene
      alarmos = 1;                  //Sett alarmverdi til 1 (på)
      alarmtid = tid_nu;            //Ny alarmtid
      //Blynk.notify("ALARM");      //Fungerer ikke på NTNU.io sin sky
    }


    //Skriv verdier til blynk
    Blynk.virtualWrite(V8, maxverdiTemp);
    Blynk.virtualWrite(V9, minverdiTemp);
    Blynk.virtualWrite(V10, maxverdiLux);
    Blynk.virtualWrite(V11, minverdiLux);
    Blynk.virtualWrite(V12, maxverdiGass);
    Blynk.virtualWrite(V13, minverdiGass);

    // SÅ NULLSTILL DEM
    maxverdiTemp = 0;           //Maksimumsverdiene settes til en lav verdi          
    maxverdiLux = 0;
    maxverdiGass = 0;
    minverdiTemp = 10000;       //Minimumsverdiene settes til en høy verdi
    minverdiLux = 10000;
    minverdiGass = 10000;
    terskel_teller = 0;       //Nullstill terskelteller

    tid_prev = tid_nu;    //Definer ny tid_prev
  }
}
