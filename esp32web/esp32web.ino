
/*
  ESP32 WebServer STA Mode
  
*/

#include <WiFi.h>
#include <WebServer.h>
#include <analogWrite.h>
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include <time.h>

Adafruit_VL6180X vl = Adafruit_VL6180X(); //BRUKER 0x29 I2C adresse

unsigned long tid = 0;

float aRead = 0;                //Analog avlesning (0-4095)
float R = 0;                    //Termistor resistans som skal utregnes
float b = 3950;                 //Termistor verdi
float R_0 = 10000;              //10k resistans i spenningsdeler med 10k NTC termistor
float T_0 = 20 + 273.15;        //Start temperatur [°C]
float temp = 0;                 //Temperatur [°C]
int gass = 0;                   //Analog avlesning for gass høyere = mer "ugass"
float lux = 0;                  //Luxverdi (lys) fra VL6180x sensor

const int gassPin = 32;                   //Gass sensor er oppkoblet til GPIO32
const int tempPin = 35;                   //Termistor er oppkoblet til GPIO35

unsigned long sist_avlesning = 0;
int tid_mellom_avlesning = 5000;


const char* ntpServer = "pool.ntp.org";
const int  gmt = 3600;
const int   daylight = 3600;


// SSID & Password
const char* ssid = "PBM";  // Enter your SSID here
const char* password = "pbmeiendom";  //Enter your Password here

WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

void les_sensor() {
  aRead = analogRead(tempPin);                             //Leser av analog spenningsverdi
  R = aRead / (4095 - aRead) * R_0;                   //Regner ut termistorresistansen
  temp = - 273.15 + 1 / ((1 / T_0) + (1 / b) * log(R / R_0)); //Regner ut temperaturen i C
  gass = analogRead(gassPin);
  lux = vl.readLux(VL6180X_ALS_GAIN_5);
}

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(1);
  }

  //Vent til I2C kommunikasjon er startet mellom ESP32 og VL6180x
  if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }





  Serial.println("Try Connecting to ");
  Serial.println(ssid);
  // Connect to your wi-fi modem
  WiFi.begin(ssid, password);
  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial
  server.on("/", handle_root);
  server.begin();
  Serial.println("HTTP server started");
  delay(100);
    configTime(gmt, daylight, ntpServer);

  les_sensor(); //Les for å få verdier
  getTime();
}
String datostring = "";
String tidstring = "";
byte dd;//Dag
byte mm;//mmåned
int  yy;//År
int hh24;//Timer
int mi;//minutt
int ss;//Sekund


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
// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", getPage());
}



String getPage() {

  String page = "<html><meta charset='UTF-8'>";

  page += "<body><table><tr><th>Måling</th><th>Verdi</th>";
  page += "</tr><tr><td>Temperatur</td><td>";
  page += temp;
  page += " *C</td></tr>";

  page += "<tr><td>Lysstyrke</td><td>";
  page += lux;
  page += " Lux</td> </tr>";

  page += "<tr><td>Gass</td><td>";
  page += gass;
  page += "</td></tr>";

  page += "<tr><td>Tid</td><td>";
  page += tidstring;
  page += "</td></tr>";

  page += "<tr><td>Dato</td><td>";
  page += datostring;
  page += "</td></tr>";
  
  page += "<tr><td>Uptime</td><td>";
  page += tid / 1000;
  page += "s</td> </tr></table>";

  page += "<meta http-equiv=\"refresh\" content=\"5\">";
  page += "</body></html>";
  return (page);
}




void loop() {

  tid = millis();

  if (tid > sist_avlesning + tid_mellom_avlesning) {
    les_sensor();
    getTime();
    sist_avlesning = tid;
  }

  server.handleClient();
}
