/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/tca9548a-i2c-multiplexer-esp32-esp8266-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  v.0 - original version from Rui's site
  v.1 - original sketch by Nicu FLORICA (niq_ro)
      - NTP clock inspired by https://nicuflorica.blogspot.com/2022/10/ceas-ntp-pe-afisaj-cu-tm1637.html
      - hardware switch for DST (Daylight Saving Time) as at https://nicuflorica.blogspot.com/2022/12/ceas-ntp-pe-afisaj-oled-de-096-128x64.html
  v.2 - added 7-segment numbers, based in ideea from https://ta-laboratories.com/blog/2018/09/07/recreating-a-7-segment-display-with-adafruit-gfx-ssd1306-oled/
  v.2a - small correction in sketch (clean unusefull lines)
  ver.3 - display info just at changes
  ver.3a - autoreset after 1 minutes
  ver.4 - added 60 leds ring as in https://github.com/leonvandenbeukel/Round-LED-Clock
  ver.4a - added on ring 12 white points (for easy identification of the time) + day/night brightness 
  ver.4b - added automatic brightness due to sunrise/sunset using https://github.com/jpb10/SolarCalculator library 
  ver.5 - removed OLED displays.. remain just 60leds ring
  ver.1 - reset counter +  added HT16K33 4-digit 14-segment display
*********/

#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator
#include <Wire.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>
#include <Wire.h>
#include "NoiascaHt16k33.h"                                // include the noiasca HT16K33 library - download from http://werner.rothschopf.net/

Noiasca_ht16k33_hw_14 display = Noiasca_ht16k33_hw_14();   // 14 segment - Present time

const char *ssid     = "niq_ro";
const char *password = "berelaburtica";
const long timezoneOffset = 2; // GMT + seconds  (Romania)
 // Location - Craiova: 44.317452,23.811336
double latitude = 44.31;
double longitude = 23.81;

#define DSTpin   14 // GPIO14 = D5, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", timezoneOffset*3600);

int ora, minut, secunda;
byte DST = 0;
byte DST0;

byte oz1, ou1, mz1, mu1;
byte oz0 = 10;
byte ou0 = 10;
byte mz0 = 10;
byte mu0 = 10;
byte secunda0 = 60;

unsigned long tpchange;
unsigned long tpscurs = 60000;

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

// Set this to true if you want the hour LED to move between hours (if set to false the hour LED will only move every hour)
#define USE_LED_MOVE_BETWEEN_HOURS true

// WS2812b day / night brightness.
#define NIGHTBRIGHTNESS 4      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 20

// HT16K33 day / night brightness.
#define NBRIGHTNESS 2      // Brightness level from 0 (off) to 15 (full brightness)
#define DBRIGHTNESS 10

#define NUM_LEDS 60     
#define DATA_PIN D6
CRGB LEDs[NUM_LEDS];

byte inel_ora;
byte inel_minut;
byte inel_secunda;
byte secundar;

unsigned long epochTime;
byte tensHour, unitsHour, tensMin, unitsMin, tensSec, unitsSec;

double transit, sunrise, sunset;
char str[6];
int ora1, ora2, oraactuala;
int m1, hr1, mn1, m2, hr2, mn2; 
//int ora, minut, secunda, rest;
int ora0, minut0;
byte ziua, luna, an;

void setup() {
  pinMode (DSTpin, INPUT);

  Wire.begin();                                  // start the I2C interface
  Wire.setClock(400000);                         // optional: activate I2C fast mode. If it is to fast for other I2C devices. deactivate this row.
  display.begin(0x70, 1);                        // I2C adress of the first display
  display.setBrightness(NBRIGHTNESS);                      // set brightness from 0 to 15
  display.clear();

 //FastLED.delay(3000);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  

  Serial.begin(115200);
  Serial.println (" ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
    Serial.println (" ");
    
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset(timezoneOffset*3600 + DST*3600);
  timeClient.begin();
  DST0 = DST;

  display.setBrightness(DBRIGHTNESS);
      display.print(12);
      display.print(F("."));
      display.print(30);

  FastLED.setBrightness(DAYBRIGHTNESS); //Number 0-255
    LEDs[32] = colorHour;   // 2 oçlock
    LEDs[37] = colorMinute; // 7 minutes
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

iaData();
Soare();
}
 
void loop() {
 if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  
timeClient.update();
 ora = timeClient.getHours();
 minut = timeClient.getMinutes();
 secundar = timeClient.getSeconds();
/*
 ora = ora%12;  // 12-hour format
 if (ora == 0) ora = 12;  // 12-hour format
*/

// display.clear();
      if (ora < 10) display.print(F("0"));
      display.print(ora);
      if (millis()/1000%2 == 0)
      display.print(F("."));
      if (minut < 10) display.print(F("0"));
      display.print(minut);
 // second_prev = secondar;

 // int diff = millis() - t;
 // delay(diff >= 0 ? (500 - (millis() - t)) : 0);

oz1 = ora/10;
ou1 = ora%10;
mz1 = minut/10;
mu1 = minut%10;

Serial.println("-----------");
Serial.print(ora);
Serial.print(":");
Serial.println(minut);

if (DST != DST0)
{
  timeClient.setTimeOffset(timezoneOffset*3600 + DST*3600);
  timeClient.begin();
DST0 = DST;
}

oz0 = oz1;
ou0 = ou1;
mz0 = mz1;
mu0 = mu1;
secunda0 = secunda;

secunda = secundar;
delay(250);


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
  
 iaData();
Soare();
} // end main loop

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

void iaData()
{
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  ziua = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(ziua);

  luna = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(luna);


  an = ptm->tm_year+1900-2000;
  Serial.print("Year: ");
  Serial.println(an);
}

void Soare()
{
   // Calculate the times of sunrise, transit, and sunset, in hours (UTC)
  calcSunriseSunset(2000+an, luna, ziua, latitude, longitude, transit, sunrise, sunset);

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
