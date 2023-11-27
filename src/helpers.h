#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#endif
#if defined(ESP32)
#include <WiFi.h>
#endif
#include <PubSubClient.h>
#include "wsData.h"

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

extern String hostname;
extern const char* mqtt_topic_lwt;
extern uint32_t countMsg;
extern wsData myData;
extern const char* wifi_ssid;
extern const char* wifi_pass;

void reboot();
void getState();
void connectToWiFi();
boolean connectToMqtt();

#endif  // HELPERS_H
