#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "../include/version.h"
#include <wsData.h>
#include <helpers.h>
#include "credentials.h"

#ifdef USE_CRYPTO
#include <Crypto.h>
#include <AES.h>
#endif

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
NTPClient timeClient(ntpUDP, NTP_SERVER, 0);
// DST and Standard time rules for Europe/Rome
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // UTC + 2
TimeChangeRule CET  = {"CET",  Last, Sun, Oct, 3, 60};  // UTC + 1
Timezone Rome(CEST, CET);

long mqttLastReconnectAttempt = 0;
int wsDataSize = 0;
uint8_t connectedClients = 0;

unsigned long previousMinute = 0;

#ifdef USE_CRYPTO
byte aeskey[16];
byte cipher[61];
byte decryptedText[61];
AES128 aes128;
boolean crypto = false;

void hexStringToByteArray(const char *hexString, byte *byteArray, size_t byteArrayLength)
{
  size_t hexStringLength = strlen(hexString);

  if (hexStringLength % 2 != 0 || hexStringLength / 2 != byteArrayLength)
  {
    Serial.print(F("CRYPTO: KEY INVALID"));
    // Invalid hex string length or mismatch with byte array length
    return;
  }
#ifdef DEBUG
  Serial.print(F("CRYPTO: KEY "));
#endif
  for (size_t i = 0; i < hexStringLength; i += 2)
  {
    // Convert each pair of hexadecimal characters to a byte
    sscanf(hexString + i, "%2hhx", &byteArray[i / 2]);
#ifdef DEBUG
    Serial.print(F("0x"));
    Serial.print(byteArray[i / 2], HEX);
    Serial.print(F(" "));
#endif
  }
#ifdef DEBUG
  Serial.println();
#endif
}
#endif

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
#ifdef USE_CRYPTO
  Serial.print(F("CRYPTO "));
#endif
#ifdef DEBUG
  Serial.print(F("DEBUG "));
#endif
#ifdef GD0
  Serial.print(F(" GD0"));
  Serial.print(GD0);
#endif
  Serial.println();
#endif
}

#ifdef MQTT_SUBSCRIBE
// Define constants for maximum values
const int MAX_PACKET_NODES = 32; // Adjust as needed
const int MAX_N_LENGTH = 8;

// Define struct to hold N and X pairs
struct Packet
{
  char N[MAX_N_LENGTH + 1]; // +1 for null terminator
  int X;
};

// Array to store pairs
Packet packets[MAX_PACKET_NODES];
int packetCount = 0;

bool packetExists(const char *N)
{
  for (int i = 0; i < packetCount; ++i)
  {
    if (strcmp(packets[i].N, N) == 0)
    {
      // Packet with N and X values exists
      return true;
    }
  }
  // Packet with N and X values does not exist
  return false;
}

bool packetExists(const char *N, int X)
{
  for (int i = 0; i < packetCount; ++i)
  {
    if (strcmp(packets[i].N, N) == 0 && packets[i].X == X)
    {
      // Packet with N and X values exists
      return true;
    }
  }
  // Packet with N and X values does not exist
  return false;
}

void printPackets()
{
  Serial.println("[Packets] ");
  for (int i = 0; i < packetCount; ++i)
  {
    Serial.print("N: ");
    Serial.print(packets[i].N);
    Serial.print(", X: ");
    Serial.println(packets[i].X);
  }
}

void onMqttMessage(char *topic, byte *payload, unsigned int len)
{
  // Parse the JSON data
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, len);
#ifdef DEBUG
  serializeJsonPretty(doc, Serial);
  Serial.println();
#endif

  if (doc.containsKey("N") && doc.containsKey("X") && packetCount < MAX_PACKETS)
  {
    String N = doc["N"];
    int X = doc["X"];

    // Check if packet with N and X values already exists
    if (packetExists(N.c_str()))
    {
      // Packet already exists, update X value
      for (int i = 0; i < packetCount; ++i)
      {
        if (strcmp(packets[i].N, N.c_str()) == 0)
        {
#ifdef DEBUG
          Serial.print(("> [PID] Update N:"));
          Serial.print(N.c_str());
          Serial.print(",X:");
          Serial.println(X);
#endif
          packets[i].X = X;
          break;
        }
      }
    }
    else
    {
      // Packet does not exist, add it to the array
      if (packetCount < MAX_PACKETS)
      {
#ifdef DEBUG
        Serial.print(("> [PID] Add N:"));
        Serial.print(N.c_str());
        Serial.print(",X:");
        Serial.println(X);
#endif
        // Add the packet to the array
        strncpy(packets[packetCount].N, N.c_str(), MAX_N_LENGTH);
        packets[packetCount].N[MAX_N_LENGTH] = '\0'; // Ensure null termination
        packets[packetCount].X = X;
        packetCount++;
      }
    }
  }
