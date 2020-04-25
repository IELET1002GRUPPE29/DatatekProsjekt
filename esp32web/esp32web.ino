/*
  Webserver
  Webserveren er en videreutvikling av sensornoden der sensordata plottes gjennom http på en lokal server. 
  Som tillegg har vi fått tida gjennom en NTPserver, gjennom internettet. 

  Datatek prosjekt
  Av Michael Berg og Elias Olsen Almenningen
*/


// ===============================================================================================================
//          INKLUDERING AV BIBLIOTEK OG DEFINISJON AV OBJEKTER

#include <WiFi.h>
#include <WebServer.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include <time.h>

Adafruit_VL6180X vl = Adafruit_VL6180X(); //BRUKER 0x29 I2C adresse

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          GLOBALE VARIABLER

//Timere
unsigned long tid = 0;
unsigned long sist_avlesning = 0;
int tid_mellom_avlesning = 5000;

//Utregningsvariabler
float aRead = 0;                //Analog avlesning (0-4095)
float R = 0;                    //Termistor resistans som skal utregnes
float b = 3950;                 //Termistor verdi
float R_0 = 10000;              //10k resistans i spenningsdeler med 10k NTC termistor
float T_0 = 20 + 273.15;        //Start temperatur [°C]
float temp = 0;                 //Temperatur [°C]
int gass = 0;                   //Analog avlesning for gass høyere = mer "ugass"
float lux = 0;                  //Luxverdi (lys) fra VL6180x sensor

//Pinverdier
const int gassPin = 32;                   //Gass sensor er oppkoblet til GPIO32
const int tempPin = 35;                   //Termistor er oppkoblet til GPIO35

//NTP og tid
const char* ntpServer = "pool.ntp.org";
const int  gmt = 3600;
const int   daylight = 3600;
String datostring = "";
String tidstring = "";
byte dd;//Dag
byte mm;//mmåned
int  yy;//År
int hh24;//Timer
int mi;//minutt
int ss;//Sekund


//Nettverk, SSID & Password
const char* ssid = "PBM";  // Enter your SSID here
const char* passord = "pbmeiendom";  //Enter your Password here
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)




// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          SETUP

void setup() {
  Serial.begin(115200);

  //Vent på serial kommunikasjon
  while (!Serial) {
    delay(1);
  }

  //Vent til I2C kommunikasjon er startet mellom ESP32 og VL6180x
  if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }

  Serial.println("Kobler til:  ");
  Serial.println(ssid);
  // Koble til ruteren
  WiFi.begin(ssid, passord);
  
  //Vent for wifi tilkobling
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Venter på WiFi...");
  }
  
  Serial.println("------------------------");
  Serial.println("Koblet til WiFi");
  Serial.print("Kopier denne IP addressen inn i nettleseren din: ");
  Serial.println(WiFi.localIP());                                       //IP addresse for å koble til webserveren
  server.on("/", handle_root);                                          
  server.begin();
  Serial.println("HTTP webserver startet");
  delay(100);
  
  configTime(gmt, daylight, ntpServer);         //Konfigurer tid...
  les_sensor();                                 //Få sensor verdier til å starte html siden med
  getTime();                                    //Samme for tid ^
}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          FUNKSJONER

//En funksjon for å hente tiden fra NTP serveren
void getTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  hh24 = timeinfo.tm_hour;
  mi  = timeinfo.tm_min;
  ss  = timeinfo.tm_sec;
  
  dd = timeinfo.tm_mday;
  mm = timeinfo.tm_mon + 1;
  yy = timeinfo.tm_year + 1900;

datostring = String(dd) + ":" + String(mm)+ ":" +String(yy);
tidstring = String(hh24)+  ":" +String(mi)+  ":" +String(ss);
//Bruker dd:mm:yy og hh24:mi:ss
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//Handle som kalles hver gang IPen skrives i nettleseren 

void handle_root() {
  server.send(200, "text/html", getHTML());     //Kode nr 200, siden er åpnet riktig med html koden
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//Sensor avlesning

void les_sensor() {
  aRead = analogRead(tempPin);                                //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                           //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(gassPin);                                     
  lux = vl.readLux(VL6180X_ALS_GAIN_5);
}

//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//HTML siden

String getHTML() {
  String html = "<html><meta charset='UTF-8'>";             //Lager HTML siden som en string

  html += "<body><table><tr><th>Måling</th><th>Verdi</th>";
  html += "</tr><tr><td>Temperatur</td><td>";
  html += temp;
  html += " *C</td></tr>";

  html += "<tr><td>Lysstyrke</td><td>";
  html += lux;
  html += " Lux</td> </tr>";

  html += "<tr><td>Gass</td><td>";
  html += gass;
  html += "</td></tr>";

  html += "<tr><td>Klokke</td><td>";
  html += tidstring;
  html += "</td></tr>";

  html += "<tr><td>Dato</td><td>";
  html += datostring;
  html += "</td></tr>";
  
  html += "<tr><td>Uptime</td><td>";
  html += tid / 1000;
  html += "s</td> </tr></table>";

  html += "<meta http-equiv=\"refresh\" content=\"5\">";
  html += "</body></html>";
  return (html);
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ===============================================================================================================

// ===============================================================================================================
//          LOOP

void loop() {
  tid = millis();

  if (tid > sist_avlesning + tid_mellom_avlesning) {      //Millis funksjon for å hente tid og sensorverdier
    les_sensor();
    getTime();
    sist_avlesning = tid;
  }

  server.handleClient();      //Sjekker webserveren og håndterer hendelser på html siden
}
