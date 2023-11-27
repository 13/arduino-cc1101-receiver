#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#endif
#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#if defined(ESP8266) || defined(ESP32)
#include <Updater.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "wsData.h"
#include "helpers.h"
#define SPIFFS LittleFS
#include <LittleFS.h>
#else
#include <EEPROM.h>
#endif
#include "credentials.h"

// Edit credentials.h

#ifdef VERBOSE
// one minute mark
#define MARK
#define INTERVAL_1MIN (1 * 60 * 1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 0;
#endif

#if defined(ESP8266)
String hostname = "esp8266-";
#endif
#if defined(ESP32)
String hostname = "esp32-";
#endif
#if defined(ESP8266) || defined(ESP32)
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
wsData myData;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

long mqttLastReconnectAttempt = 0;

int wsDataSize = 0;
int connectedClients = 0;

// Websocket
String wsSerializeJson()
{
  myData.uptime = countMsg;
  myData.rssi = WiFi.RSSI();
  myData.memfree = ESP.getFreeHeap();
  myData.memfrag = ESP.getHeapFragmentation();
  String jsonStr = myData.toJson();
  Serial.print("> [WS] ");
  Serial.println(jsonStr);
  return jsonStr;
}

void notifyClients()
{
  if (connectedClients > 0)
  {
    ws.textAll(wsSerializeJson());
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("> [WebSocket] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    connectedClients++;
    getState();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("> [WebSocket] Client #%u disconnected\n", client->id());
    connectedClients--;
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
#endif

// cc1101
const uint8_t byteArrSize = 61;

// Last 4 digits of ChipID
String getUniqueID()
{
  String uid = "0";
#if defined(ESP8266) || defined(ESP32)
  uid = WiFi.macAddress().substring(12);
  uid.replace(":", "");
#else
  // read EEPROM serial number
  int address = 13;
  int serialNumber;
  if (EEPROM.read(address) != 255)
  {
    EEPROM.get(address, serialNumber);
    uid = String(serialNumber, HEX);
  }
#endif
  return uid;
}

// supplementary functions
#ifdef MARK
void printMARK()
{
  if (countMsg == 0)
  {
    Serial.println(F("> [MARK] Starting... OK"));
    countMsg++;
  }
  if (countMsg == UINT32_MAX)
  {
    countMsg = 1;
  }
  if (millis() - lastMillis >= INTERVAL_1MIN)
  {
    Serial.print(F("> [MARK] Uptime: "));

    if (countMsg >= 60)
    {
      int hours = countMsg / 60;
      int remMins = countMsg % 60;
      if (hours >= 24)
      {
        int days = hours / 24;
        hours = hours % 24;
        Serial.print(days);
        Serial.print(F("d "));
      }
      Serial.print(hours);
      Serial.print(F("h "));
      Serial.print(remMins);
      Serial.println(F("m"));
    }
    else
    {
      Serial.print(countMsg);
      Serial.println(F("m"));
    }
    countMsg++;
    lastMillis += INTERVAL_1MIN;
#if defined(ESP8266) || defined(ESP32)
    // 1 minute status update
    connectToMqtt();
    notifyClients();
#endif
  }
}
#endif

void setup()
{
  Serial.begin(115200);
  delay(10);
#ifdef VERBOSE
  delay(5000);
#endif
  // Start Boot
  Serial.println(F("> "));
  Serial.println(F("> "));
  Serial.print(F("> Booting... Compiled: "));
  Serial.println(GIT_VERSION);
  Serial.print(F("> Node ID: "));
  Serial.println(getUniqueID());
#if defined(ESP8266) || defined(ESP32)
  hostname += getUniqueID();
#endif
#ifdef VERBOSE
  Serial.print(("> Mode: "));
  Serial.print(F("VERBOSE "));
#ifdef DEBUG
  Serial.print(F("DEBUG "));
#endif
#ifdef GD0
  Serial.print(F("GD0"));
  Serial.print(GD0);
#endif
  Serial.println();
#endif
#if defined(ESP8266) || defined(ESP32)
  // Initialize LittleFS
  if (!LittleFS.begin())
  {
    Serial.println(F("> [LittleFS] ERROR "));
    reboot();
  }
  connectToWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  if (WiFi.status() == WL_CONNECTED)
  {
    connectToMqtt();
  }
  // Initialize mDNS & OTA
  if (!MDNS.begin(hostname))
  {
    Serial.println(F("> [mDNS] ERROR"));
  }
  MDNS.addService("http", "tcp", 80);
#endif
  // Start CC1101
  Serial.print(F("> [CC1101] Initializing... "));
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
    Serial.println(F("OK"));
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setModulation(0);
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);
    // ELECHOUSE_cc1101.setPA(CC_POWER);
    ELECHOUSE_cc1101.setSyncMode(2);
    ELECHOUSE_cc1101.setCrc(1);
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_state);
#if defined(ESP8266) || defined(ESP32)
    reboot();
#else
    while (true)
      ;
#endif
  }
#if defined(ESP8266) || defined(ESP32)
  initWebSocket();
  // Route web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/bootstrap.min.css", "text/css"); });
  server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/bootstrap.bundle.min.js", "text/javascript"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", myData.ip.c_str()); });
  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", wsSerializeJson()); });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", "{\"status\":\"rebooting\"}");
              reboot(); });
  server.on("/update.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/update.html", "text/html"); });
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "File uploaded successfully");
        request->send(response); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
          Serial.print(F("> [OTA] Updating ... "));
          Serial.println(filename);
          Update.runAsync(true);
          uint32_t free_space;
          int cmd;

          if (filename.indexOf("littlefs") > -1)
          {
            FSInfo fs_info;
            LittleFS.info(fs_info);
            free_space = fs_info.totalBytes;
            cmd = U_FS;
          }
          else
          {
            free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            cmd = U_FLASH;
          }

          if (!Update.begin(free_space, cmd))
          {
            Update.printError(Serial);
          }
        }

        if (Update.write(data, len) != len)
        {
          Update.printError(Serial);
        }

        if (final)
        {
          if (!Update.end(true))
          {
            Update.printError(Serial);
          }
          else
          {
            Serial.println(F("> [OTA] Successful"));
            reboot();
          }
        }
      });

  // Start server
  server.begin();
