#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#define SPIFFS LittleFS
#include <LittleFS.h>
#else
#include <EEPROM.h>
#endif
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "credentials.h"

// Edit credentials.h

#if defined(ESP8266)
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
long mqttLastReconnectAttempt = 0;

String getIP()
{
  return String(WiFi.localIP().toString());
}

void notifyClients()
{
  ws.textAll(getIP());
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      // ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
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

// Replaces placeholder with LED state value
String processor(const String &var)
{
  Serial.println(var);
  if (var == "IP")
  {
    return getIP();
  }
  return String();
}

void connectToWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.print("> [WiFi] Connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" OK");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("> [WiFi] IP: ");
    Serial.println(WiFi.localIP().toString());
  }
}
boolean connectToMqtt(String uid)
{
  String clientId = "esp8266-";
  clientId += uid;
  String lastWillTopic = "esp/";
  lastWillTopic += clientId;
  lastWillTopic += "/LWT";
  if (!mqttClient.connected())
  {
    Serial.print("> [MQTT] Connecting...");
    if (mqttClient.connect(clientId.c_str(), lastWillTopic.c_str(), 1, true, "offline"))
    {
      Serial.println(" OK");
      mqttClient.publish(lastWillTopic.c_str(), "online", true);
    }
    else
    {
      Serial.print(" failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
  else
  {
    mqttClient.publish(lastWillTopic.c_str(), "online", true);
  }
  return mqttClient.connected();
}
#endif

// cc1101
const uint8_t byteArrSize = 61;

// Last 4 digits of ChipID
String getUniqueID()
{
  String uid = "0";
#if defined(ESP8266)
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

#ifdef VERBOSE
// one minute mark
#define MARK
#define INTERVAL_1MIN (1 * 60 * 1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 0;
#endif

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
#if defined(ESP8266)
    // 1 minute status update
    connectToMqtt(getUniqueID());
#endif
  }
}
#endif

void setup()
{
  Serial.begin(9600);
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
#ifdef VERBOSE
  Serial.print(("> Mode: "));
#ifdef GD0
  Serial.print(F("GD0 "));
#endif
  Serial.print(F("VERBOSE "));
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
  Serial.println();
#endif
#if defined(ESP8266)
  // Initialize LittleFS
  if (!LittleFS.begin())
  {
    Serial.println(F("> [LittleFS] ERROR "));
    return;
  }
  connectToWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  if (WiFi.status() == WL_CONNECTED)
  {
    connectToMqtt(getUniqueID());
  }
#endif
  // Start CC1101
  Serial.print(F("> [CC1101] Initializing... "));
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
    Serial.println(F("OK"));
    ELECHOUSE_cc1101.Init(); // must be set to initialize the cc1101!
#ifdef GD0
    ELECHOUSE_cc1101.setGDO0(GD0); // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
#endif
    ELECHOUSE_cc1101.setCCMode(1);     // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);  // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    // ELECHOUSE_cc1101.setPA(CC_POWER);  // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setSyncMode(2); // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setCrc(1);      // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    ELECHOUSE_cc1101.setCRC_AF(1);   // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    // ELECHOUSE_cc1101.setAdrChk(1);   // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    // ELECHOUSE_cc1101.setAddr(0);     // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_state);
    while (true)
      ;
  }
#if defined(ESP8266)
  initWebSocket();
  // Route for root / web page
  //server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //          { request->send_P(200, "text/html", index_html, processor); });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, processor); });
  
  //server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //          { request->send(LittleFS, "/index.html", String(), false, processor); });
  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/bootstrap.min.css", "text/css"); });
  server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/bootstrap.bundle.min.js", "text/javascript"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });
  server.on("/IP", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getIP().c_str()); });

  // Start server
  server.begin();
#endif
}

void loop()
{
#if defined(ESP8266)
  ws.cleanupClients();
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  if (!mqttClient.connected())
  {
    long mqttNow = millis();
    if (mqttNow - mqttLastReconnectAttempt > 5000)
    {
      mqttLastReconnectAttempt = mqttNow;
      // Attempt to reconnect
      if (connectToMqtt(getUniqueID()))
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
      byteArr[byteArrLen] = '\0'; // 0, \0
#ifdef VERBOSE
      Serial.println(F("OK"));
      Serial.print(F("> [CC1101] Length: "));
      Serial.println(byteArrLen);
#endif
#if defined(ESP8266)
      String input_str = "";
#endif
      for (uint8_t i = 0; i < byteArrLen; i++)
      {
        // Filter [0-9A-Za-z,:]
        if ((byteArr[i] >= '0' && byteArr[i] <= '9') ||
            (byteArr[i] >= 'A' && byteArr[i] <= 'Z') ||
            (byteArr[i] >= 'a' && byteArr[i] <= 'z') ||
            byteArr[i] == ',' || byteArr[i] == ':' || byteArr[i] == '-')
        {
          Serial.print((char)byteArr[i]);
#if defined(ESP8266)
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
#if defined(ESP8266)
      input_str += ",RSSI:";
      input_str += String(rssi);
      input_str += ",LQI:";
      input_str += String(lqi);
      input_str += ",RN:";
      input_str += getUniqueID();
      // Serial.println(input_str);

      // Create a DynamicJsonDocument object
      DynamicJsonDocument doc(1024);

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
            doc[key] = value_str;
          }
          else if ((key.startsWith("T") || key.startsWith("H") || key.startsWith("P")) || (key.startsWith("V") && value_str.length() > 1))
          {
            float value = value_str.toFloat() / 10.0;
            doc[key] = round(value * 10.0) / 10.0;
          }
          else
          {
            int value = value_str.toInt();
            doc[key] = value;
          }
        }
        pos = sep_pos + 1;
      }
      doc.remove("Z");
      // Print the JSON object to the serial monitor
      // serializeJsonPretty(doc, Serial);
      String jsonStr;
      serializeJson(doc, jsonStr);
      Serial.println(jsonStr);
      String topic = String(mqtt_topic) + "/" + String(doc["N"].as<String>()) + "/json";
      if (!mqttClient.connected())
      {
        Serial.println("> [MQTT] Not connected");
        connectToMqtt(getUniqueID());
      }
      bool published = mqttClient.publish(topic.c_str(), jsonStr.c_str(), true);
      if (published)
      {
        Serial.println("> [MQTT] Message published");
      }
      else
      {
        Serial.println("> [MQTT] Failed to publish message");
      }
#endif
#ifdef VERBOSE_FW
      Serial.print(F(",RF:"));
      Serial.println(String(GIT_VERSION_SHORT));
#endif
    }
#ifdef DEBUG
    else
    {
#ifdef DEBUG_CRC
      for (uint8_t i = 0; i < byteArrSize; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(ELECHOUSE_cc1101.getRssi());
      Serial.print(F(",LQI:"));
      Serial.print(ELECHOUSE_cc1101.getLqi());
      Serial.print(F(",RN:"));
      Serial.println(getUniqueID());
      Serial.print(F(",RF:"));
      Serial.println(String(GIT_VERSION_SHORT));
#endif
    }
#endif
  }
}
