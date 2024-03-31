#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "../include/version.h"
#include <wsData.h>
#include <helpers.h>
#include "credentials.h"

// cc1101
const uint8_t byteArrSize = 61;

// ESP
#if defined(ESP8266)
String hostname = "esp8266-";
#endif
#if defined(ESP32)
String hostname = "esp32-";
#endif
// WiFi & MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
wsData myData;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

long mqttLastReconnectAttempt = 0;
int wsDataSize = 0;
uint8_t connectedClients = 0;

unsigned long previousMinute = 0;

// supplementary functions
#ifdef VERBOSE
// one minute mark
#define MARK
unsigned long lastMillisMark = 0L;
uint32_t countMsg = 0;
#endif

void initSerial()
{
  Serial.begin(115200);
  delay(10);
}

void printBootMsg()
{
#ifdef DEBUG
  delay(5000);
#endif
  // Start Boot
  delay(1000);
  Serial.println(F("> "));
  Serial.println(F("> "));
  Serial.print(F("> Booting... Compiled: "));
  Serial.println(VERSION);
#if defined(ESP8266) || defined(ESP32)
  Serial.print(F("> Node ID: "));
  Serial.println(getUniqueID());
  hostname += getUniqueID();
#endif
#ifdef VERBOSE
  Serial.print(("> Mode: "));
  Serial.print(F("VERBOSE "));
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
#ifdef GD0
  Serial.print(F("GD0"));
  Serial.print(GD0);
#endif
  Serial.println();
#endif
}

void initCC1101()
{
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
    reboot();
  }
}

void processCC1101Data()
{
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
      String input_str = "";
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
            input_str += (char)byteArr[i];
          }
        }
        Serial.print(F(",RSSI:"));
        Serial.print(rssi);
        Serial.print(F(",LQI:"));
        Serial.print(lqi);
        Serial.print(F(",RN:"));
        Serial.println(getUniqueID());

        input_str += ",RSSI:";
        input_str += String(rssi);
        input_str += ",LQI:";
        input_str += String(lqi);
        input_str += ",RN:";
        input_str += getUniqueID();
        input_str += ",PID:";
        input_str += String(getPID(int(lqi-rssi)), HEX);
        // Serial.println(input_str);

        StaticJsonDocument<384> ccJson;
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
            if (key.startsWith("N") || key.startsWith("RN") || key.startsWith("F") || key.startsWith("RF") || key.startsWith("PID"))
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
        ccJson["timestamp"] = timeClient.getEpochTime();

        String ccJsonStr;
        serializeJson(ccJson, ccJsonStr);
        Serial.print("> [JSON] ");
        Serial.println(ccJsonStr);

        if (ccJson.containsKey("N") && !ccJson["N"].isNull())
        {
          String topic = String(MQTT_TOPIC) + "/" + String(ccJson["N"].as<String>()) + "/json";
          if (!mqttClient.connected())
          {
            Serial.println("> [MQTT] Not connected");
            connectToMqtt();
          }
          boolean retained = !(ccJson.containsKey("R") && !ccJson["R"].isNull());
          boolean published = mqttClient.publish(topic.c_str(), ccJsonStr.c_str(), retained);
          // Check if published
          if (published)
          {
            Serial.print("> [MQTT] Message published");
            if (retained) {
              Serial.print(" retained");
            }
            Serial.println("");
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

void setup()
{
  initSerial();
  printBootMsg();
  initFS();
  checkWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
#ifdef MQTT_SUBSCRIBE
  mqttClient.setCallback(onMqttMessage);
#endif
  if (WiFi.status() == WL_CONNECTED)
  {
    connectToMqtt();
    timeClient.begin();
    timeClient.update();
    myData.boottime = timeClient.getEpochTime();
    initMDNS();
  }
  // Init CC1101
  initCC1101();
  // Initalize websocket
  initWebSocket();
}

void loop()
{
  ws.cleanupClients();
  checkMqtt();
#ifdef MARK
  printMARK();
#endif

  // CC1101
  processCC1101Data();
}
