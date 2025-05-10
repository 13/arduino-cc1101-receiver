#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "../include/version.h"
#include "wsData.h"
#include "helpers.h"
#include "credentials.h"

#ifdef USE_CRYPTO
#include <Crypto.h>
#include <AES.h>
#endif

// SPI
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define MISO 7 // Master In Slave Out
#define MOSI 8 // Master Out Slave In
#define SCK 9  // Clock
#define SS 10  // Chip Select
#define RST 11 // Reset
#define DIO0 2 // DIO0
#else
#define MISO 6 // Master In Slave Out
#define MOSI 7 // Master Out Slave In
#define SCK 8  // Clock
#define SS 9   // Chip Select
#define RST 10 // Reset
#define DIO0 2 // DIO0
#endif

// LoRa
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
  if (getUniqueID() == "0000")
  {
    connectToWiFi();
  }
  Serial.print(F("> NodeID: "));
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

void processLoRaData(int packetSize)
{
  if (packetSize == 0)
  {
    Serial.println(F("> [LoRa] ERR: empty packet"));
    return; // if there's no packet, return
  }
  // received a packet
  Serial.println(F("> [LoRa] Receive... "));

  byte byteArr[byteArrSize] = {0};

  int byteArrLen = packetSize;
  int rssi = LoRa.packetRssi();
  float snr = round(LoRa.packetSnr());

  // read packet
  // for (int i = 0; i < byteArrLen && LoRa.available(); i++)
  int l = 0;
  while (LoRa.available())
  {
    byteArr[l] = LoRa.read();
    // Serial.print((char)LoRa.read());
    l++;
  }

  byteArr[byteArrLen] = '\0'; // 0, \0

#ifdef DEBUG
  Serial.print(F("> [LoRa] Length: "));
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
  if (!(sizeof(byteArr) >= 4 && byteArr[0] == 'Z' && byteArr[1] == ':' && isdigit(byteArr[2]) && isdigit(byteArr[3])))
  {
    Serial.println(F("> [LoRa] ERR: packet format"));
    return;
  }
  else
  {
    Serial.println(F("> [LoRa] Valid format"));
  }

#ifdef DEBUGX
  Serial.println(F("> [LoRa] DBG: "));
  for (uint8_t i = 0; i < byteArrLen; i++)
  {
    /*Serial.print(i);
    Serial.print(" ");
    Serial.print(byteArr[i]);
    Serial.print(" ");*/
    Serial.println((char)byteArr[i]);
  }
  Serial.println();
#endif

  String input_str = "";
  int input_size = (byteArr[2] - 48) * 10 + (byteArr[3] - 48);

#ifdef DEBUG
  Serial.print(F("> [LoRa] Length byte: "));
  Serial.println(input_size);
#endif

  if (byteArrLen > 0 && byteArrLen <= byteArrSize && byteArrLen == input_size)
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
          // Serial.print((char)byteArr[i]);
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

    /*Serial.print(F(",RSSI:"));
    Serial.print(rssi);
    Serial.print(F(",SNR:"));
    Serial.print(snr);
    Serial.print(F(",RN:"));
    Serial.println(getUniqueID());*/

    input_str += ",RSSI:";
    input_str += String(rssi);
    input_str += ",SNR:";
    input_str += String(snr);
    input_str += ",RN:";
    input_str += getUniqueID();
    if (input_str.length() < 4)
    {
      Serial.println(F("> [LoRa] ERR: string empty"));
      return; // if there's no packet, return
    }
#ifdef DEBUG
    Serial.print("> input_str: ");
    Serial.println(input_str);
#endif

    JsonDocument ccJson; // 384
    //  Split the input string into key-value pairs using comma separator
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

#ifdef DEBUG
        Serial.print("[");
        Serial.print(key);
        Serial.print("]");
        Serial.println(value_str);
#endif

        if (key.startsWith("N") || key.startsWith("RN") || key.startsWith("F") || key.startsWith("RF"))
        {
          ccJson[key] = value_str;
        }
        else if ((key.startsWith("T") || key.startsWith("H") || key.startsWith("P")) || (key.startsWith("V") && value_str.length() > 1))
        {
          float valueFloat = value_str.toFloat() / 10.0;
          ccJson[key] = round(valueFloat * 10.0) / 10.0;
        }
        else
        {
          int valueInt = value_str.toInt();
          ccJson[key] = valueInt;
        }
      }
      pos = sep_pos + 1;
    }

    ccJson.remove("Z");
    ccJson["timestamp"] = timeClient.getEpochTime();

    String ccJsonStr;
    ccJson.shrinkToFit(); // optional

    serializeJson(ccJson, ccJsonStr);

    Serial.print("> [JSON] ");
    Serial.println(ccJsonStr);

    // if (!ccJson["N"].isNull())
    if (true)
    {
      //String topic = String(MQTT_TOPIC) + "/" + String(ccJson["N"].as<String>()) + "/json";
      String topic = String(MQTT_TOPIC) + "/acdc/json";
      if (!mqttClient.connected())
      {
        Serial.println("> [MQTT] Not connected");
        connectToMqtt();
      }

      boolean publishMqtt = true;
      boolean published = true;
      //boolean retained = !ccJson["R"].isNull();
      boolean retained = false;

      if (publishMqtt)
      {
        // Serial.println(ccJsonStr.c_str());
        // BUG
        published = mqttClient.publish(topic.c_str(), ccJsonStr.c_str(), retained);
      }
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
    if (!ccJson.isNull())
    {
      myData.addPacket(ccJsonStr);
    }
    ccJsonStr = "";
    notifyClients();
  } // length 0
}

void initLoRa()
{
  // Start LoRa
  Serial.print(F("> [LoRa] Initializing... "));
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setSPI(SPI);
  LoRa.setPins(SS, RST, DIO0);

  int lo_state = LoRa.begin(LO_FREQ);
  if (lo_state)
  {
    // LoRa.setGain(6);
    // LoRa.setPreambleLength(8);
    // LoRa.setCodingRate4(5);
    LoRa.setSpreadingFactor(10);
    LoRa.setSyncWord(0x13);
    LoRa.enableCrc();
    LoRa.onReceive(processLoRaData);
    LoRa.receive();
    Serial.println(F("OK"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(lo_state);
    reboot();
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
    timeClient.begin();
    timeClient.update();
    myData.boottime = timeClient.getEpochTime();
  }
  // Init LoRa
  initLoRa();
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
}
