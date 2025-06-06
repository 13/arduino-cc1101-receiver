#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <Updater.h>
#endif
#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include "time.h" // Built-in
#include "esp_system.h"
#include <Update.h>
#endif
#include <FS.h>
#define SPIFFS LittleFS
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>
#include "../../include/version.h"
#include <wsData.h>

#define INTERVAL_1MIN (1 * 60 * 1000L)

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern TimeChangeRule CEST;
extern TimeChangeRule CET;
extern Timezone Rome;

extern String hostname;
extern uint32_t countMsg;
extern unsigned long lastMillisMark;
extern wsData myData;
extern long mqttLastReconnectAttempt;

// http & websocket
extern AsyncWebSocket ws;
extern AsyncWebServer server;
extern uint8_t connectedClients;

// core
String getUniqueID();
void getState();
void reboot();
const char* boolToString(boolean value);
void turnOffLed();
void checkWiFi();
void connectToWiFi();
void initFS();
void initMDNS();
void printMARK();
// mqtt
void checkMqtt();
boolean connectToMqtt();
void subscribeMqtt();
// http & websocket
String wsSerializeJson();
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();

#endif  // HELPERS_H