#ifdef DEBUG
  printPackets();
#endif
}
#endif

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

#ifdef USE_CRYPTO
      if (!(sizeof(byteArr) >= 4 && byteArr[0] == 'Z' && byteArr[1] == ':' && isdigit(byteArr[2]) && isdigit(byteArr[3])))
      {
        Serial.print(F("> [CRYPTO] Decrypting: "));
        // Decrypt the cipher
        aes128.decryptBlock(decryptedText, byteArr);

        int blockCount = byteArrLen / 16 + 1;
        for (int i = 0; i < blockCount; ++i)
        {
          aes128.decryptBlock(&decryptedText[i * 16], &byteArr[i * 16]);
        }

        for (int i = 0; i < sizeof(decryptedText); i++)
        {
          Serial.print((char)decryptedText[i]);
        }
        Serial.println();

        // Check if decryptedText meets the specified conditions
        if (sizeof(decryptedText) >= 4 && decryptedText[0] == 'Z' && decryptedText[1] == ':' && isdigit(decryptedText[2]) && isdigit(decryptedText[3]))
        {
          Serial.println(F("> [CRYPTO] Decryption successful."));
          // Extracting third and fourth characters and converting to integer
          byteArrLen = (decryptedText[2] - '0') * 10 + (decryptedText[3] - '0');
#ifdef DEBUG
          Serial.print(F("> [CRYPTO] Length: "));
          Serial.println(byteArrLen);
#endif
          crypto = true;
        }
      }
#endif

      String input_str = "";

      if (byteArrLen > 0 && byteArrLen <= byteArrSize)
      {
#ifdef USE_CRYPTO
        if (!crypto)
        {
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
              input_str += (char)byteArr[i];
            }
          }
#ifdef USE_CRYPTO
        }
        else
        {
          for (uint8_t i = 0; i < byteArrLen; i++)
          {
            // Filter [0-9A-Za-z,:]
            if ((decryptedText[i] >= '0' && decryptedText[i] <= '9') ||
                (decryptedText[i] >= 'A' && decryptedText[i] <= 'Z') ||
                (decryptedText[i] >= 'a' && decryptedText[i] <= 'z') ||
                decryptedText[i] == ',' || decryptedText[i] == ':' || decryptedText[i] == '-')
            {
              Serial.print((char)decryptedText[i]);
              input_str += (char)decryptedText[i];
            }
          }
          crypto = false;
        }
#endif

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
        ccJson["timestamp"] = Rome.toLocal(timeClient.getEpochTime());

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

          boolean publishMqtt = true;
          boolean published = true;
          boolean retained = !(ccJson.containsKey("R") && !ccJson["R"].isNull());

#ifdef MQTT_SUBSCRIBE
          if (ccJson.containsKey("X") && !ccJson["X"].isNull())
          {
            publishMqtt = !packetExists(ccJson["N"], ccJson["X"]);
          }
#endif

          if (publishMqtt)
          {
            published = mqttClient.publish(topic.c_str(), ccJsonStr.c_str(), retained);
          }
#ifdef MQTT_SUBSCRIBE
          else
          {
            Serial.print(("> [PID] Exists N:"));
            Serial.print(String(ccJson["N"].as<String>()));
            Serial.print(", X:");
            Serial.println(String(ccJson["X"].as<String>()));
            published = false;
          }
#endif
          // Check if published
          if (published)
          {
            Serial.print("> [MQTT] Message published");
            if (retained)
            {
              Serial.print(" retained");
            }
            Serial.println("");
          }
          else
          {
            Serial.println("> [MQTT] No publish message");
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
#ifdef USE_CRYPTO
  hexStringToByteArray(AES_KEY, aeskey, 16);
  aes128.setKey(aeskey, 16); // Setting Key for AES
#endif
  checkWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  if (WiFi.status() == WL_CONNECTED)
  {
    initMDNS();
    connectToMqtt();
#ifdef MQTT_SUBSCRIBE
    mqttClient.setCallback(onMqttMessage);
#endif
    timeClient.begin();
    timeClient.update();
    // myData.boottime = Rome.toLocal(timeClient.getEpochTime());
    myData.boottime = timeClient.getEpochTime();
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
