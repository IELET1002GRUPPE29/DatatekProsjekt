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
BlynkTimer timer;
Adafruit_VL6180X vl = Adafruit_VL6180X();

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================


// ===============================================================================================================
//          GLOBALE VARIABLER
unsigned long tid = 0;    //Definerer tid for millis funksjon 
int period = 30000;        //Definerer periode (Tid mellom hver gang koden skal kjøre)
float aRead = 0;              //Analog avlesning (0-4095)
float R = 0;                  //Termistor resistans som skal utregnes
float b = 4500;               //Termistor verdi
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

const int gassPin = 33;
const int tempPin = 32;

float maxverdiTemp = 0;
float maxverdiLux = 0;
float maxverdiGass = 0;

float minverdiTemp = 0;
float minverdiLux = 0;
float minverdiGass = 0;

char auth[] = "thi6MWmSU17ZP4nTzTsTdojm2wV5hJ2x";
char ssid[] = "PBM";
char pass[] = "pbmeiendom";
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================


// ===============================================================================================================
//          SETUP


void setup() {
  Serial.begin(115200);

  //Fyller array med 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    avlesningerTemp[thisReading] = 0;
  }


  pinMode(tempPin, INPUT);
  pinMode(gassPin, INPUT);


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
void max_and_min(float temperatur, float gassniva, float lux){
  if(temperatur > maxverdiTemp){maxverdiTemp = temperatur;}
  if(temperatur < minverdiTemp){minverdiTemp = temperatur;}
  
  if(gassniva > maxverdiGass){maxverdiGass = gassniva;}
  if(gassniva < minverdiGass){minverdiGass = gassniva;}

  if(lux > maxverdiLux){maxverdiLux = lux;}
  if(lux < minverdiLux){minverdiLux = lux;}
  
  }


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


float gjennomsnittArray(float * array, int len){
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
  aRead = analogRead(gassPin);                             //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                   //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(tempPin);
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
    
        Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerTemp,relevantnumReadings));

    }


}


  if (selectedreading == 2) {
   Blynk.virtualWrite(V4, gass);
    String printstring = "The analog gas reading is: " + String(gass) + "\n";
    terminal.print(printstring);
        if (en_boolsk_verdi_for_utregning == 1) {
    //gjennomsnittArray(avlesningerGass,relevantnumReadings);

            Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerGass,relevantnumReadings));

    }

  }

  if (selectedreading == 3) {
    Blynk.virtualWrite(V4, lux);
    String printstring = "The lux measurement is: " + String(lux) + "\n";
    terminal.print(printstring);
        if (en_boolsk_verdi_for_utregning == 1) {
          Blynk.virtualWrite(V6, gjennomsnittArray(avlesningerLux,relevantnumReadings));
    }

  }






  // read from the sensor:
  avlesningerTemp[readIndex] = float(temp);
  avlesningerLux[readIndex] = float(lux);
  avlesningerGass[readIndex] = float(gass);

  max_and_min(temp,gass,lux);

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



// ===============================================================================================================
//          SETUP

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================


// ===============================================================================================================
//          SETUP

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================






void loop() {
  Blynk.run();
  timer.run();

  if(millis() > tid + period){
    // PRINT DEM FØRST
   // Serial.print("maxverdiTemp: " );
   // Serial.println(maxverdiTemp);
    Blynk.virtualWrite(V0, maxverdiTemp);
    Blynk.virtualWrite(V0, minverdiTemp);

    // SÅ NULLSTILL DEM
     maxverdiTemp = 0;
     maxverdiLux = 0;
     maxverdiGass = 0;
    
     minverdiTemp = 0;
     minverdiLux = 0;
     minverdiGass = 0;
     tid = millis();
    }
}
