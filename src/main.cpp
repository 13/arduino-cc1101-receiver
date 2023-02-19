#include <Arduino.h>
#include <EEPROM.h>
#include <RadioLib.h>
#include "credentials.h"

// Edit credentials.h

// cc1101
const uint8_t byteArrSize = 61;
CC1101 radio = new Module(10, GD0, RADIOLIB_NC, 3);

#ifdef VERBOSE
// one minute mark
#define MARK
#define INTERVAL_1MIN (1 * 60 * 1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 0;
#endif

// platformio fix
void printMARK();
int getUniqueID();

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
  Serial.println(String(getUniqueID(), HEX));
#ifdef VERBOSE
  Serial.print(("> Mode: "));
  Serial.print(F("VERBOSE "));
#endif
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
  Serial.println();
  // Start CC1101
  Serial.print(F("> [CC1101] Initializing... "));
  int cc_state = radio.begin(CC_FREQ);
  if (cc_state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("OK"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_state);
    while (true)
      ;
  }
  // Enabling address filtering
  Serial.print(F("> [CC1101] Enabling Address Filtering... "));
  int cc_af_state = radio.setNodeAddress(0x01, 1);
  if (cc_af_state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("OK"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_af_state);
    while (true)
      ;
  }
}

void loop()
{
#ifdef MARK
  printMARK();
#endif
  byte byteArr[byteArrSize] = {0};
  int state = radio.receive(byteArr, byteArrSize);
  if (state == RADIOLIB_ERR_NONE)
  {

#ifdef VERBOSE
    Serial.print(F("> [CC1101] Receive... "));
    Serial.print(F("CRC "));
    Serial.println(F("OK"));

    Serial.print(F("> [CC1101] Length: "));
    Serial.println(sizeof(byteArr));
#endif
    // byteArr[sizeof(byteArr) / sizeof(byteArr[0])] = '\0'; // 0 \0

    for (uint8_t i = 0; i < sizeof(byteArr); i++)
    {
      Serial.print(byteArr[i], HEX);
    }
    Serial.println();

    for (uint8_t i = 0; i < sizeof(byteArr); i++)
    {
      if ((byteArr[i] >= '0' && byteArr[i] <= '9') ||
          (byteArr[i] >= 'A' && byteArr[i] <= 'Z') ||
          (byteArr[i] >= 'a' && byteArr[i] <= 'z') ||
          byteArr[i] == ',' || byteArr[i] == ':' || byteArr[i] == '-')
      {
        Serial.print((char)byteArr[i]);
      }
    }
    Serial.print(F(",RSSI:"));
    Serial.print((int)radio.getRSSI());
    Serial.print(F(",LQI:"));
    Serial.print(radio.getLQI());
    Serial.print(F(",RN:"));
    Serial.println(String(getUniqueID(), HEX));
  }
}

#ifdef MARK
void printMARK()
{
  if (countMsg == 0)
  {
    Serial.println(F("> [MARK] Starting... OK"));
    countMsg++;
  }
  if (millis() - lastMillis >= INTERVAL_1MIN)
  {
    Serial.print(F("> [MARK] Uptime: "));
    Serial.print(countMsg);
    Serial.println(F(" min"));
    countMsg++;
    lastMillis += INTERVAL_1MIN;
  }
}
#endif

// Last 4 digits of ChipID
int getUniqueID()
{
  int uid = 0;
  // read EEPROM serial number
  int address = 13;
  int serialNumber;
  if (EEPROM.read(address) != 255)
  {
    EEPROM.get(address, serialNumber);
    uid = serialNumber;
  }
  return uid;
}