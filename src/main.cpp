#include <Arduino.h>
#include <SPI.h>
#include <CC1101_RF.h>
#include "credentials.h"

// Edit credentials.h

// cc1101
CC1101 radio;

#ifdef VERBOSE
// one minute mark
#define MARK
#define INTERVAL_1MIN (1 * 60 * 1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 0;
#endif

// platformio fix
void printMARK();

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
  Serial.println(F(__TIMESTAMP__));
  Serial.print(("> Mode: "));
#ifdef VERBOSE
  Serial.print(F("VERBOSE "));
#endif
#ifdef DEBUG
  Serial.print(F("DEBUG"));
#endif
  Serial.println();
  // Start CC1101
  Serial.println(F("> [CC1101] Initializing... "));
  SPI.begin();
  radio.begin(CC_FREQ);
  radio.setRXstate();
}

void loop()
{
#ifdef MARK
  printMARK();
#endif
#ifdef GD0
  if (digitalRead(GD0))
#endif
  {
    // receive
    const uint8_t byteArrSize = 64;
    byte byteArr[byteArrSize] = {0};
    int byteArrLen = radio.getPacket(byteArr);
#ifdef DEBUG
    Serial.print(F("> [CC1101] Receive... "));
#endif
    if (byteArrLen > 0 && radio.crcok())
    {
#ifdef VERBOSE
#ifndef DEBUG
      Serial.print(F("> [CC1101] Receive... "));
#endif
#endif
#ifdef VERBOSE
      Serial.print(F("CRC "));
#endif
#ifdef VERBOSE
      Serial.println(F("OK"));

      Serial.print(F("> [CC1101] Length: "));
      Serial.println(byteArrLen);
#endif
      // byteArr[byteArrLen] = '0';
      Serial.write(byteArr,byteArrLen);
      Serial.println();
      for (uint8_t i = 0; i < byteArrLen; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(radio.getRSSIdbm());
      Serial.print(F(",LQI:"));
      Serial.println(radio.getLQI());
    }
#ifdef DEBUG
    else
    {
      Serial.println(F("ERR CRC"));
#ifdef DEBUG_CRC
      for (uint8_t i = 0; i < byteArrSize; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(radio.getRSSIdbm());
      Serial.print(F(",LQI:"));
      Serial.println(radio.getLQI());
#endif
    }
#endif
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
