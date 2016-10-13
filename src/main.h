#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "FastLED.h"
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h> // needed by ArduinoOTA

#define FASTLED_ESP8266_RAW_PIN_ORDER
#define NUM_LEDS 30
#define CHIPSET WS2812
#define DATA_PIN 5 // GPIO5


void setup_wifi();

void callback(char* topic, byte* payload, unsigned int length);

void reconnect();


void disableFx();
void runRainbowFx();
void runChaseFx();
void runFx();
