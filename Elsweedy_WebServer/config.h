/*
   created a delay of 50ms between two counts, Counts outside the interrupt
   Problem if code stuck it wont count
   Sol: make array of readings and compare their millis
   waiting for Reset interrupt making
*/

#ifdef ESP8266

#define chipID  ESP.getChipId()

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#endif

#ifdef ESP32

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

String chipID = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX) + String((uint32_t)ESP.getEfuseMac(), HEX);

#endif


#include <WiFiClient.h>
#include "EEPROM.h"
#include "private.h"


String GOOGLE_SCRIPT_ID = "AKfycbz7XcVfZQgc8TxbtES0GVVWpOq4TJX7MsuAgVyPu3tOtFQkM9yyU4SJaxDuslaLYvgq";

#define EEPROM_SIZE 200             // EEPROM size

/*********** Pins *****************/
#define LED 2
#define SensorPin 15
#define ResetButtonPin 21
//#define AccessPointPin 18
#define AccessPointPin 9
#define ResetOutSignal 5
/*************************************/
struct Button {
  const uint8_t PIN;
  bool pressed;
};

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