#endif
}

void loop()
{
#if defined(ESP8266) || defined(ESP32)
  MDNS.update();
  ws.cleanupClients();
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  if (!mqttClient.connected())
  {
    Serial.println("> [MQTT] Not connected loop");
    long mqttNow = millis();
    if (mqttNow - mqttLastReconnectAttempt > 5000)
    {
      mqttLastReconnectAttempt = mqttNow;
      // Attempt to reconnect
      if (connectToMqtt())
      {
        mqttLastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    mqttClient.loop();
  }
#endif
#ifdef MARK
  printMARK();
#endif

#ifdef GD0
  if (ELECHOUSE_cc1101.CheckReceiveFlag())
#else
  if (ELECHOUSE_cc1101.CheckRxFifo(CC_DELAY))
#endif
  {
    byte byteArr[byteArrSize] = {0};
#ifdef DEBUG
    Serial.print(F("> [CC1101] Receive... "));
#endif
    if (ELECHOUSE_cc1101.CheckCRC())
    {
#ifdef VERBOSE
#ifndef DEBUG
      Serial.print(F("> [CC1101] Receive... "));
#endif
#endif
#ifdef VERBOSE
      Serial.print(F("CRC "));
#endif
      int byteArrLen = ELECHOUSE_cc1101.ReceiveData(byteArr);
      int rssi = ELECHOUSE_cc1101.getRssi();
      int lqi = ELECHOUSE_cc1101.getLqi();
      if (byteArrLen > 0 && byteArrLen <= byteArrSize)
      {
#ifdef VERBOSE
        Serial.println(F("OK"));
#endif
        byteArr[byteArrLen] = '\0'; // 0, \0
      }
      else
      {
#ifdef VERBOSE
        Serial.println(F("ERR"));
#endif
      }
#ifdef VERBOSE
      Serial.print(F("> [CC1101] Length: "));
      Serial.println(byteArrLen);
#endif
#if defined(ESP8266) || defined(ESP32)
      String input_str = "";
#endif
      if (byteArrLen > 0 && byteArrLen <= byteArrSize)
      {
        for (uint8_t i = 0; i < byteArrLen; i++)
        {
          // Filter [0-9A-Za-z,:]
          if ((byteArr[i] >= '0' && byteArr[i] <= '9') ||
              (byteArr[i] >= 'A' && byteArr[i] <= 'Z') ||
              (byteArr[i] >= 'a' && byteArr[i] <= 'z') ||
              byteArr[i] == ',' || byteArr[i] == ':' || byteArr[i] == '-')
          {
            Serial.print((char)byteArr[i]);
#if defined(ESP8266) || defined(ESP32)
            input_str += (char)byteArr[i];
#endif
          }
        }
        Serial.print(F(",RSSI:"));
        Serial.print(rssi);
        Serial.print(F(",LQI:"));
        Serial.print(lqi);
        Serial.print(F(",RN:"));
        Serial.println(getUniqueID());
#if defined(ESP8266) || defined(ESP32)
        input_str += ",RSSI:";
        input_str += String(rssi);
        input_str += ",LQI:";
        input_str += String(lqi);
        input_str += ",RN:";
        input_str += getUniqueID();
        // Serial.println(input_str);

        StaticJsonDocument<256> ccJson;
        // Split the input string into key-value pairs using comma separator
        uint8_t pos = 0;
        while (pos < input_str.length())
        {
          int sep_pos = input_str.indexOf(',', pos);
          if (sep_pos == -1)
          {
            sep_pos = input_str.length();
          }
          String pair = input_str.substring(pos, sep_pos);
          int colon_pos = pair.indexOf(':');
          if (colon_pos != -1)
          {
            String key = pair.substring(0, colon_pos);
            String value_str = pair.substring(colon_pos + 1);
            if (key.startsWith("N") || key.startsWith("RN") || key.startsWith("F") || key.startsWith("RF"))
            {
              ccJson[key] = value_str;
            }
            else if ((key.startsWith("T") || key.startsWith("H") || key.startsWith("P")) || (key.startsWith("V") && value_str.length() > 1))
            {
              float value = value_str.toFloat() / 10.0;
              ccJson[key] = round(value * 10.0) / 10.0;
            }
            else
            {
              int value = value_str.toInt();
              ccJson[key] = value;
            }
          }
          pos = sep_pos + 1;
        }
        ccJson.remove("Z");

        String ccJsonStr;
        serializeJson(ccJson, ccJsonStr);
        Serial.print("> [JSON] ");
        Serial.println(ccJsonStr);

        if (ccJson.containsKey("N") && !ccJson["N"].isNull())
        {
          String topic = String(mqtt_topic) + "/" + String(ccJson["N"].as<String>()) + "/json";
          if (!mqttClient.connected())
          {
            Serial.println("> [MQTT] Not connected");
            connectToMqtt();
          }
          bool published = mqttClient.publish(topic.c_str(), ccJsonStr.c_str(), true);
          if (published)
          {
            Serial.println("> [MQTT] Message published");
          }
          else
          {
            Serial.println("> [MQTT] Failed to publish message");
          }
        }

        // websocket
#ifdef DEBUG
        wsDataSize = ccJson.size();
        Serial.print("> [WS] ccJson size: ");
        Serial.println(wsDataSize);
#endif
#ifdef WSPACKETS
        if (!ccJson.isNull() && ccJson.containsKey("N"))
        {
          myData.addPacket(ccJsonStr);
        }
#else
        if (!ccJson.isNull() && ccJson.containsKey("N"))
        {
          myData.addPacket(ccJsonStr);
        }
#endif
        // ccJsonStr = "";
        notifyClients();
#endif
      } // length 0
#ifdef DEBUG_CRC
      else
      {
        Serial.println(F("> [CC1101] Err: Size 0 "));
        for (uint8_t i = 0; i < byteArrSize; i++)
        {
          Serial.print((char)byteArr[i]);
        }
        Serial.print(F(",RSSI:"));
        Serial.print(rssi);
        Serial.print(F(",LQI:"));
        Serial.print(lqi);
        Serial.print(F(",RN:"));
        Serial.println(getUniqueID());
      }
#endif
    } // check crc
#ifdef DEBUG_CRC
    else
    {
      Serial.print(F("CRC "));
      int byteArrLen = ELECHOUSE_cc1101.ReceiveData(byteArr);
      int rssi = ELECHOUSE_cc1101.getRssi();
      int lqi = ELECHOUSE_cc1101.getLqi();
      byteArr[byteArrLen] = '\0'; // 0, \0
      Serial.println(F("ERR"));

      for (uint8_t i = 0; i < byteArrSize; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(rssi);
      Serial.print(F(",LQI:"));
      Serial.println(lqi);
    }
#endif
  }
}
