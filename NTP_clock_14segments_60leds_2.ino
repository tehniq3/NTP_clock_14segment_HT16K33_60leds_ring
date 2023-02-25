/*******************************************************************************
// https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.0 - changed date mode (day.mounth not month/day)
// v.1 - added hardware switch for DST (Daylight Saving Time)
// v.2 - added WiFiManager library 
// HT16K33_v.1 - changed to HT16K33 4-digit 14-segment display
// v.2 - added month, year and scrolling name day name
// v.3 - added DHT22 (AM2302) thermometer/hygrometer
// v.5 - added a lot of animation (maybe too much)
// v.5a - added animation also at enter the date and exit of year
// v.6 - added automatic brightness due to sunrise/sunset using https://github.com/jpb10/SolarCalculator library
// combo (HT16K33 - 14 segment led display + 60leds ring (WS2812b)
// v.2 - added 60leds ring (WS2812b) + moved part of display mode into subroutine
 
 *******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include "NoiascaHt16k33.h"                                // include the noiasca HT16K33 library - download from http://werner.rothschopf.net/
#include <FastLED.h>
#include "DHTesp.h" // Click here to get the library: https://www.arduinolibraries.info/libraries/dht-sensor-library-for-es-px
#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator


//Noiasca_ht16k33_hw_14 display = Noiasca_ht16k33_hw_14();   // 14 segment - Present time
Noiasca_ht16k33_hw_14_ext display = Noiasca_ht16k33_hw_14_ext();     // 14 segment, extended class with scroll support

#define DSTpin 14 // GPIO14 = D5, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
#define DHTPIN 13 // GPIO13 = D7    // what pin we're connected the DHT output
// Set this to true if you want the hour LED to move between hours (if set to false the hour LED will only move every hour)
#define NUM_LEDS 60     
#define DATA_PIN 12 // GPIO12 = D6 see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

#define USE_LED_MOVE_BETWEEN_HOURS true

// WS2812b day / night brightness.
#define NIGHTBRIGHTNESS 4      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 20

// HT16K33 day / night brightness.
#define NBRIGHTNESS 2      // Brightness level from 0 (off) to 15 (full brightness)
#define DBRIGHTNESS 10

DHTesp dht;

const long timezoneOffset = 2; // ? hours

const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed

unsigned long tpafisare;
unsigned long tpinfo = 60000;  // 15000 for test in video

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

byte DST = 0;
byte DST0;
bool updated;
byte a = 0;

  unsigned int year;
  unsigned int month;
  unsigned int day;
  unsigned int hour;
  unsigned int minute;
  unsigned int second;

int i = 0;
int j = 4;

int pauzamica = 100;  // step time (speed) for animation
//int pauzamare = 3000; // step time for static info
int pauzemari = 30; // x pauzamica [ms], ex= 30x 100 if pauzamica = 100 [ms]
int pauzamedie = 300;  // step time (speed) for info 

int ora, minut;

int humidity = 0;
int temperature = 0;
int temperature2 = 0;
int zi = 0;

int n = 0;

const String weekDays[7][2] = {
  "    Saturday    ", "    Duminica    ",
  "    Monday    ", "    Luni    ",
  "    Tuesday    ", "    Marti    ",
  "    Wednesday    ", "    Miercuri    ",
  "    Thursday   ", "    Joi    ",
  "    Friday    ", "    Vineri    ",
  "    Sunday    ", "    Sambata    "
  };  // english, lb. romana

String informatii[5][2] = {
  "    Clock    ", "    Ora    ",   // clock in english & lb. romana
  "    Date    ", "    Data    ",   // date in english & lb. romana
  "    Year    ", "    An    ",     // an in english & lb. romana
  "    Temperature    ", "    Temperatura    ",     // temperature in english & lb. romana
  "    Humidity    ", "    Umiditate    "     // temperature in english & lb. romana   
  };
byte b = 0; // 0 = clock, 1 - date (day.month), 2 - year, 3 - temperature, 4 -humidity
String info1;
char info2[20];

const String intro = {"    NTP clock with DHT sensor on 2 displays by Nicu FLORICA (niq_ro) v-2          "};

// Location - Craiova: 44.317452,23.811336
double latitude = 44.31;
double longitude = 23.81;
double transit, sunrise, sunset;


char str[6];
int ora1, ora2, oraactuala;
int m1, hr1, mn1, m2, hr2, mn2; 
//int ora, minut, secunda, rest;
int ora0, minut0;
byte ziua, luna, an;


// Change the colors here if you want.
// Check for reference: https://github.com/FastLED/FastLED/wiki/Pixel-reference#predefined-colors-list
// You can also set the colors with RGB values, for example red:
// CRGB colorHour = CRGB(255, 0, 0);
byte ics = 64;
CRGB fundal = CRGB(ics, ics, ics);
CRGB colorHour = CRGB::Red;
CRGB colorMinute = CRGB::Green;
CRGB colorSecond = CRGB::Blue;
CRGB colorHourMinute = CRGB::Yellow;
CRGB colorHourSecond = CRGB::Magenta;
CRGB colorMinuteSecond = CRGB::Cyan;
CRGB colorAll = CRGB::White;

CRGB LEDs[NUM_LEDS];

byte inel_ora;
byte inel_minut;
byte inel_secunda;
byte secundar;


void setup() {
 pinMode (DSTpin, INPUT);
 // Autodetect is not working reliable, don't use the following line
  // dht.setup(17);
  // use this instead: 
  dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
  delay(500);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.clear();
  
     //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    wifiManager.autoConnect("AutoConnectAP");
      
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    delay(500);

  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
  DST0 = DST;
  readDHT();  
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("\nScroll a text"));
  Wire.begin();                                  // start the I2C interface
  //Wire.setClock(400000);                       // optional: activate I2C fast mode. If it is to fast for other I2C devices deactivate this row.
  display.begin(0x70, 1);                        // I2C adress of first display, in total we use 3 displays
  if (display.isConnected() == false)            // check if all HT16K33 are connected
    Serial.println(F("E: display error"));
  display.setDigits(4);                          // if your modules are not using all 8 digits, reduce the used digits
  display.setBrightness(2);                      // set brightness from 0 to 15
display.clear();

delay(4000);
    char intro_char[88];
    intro.toCharArray(intro_char,intro.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (intro_char);
    
j = 0;
while (j <= intro.length()-4)
  {
    display.print(intro_char[j]);
    display.print(intro_char[j+1]);
    display.print(intro_char[j+2]);
    display.print(intro_char[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();
  
 readDHT();  // read DHT sensor 
  
  display.setBrightness(DBRIGHTNESS);
      display.print(12);
      display.print(F("."));
      display.print(17);

  FastLED.setBrightness(DAYBRIGHTNESS); //Number 0-255
    LEDs[32] = colorHour;   // 2 oçlock
    LEDs[47] = colorMinute; // 7 minutes
    LEDs[42] = colorSecond; // 12 seconds  

for (int i=0; i<12; i++)  // show important points
      {
        LEDs[i*5] = fundal;  
        LEDs[i*5].fadeToBlackBy(25);      
      }
  FastLED.show();
  delay(3000);
  FastLED.setBrightness(NIGHTBRIGHTNESS); //Number 0-255
  FastLED.show();
  display.setBrightness(NBRIGHTNESS);
  delay(3000);
  FastLED.clear();

Soare();
if (night())
      {
        FastLED.setBrightness (NIGHTBRIGHTNESS);
        display.setBrightness(NBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      FastLED.setBrightness (DAYBRIGHTNESS);
       display.setBrightness(DBRIGHTNESS); 
      }
    FastLED.show();
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }
 
unsigned long t = millis();

year = getYear();
month = getMonth();
day = getDate();
zi = timeClient.getDay();

hour = timeClient.getHours();
minute = timeClient.getMinutes();
second = timeClient.getSeconds();
ora = hour;
minut = minute;
secundar = second;
readDHT();


// clock (description)
b = 0; // 0 = clock, 1 - date (day.month), 2 - year, 3 - temperature, 4 -humidity
    info1 = informatii[b][a%2];
    info1.toCharArray(info2,info1.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (info2);

 
j = 0;
while (j <= info1.length()-4)
  {
    afisareinel();
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();

// clock 
// in
  Serial.print(ora);
  Serial.print(":");
  Serial.println(minut);
  
    String intrare = "    ";
    if (ora < 10) 
        intrare = intrare + " ";
        else
        intrare = intrare + ora/10;
    intrare = intrare + ora%10;
    intrare = intrare + minut/10;
    intrare = intrare + minut%10;    
    Serial.println (intrare);
    char intrare_char[9];
    intrare.toCharArray(intrare_char,intrare.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
        
Serial.println (intrare_char);  
j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(intrare_char[j]);
    display.print(intrare_char[j+1]);
    display.print(intrare_char[j+2]);
    display.print(intrare_char[j+3]);
    delay(pauzamica);
    j++;
  }

display.clear();

//static
int s = 0;
while (s < 30)
{
    afisareinel();
   //   if (ora < 10) display.print(F("0"));
      if (ora < 10) display.print(F(" "));
      display.print(ora);
      if (millis()/1000%2 == 0)
      display.print(F("."));
      if (minut < 10) display.print(F("0"));
      display.print(minut);  
      delay(500); 
      s++;
      Serial.print("s = ");
      Serial.println(s);
}
//delay(4000);
display.clear();
  
    String iesire = "";
    if (ora < 10) 
        iesire = iesire + " ";
        else
        iesire = iesire + ora/10;
    iesire = iesire + ora%10;
    iesire = iesire + minut/10;
    iesire = iesire + minut%10;
    iesire = iesire + "    "; 
    Serial.print("iesire = "); 
    Serial.println (iesire);
    char iesire_char[9];
    iesire.toCharArray(iesire_char,iesire.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (iesire_char);

j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(iesire_char[j]);
    display.print(iesire_char[j+1]);
    display.print(iesire_char[j+2]);
    display.print(iesire_char[j+3]);
    delay(pauzamica);
    j++;
  }
display.clear();


//display.clear();
    String weekDay = weekDays[zi][a%2];
    char numezi[20];
    weekDay.toCharArray(numezi,weekDay.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (numezi);
    
j = 0;
while (j <= weekDay.length()-4)
  {
    afisareinel();
    display.print(numezi[j]);
    display.print(numezi[j+1]);
    display.print(numezi[j+2]);
    display.print(numezi[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();

// data (description)
b = 1; // 0 = clock, 1 - date (day.month), 2 - year, 3 - temperature, 4 -humidity
    info1 = informatii[b][a%2];
    info1.toCharArray(info2,info1.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (info2);

display.clear();   
j = 0;
while (j <= info1.length()-4)
  {
    afisareinel();
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();

// in of date
    String intrare3 = "    ";
    intrare3 = intrare3 + day/10;
    intrare3 = intrare3 + day%10;
    intrare3 = intrare3 + month/10;
    intrare3 = intrare3 + month%10;    
    Serial.println (intrare3);
    char intrare_char3[9];
    intrare3.toCharArray(intrare_char3,intrare3.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
        
Serial.println (intrare_char3);  
j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(intrare_char3[j]);
    display.print(intrare_char3[j+1]);
    display.print(intrare_char3[j+2]);
    display.print(intrare_char3[j+3]);
    delay(pauzamica);
    j++;
  }
display.clear();


//static data
   if (day < 10) display.print(F("0"));
      display.print(day);
      display.print(F("."));
      if (month < 10) display.print(F("0"));
      display.print(month); 
      display.print(F("."));
//delay(pauzamare);
j = 0;
while (j <= pauzemari)
  {
    afisareinel();
    delay(pauzamica);
    j++;
  }
display.clear();

      display.print(year);
      display.print(F("."));
//delay(pauzamare);
j = 0;
while (j <= pauzemari)
  {
    afisareinel();
    delay(pauzamica);
    j++;
  }
display.clear();

// gone
    String iesire4 = "";
    iesire4 = iesire4 + year;
    iesire4 = iesire4 + "    "; 
    Serial.print("iesire4 = "); 
    Serial.println (iesire4);
    char iesire_char4[9];
    iesire4.toCharArray(iesire_char4,iesire4.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (iesire_char4);

j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(iesire_char4[j]);
    display.print(iesire_char4[j+1]);
    display.print(iesire_char4[j+2]);
    display.print(iesire_char4[j+3]);
    delay(pauzamica);
    j++;
  }
display.clear(); 


// temperature (description)
b = 3;  // 0 = clock, 1 - date (day.month), 2 - year, 3 - temperature, 4 -humidity
    info1 = informatii[b][a%2];
    info1.toCharArray(info2,info1.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (info2);

display.clear();   
j = 0;
while (j <= info1.length()-4)
  {
    afisareinel();
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();

// temperature
//in
    String intrare2 = "    ";
    if (temperature >= 0)
      {
        temperature2 = temperature;
      if (temperature2/10 == 0)
      intrare2 = intrare2 + " ";
      else
      intrare2 = intrare2 + temperature2/10;
      intrare2 = intrare2 + temperature2%10;
      intrare2 = intrare2 + "%";
      intrare2 = intrare2 + "C"; 
      }  
    else  // if temperture is negative
     {
      temperature2 = -temperature;
      intrare2 = intrare2 + "-";
       if (temperature2 < 10)
       {
       intrare2 = intrare2 + temperature2%10;
       intrare2 = intrare2 + "%";
       intrare2 = intrare2 + "C";
       }
       else
       {
       intrare2 = intrare2 + temperature2/10;
       intrare2 = intrare2 + temperature2%10;
       intrare2 = intrare2 + "C"; 
       } 
     }
    Serial.println (intrare2);
    char intrare_char2[9];
    intrare2.toCharArray(intrare_char2,intrare2.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
        
Serial.println (intrare_char2);  
j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(intrare_char2[j]);
    display.print(intrare_char2[j+1]);
    display.print(intrare_char2[j+2]);
    display.print(intrare_char2[j+3]);
    delay(pauzamica);
    j++;
  }

// static
if (temperature >= 0)
 {
  temperature2 = temperature;
  if (temperature2 < 10) display.print(F(" "));
  display.print(temperature2);
  display.print("%C"); 
 }
else
 {
  temperature2 = -temperature;
  display.print("-");
  display.print(temperature2);
  if (temperature2 < 10)
  display.print("%C"); 
  else
  display.print("C");  
 }
//  delay(pauzamare);
j = 0;
while (j <= pauzemari)
  {
    afisareinel();
    delay(pauzamica);
    j++;
  }
// gone
    String iesire2 = "";
   if (temperature >= 0)
      {
      temperature2 = temperature;
      if (temperature2/10 == 0)
      iesire2 = iesire2 + " ";
      else
      iesire2 = iesire2 + temperature2/10;
      iesire2 = iesire2 + temperature2%10;
      iesire2 = iesire2 + "%";
      iesire2 = iesire2 + "C"; 
      }  
    else  // if temperature is negative
     {
      temperature2 = -temperature;
      iesire2 = iesire2 + "-";
       if (temperature2 < 10)
       {
       iesire2 = iesire2 + temperature2%10;
       iesire2 = iesire2 + "%";
       iesire2 = iesire2 + "C";
       }
       else
       {
       iesire2 = iesire2 + temperature2/10;
       iesire2 = iesire2 + temperature2%10;
       iesire2 = iesire2 + "C"; 
       } 
     }
     iesire2 = iesire2 + "    "; 
       
    Serial.print("iesire2 = "); 
    Serial.println (iesire2);
    char iesire_char2[9];
    iesire2.toCharArray(iesire_char2,iesire2.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (iesire_char2);

j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(iesire_char2[j]);
    display.print(iesire_char2[j+1]);
    display.print(iesire_char2[j+2]);
    display.print(iesire_char2[j+3]);
    delay(pauzamica);
    j++;
  }
display.clear();


// humidity (description)
b = 4;  // 0 = clock, 1 - date (day.month), 2 - year, 3 - temperature, 4 -humidity
    info1 = informatii[b][a%2];
    info1.toCharArray(info2,info1.length()); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (info2);

display.clear();   
j = 0;
while (j <= info1.length()-4)
  {
    afisareinel();
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
display.clear();

// humidity
 if (humidity >= 100) humidity = 99;

// in
    String intrare1 = "    ";
    if (humidity/10 == 0) 
    intrare1 = intrare1 + " ";
    else
    intrare1 = intrare1 + humidity/10;
    intrare1 = intrare1 + humidity%10;
    intrare1 = intrare1 + "%";
    intrare1 = intrare1 + "o";    
    Serial.println (intrare1);
    char intrare_char1[9];
    intrare1.toCharArray(intrare_char1,intrare1.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
        
Serial.println (intrare_char1);  
j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(intrare_char1[j]);
    display.print(intrare_char1[j+1]);
    display.print(intrare_char1[j+2]);
    display.print(intrare_char1[j+3]);
    delay(pauzamica);
    j++;
  }

// static
  if (humidity < 10) display.print(F(" "));
  display.print(humidity);
  display.print("%o"); 
//  delay(pauzamare);
j = 0;
while (j <= pauzemari)
  {
    afisareinel();
    delay(pauzamica);
    j++;
  }
  display.clear();

// gone
    String iesire1 = "";
    if (humidity/10 == 0) 
    iesire1 = iesire1 + " ";
    else
    iesire1 = iesire1 + humidity/10;
    iesire1 = iesire1 + humidity%10;
    iesire1 = iesire1 + "%";;
    iesire1 = iesire1 + "o";;
    iesire1 = iesire1 + "    "; 
    Serial.print("iesire1 = "); 
    Serial.println (iesire1);
    char iesire_char1[9];
    iesire1.toCharArray(iesire_char1,iesire1.length()+1); // https://www.tutorialspoint.com/convert-string-to-character-array-in-arduino
    Serial.println (iesire_char1);

j = 0;
while (j <= 4)
  {
    afisareinel();
    display.print(iesire_char1[j]);
    display.print(iesire_char1[j+1]);
    display.print(iesire_char1[j+2]);
    display.print(iesire_char1[j+3]);
    delay(pauzamica);
    j++;
  }
display.clear(); 

n++;
if (n > 4) n=0;
a++;
i++;
if (i>6) i=0;

if (DST != DST0)
{
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
DST0 = DST;
}
Soare();
}  // end main loop                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               

void readDHT()
{
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();  
  Serial.println(" ");
  Serial.print("t = ");
  Serial.print(temperature);
  Serial.print("C h = ");
  Serial.print(humidity);
  Serial.println("%"); 
}

unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int year = ti->tm_year + 1900;
  return year;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mon + 1;
  return month;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mday;
  return month;
}

void Soare()
{
   // Calculate the times of sunrise, transit, and sunset, in hours (UTC)
  calcSunriseSunset(year, month, day, latitude, longitude, transit, sunrise, sunset);

m1 = int(round((sunrise + timezoneOffset + DST) * 60));
hr1 = (m1 / 60) % 24;
mn1 = m1 % 60;

m2 = int(round((sunset + timezoneOffset + DST) * 60));
hr2 = (m2 / 60) % 24;
mn2 = m2 % 60;

  Serial.print("Sunrise = ");
  Serial.print(sunrise+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr1);
  Serial.print(":");
  Serial.print(mn1);
  Serial.println(" ! ");
  Serial.print("Sunset = ");
  Serial.print(sunset+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr2);
  Serial.print(":");
  Serial.print(mn2);
  Serial.println(" ! ");
}

void afisareinel()
{
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  if (DST != DST0)
  {
  timeClient.setTimeOffset(timezoneOffset*3600 + DST*3600);
  timeClient.begin();
  DST0 = DST;
  Soare();
  }
hour = timeClient.getHours();
minute = timeClient.getMinutes();
second = timeClient.getSeconds();
ora = hour;
minut = minute;
secundar = second;
  
  // --------- display ring clock part ------------------------------------
    for (int i=0; i<NUM_LEDS; i++)  // clear all leds
      LEDs[i] = CRGB::Black;

    int inel_secunda = getLEDMinuteOrSecond(secundar);
    int inel_minut = getLEDMinuteOrSecond(minut);
    int inel_ora = getLEDHour(ora, minut);

for (int i=0; i<12; i++)  // show important points
      {
        LEDs[i*5] = fundal;  
        LEDs[i*5].fadeToBlackBy(25);
      }

    // Set "Hands"
    LEDs[inel_secunda] = colorSecond;
    LEDs[inel_minut] = colorMinute;  
    LEDs[inel_ora] = colorHour;  

    // Hour and min are on same spot
    if ( inel_ora == inel_minut)
      LEDs[inel_ora] = colorHourMinute;

    // Hour and sec are on same spot
    if ( inel_ora == inel_secunda)
      LEDs[inel_ora] = colorHourSecond;

    // Min and sec are on same spot
    if ( inel_minut == inel_secunda)
      LEDs[inel_minut] = colorMinuteSecond;

    // All are on same spot
    if ( inel_minut == inel_secunda && inel_minut == inel_ora)
      LEDs[inel_minut] = colorAll;

    if (night())
      {
        FastLED.setBrightness (NIGHTBRIGHTNESS);
        display.setBrightness(NBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      FastLED.setBrightness (DAYBRIGHTNESS);
       display.setBrightness(DBRIGHTNESS); 
      }
    FastLED.show();
// ---------end of display ring clock part ------------------------------------
}

byte getLEDHour(byte orele, byte minutele) {
  if (orele > 12)
    orele = orele - 12;

  byte hourLED;
  if (orele <= 5) 
    hourLED = (orele * 5) + 30;
  else
    hourLED = (orele * 5) - 30;

  if (USE_LED_MOVE_BETWEEN_HOURS == true) {
    if        (minutele >= 12 && minutele < 24) {
      hourLED += 1;
    } else if (minutele >= 24 && minutele < 36) {
      hourLED += 2;
    } else if (minutele >= 36 && minutele < 48) {
      hourLED += 3;
    } else if (minutele >= 48) {
      hourLED += 4;
    }
  }
  return hourLED;  
}

byte getLEDMinuteOrSecond(byte minuteOrSecond) {
  if (minuteOrSecond < 30) 
    return minuteOrSecond + 30;
  else 
    return minuteOrSecond - 30;
}

boolean night() { 
  ora1 = 100*hr1 + mn1;  // rasarit 
  ora2 = 100*hr2 + mn2;  // apus
  oraactuala = 100*ora + minut;  // ora actuala

  Serial.print(ora1);
  Serial.print(" ? ");
  Serial.print(oraactuala);
  Serial.print(" ? ");
  Serial.println(ora2);  

  if ((oraactuala > ora2) or (oraactuala < ora1))  // night 
    return true;  
    else
    return false;  
}
