/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RtcDS3231.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp8266.h>
#include "configWifi.h"

char blynkToken[] = "MW5Dk5Cy1pLlM-u4xBI1wSJtFFHmbZZT";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

int displayNum = 0;

int offset1 = -1;
int offset2 = -3;

unsigned long lastTime = millis();
unsigned long lastNistUpdate = 0;
unsigned long nistUpdate = 900000; // 15 minutes

bool showColon = true;

// wifi networks
const char *homeWifi = configHomeWifi;
const char *homePassword = configHomePassword;
const char *officeWifi = configOfficeWifi;
const char *officePassword = configOfficePassword;
const char *remoteWifi = configRemoteWifi;
const char *remotePassword = configRemotePassword;

const char *timeserver = "time.nist.gov";

// UTC offset in seconds
const long utcOffset = -14400;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, timeserver, utcOffset);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

RtcDS3231<TwoWire> rtc(Wire);

void setup() {
    // // kill blue LED
    // pinMode(LED_BUILTIN, OUTPUT);
    // digitalWrite(LED_BUILTIN, HIGH);
    
    Serial.begin(115200);
    while (!Serial); // wait for serial device to attach
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == homeWifi ) {
            WiFi.begin(homeWifi, homePassword);
            break;
        }
        if (WiFi.SSID(i) == officeWifi) {
            WiFi.begin(officeWifi, officePassword);
            break;
        }
        if (WiFi.SSID(i) == remoteWifi) {
            WiFi.begin(remoteWifi, remotePassword);
            break;
        }
    }
    
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    setupDisplayAndClock(4, 5);
    
    setupDisplayAndClock(12, 14);
    
    setupDisplayAndClock(2, 0);
    
    timeClient.begin();
}

void setupDisplayAndClock(int sda, int scl) {
    // RTC is only on one set of pins
    Wire.begin(4, 5);
    
    rtc.Begin();
    
    RtcDateTime currentTime = RtcDateTime(2000, 1, 1, 12, 0, 0);
    rtc.SetDateTime(currentTime);
    
    // displays are on different pins
    Wire.begin(sda, scl);
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
}

int getHour(int displayNum, int hour) {
    // displays: 0, 1, 2
    if (displayNum > 0) {
        if (displayNum == 1) {
            hour = hour + offset1;
        }
        else {
            hour = hour + offset2;
        }
    }
    
    // 12 hour display
    if (hour > 12) {
        hour = hour - 12;
    }
    else if (hour < 0) {
        hour = 12 - hour;
    }
    return hour;
}

void rtcUpdate() {
    timeClient.update();
    RtcDateTime nistTime = RtcDateTime(2020, 1, 1, 
        timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
    rtc.SetDateTime(nistTime);
}

void loop() {
    if (lastNistUpdate == 0) {
        rtcUpdate();
    }
    
    // update RTC from nist at the nist update interval
    if (millis() - lastNistUpdate > nistUpdate) {
        rtcUpdate();
        lastNistUpdate = millis();
    }
    
    // RTC is only on one set of pins
    Wire.begin(4, 5);
    RtcDateTime currentTime = rtc.GetDateTime();
    
    // displays are on different pins
    if (displayNum == 0)
    {
        Wire.begin(4, 5); // D1 = GPIO5, D2 = GPIO4
    }
    else if (displayNum == 1)
    {
        Wire.begin(12, 14);
    }
    else
    {
        Wire.begin(2, 0); // D3 = GPIO0, D4 = GPIO2
    }
    
    display.clearDisplay();
    display.setTextSize(4);
    display.setTextColor(WHITE);
    display.setCursor(3, 0);
    
    // display times
    if (getHour(displayNum, currentTime.Hour()) < 10) {
        display.print(" ");
    }
    display.print(getHour(displayNum, currentTime.Hour()));
    
    if (millis() - lastTime > 500) {
        showColon = !showColon;
        lastTime = millis();
    }
    if (showColon) {
        display.print(":");
    }
    else {
        display.print(" ");
    }
    
    if (currentTime.Minute() < 10) {
        display.print("0");
    }
    display.print(currentTime.Minute());
    
    // display location names
    display.setTextSize(1);
    display.setCursor(7, 40);
    
    if (displayNum == 0)
    {
        display.print("Toronto ");
        displayNum = 1;
    }
    else if (displayNum == 1)
    {
        display.print("Minneapolis");
        displayNum = 2;
    }
    else
    {
        display.print("Portland");
        displayNum = 0;
    }
    
    display.display();
    
    if (displayNum == 0)
    {
        delay(250);
    }
}
