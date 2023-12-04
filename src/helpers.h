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
#include <FS.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "version.h"
#include "OTAUpdate.h"
#include "wsData.h"

extern WiFiClient wifiClient;
extern PubSubClient mqttClient;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;

extern String hostname;
extern const char* mqtt_topic_lwt;
extern uint32_t countMsg;
extern wsData myData;
extern const char* wifi_ssid;
extern const char* wifi_pass;
extern OTAUpdater otaUpdater;
extern long mqttLastReconnectAttempt;

// http & websocket
extern AsyncWebSocket ws;
extern AsyncWebServer server;
extern uint8_t connectedClients;

// core
String getUniqueID();
void getState();
void reboot();
void checkWiFi();
void connectToWiFi();
// mqtt
void checkMqtt();
boolean connectToMqtt();
// http & websocket
String wsSerializeJson();
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();

#endif  // HELPERS_H
