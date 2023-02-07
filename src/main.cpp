#include <Arduino.h>
#include <RadioLib.h>
#include "credentials.h"

// Edit credentials.h

// CC1101
// CS pin:    10
// GDO0 pin:  2
CC1101 cc = new Module(10, GD0, RADIOLIB_NC);

// receive
const uint8_t byteArrSize = 61;
byte byteArr[byteArrSize] = {0};

#ifdef DEBUG
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
  Serial.print(F("> [CC1101] Initializing... "));
  int cc_state = cc.begin(CC_FREQ, 48.0, 48.0, 135.0, CC_POWER, 16);
  if (cc_state == ERR_NONE)
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
}

void loop()
{
#ifdef MARK
  printMARK();
#endif
#ifdef VERBOSE
  Serial.print(F("> [CC1101] Receive... "));
#endif
  int cc_rx_state = cc.receive(byteArr, sizeof(byteArr) / sizeof(byteArr[0]) + 1); // +1
  if (cc_rx_state == ERR_NONE)
  {
#ifdef DEBUG
    Serial.print(F("OK "));
#endif
    // check packet size
    boolean equalPacketSize = (byteArr[0] == (sizeof(byteArr) / sizeof(byteArr[0]))) ? true : false;
    if (equalPacketSize)
    {
#ifdef VERBOSE
      Serial.println(F("OK"));
#endif
      // add
      byteArr[sizeof(byteArr) / sizeof(byteArr[0])] = '\0';
      // i = 1 remove length byte
      // print char
      if ((char)byteArr[1] == 'Z')
      {
        for (uint8_t i = 1; i < sizeof(byteArr); i++)
        {
          if (byteArr[i] != 32)
          {
            Serial.print((char)byteArr[i]);
          }
        }
        Serial.print(F(",RSSI:"));
        Serial.print(cc.getRSSI());
        Serial.print(F(",LQI:"));
        Serial.println(cc.getLQI());
      }
#ifdef DEBUG
      else
      {
        Serial.print(F("> [CC1101] Receive... "));
        Serial.println(F("ERR Z"));
        Serial.print(F("> "));
        for (uint8_t i = 0; i < sizeof(byteArr); i++)
        {
          Serial.print((char)byteArr[i]);
        }
        Serial.println();
      }
#endif
    }
#ifdef DEBUG
    else
    {
      Serial.print(F("ERR LENGTH: "));
      Serial.println(byteArr[0]);
      Serial.print(F("> "));
      for (uint8_t i = 1; i < byteArr[0]; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.println();
    }
#endif
  }
#ifdef DEBUG
  else if (cc_rx_state == ERR_CRC_MISMATCH)
  {
    Serial.println(F("ERR CRC MISMATCH"));
  }
  else if (cc_rx_state == ERR_RX_TIMEOUT)
  {
    Serial.println(F("ERR RX TIMEOUT"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(cc_rx_state);
  }
#endif
}

#ifdef MARK
void printMARK()
{
  if (countMsg == 0)
  {
    Serial.println(F("> Running... OK"));
    countMsg++;
  }
  if (millis() - lastMillis >= INTERVAL_1MIN)
  {
    Serial.print(F("> Uptime: "));
    Serial.print(countMsg);
    Serial.println(F(" min"));
    countMsg++;
    lastMillis += INTERVAL_1MIN;
  }
}
#endif
