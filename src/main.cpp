#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "credentials.h"

// Edit credentials.h

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
  int cc_state = ELECHOUSE_cc1101.getCC1101();
  if (cc_state)
  {
    Serial.println(F("OK"));
    ELECHOUSE_cc1101.Init();           // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setGDO0(GD0);     // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
    ELECHOUSE_cc1101.setCCMode(1);     // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(CC_FREQ);  // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setSyncMode(2);   // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setCrc(1);        // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
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
  // int cc_rx_state = cc.receive(byteArr, sizeof(byteArr) / sizeof(byteArr[0]) + 1); // +1
  int cc_rx_state = ELECHOUSE_cc1101.CheckReceiveFlag();
  if (cc_rx_state && ELECHOUSE_cc1101.CheckCRC())
  {
#ifdef DEBUG
    Serial.print(F("ERR_NONE "));
#endif
    int byteArrLen = ELECHOUSE_cc1101.ReceiveData(byteArr);
    // check packet size
    boolean equalPacketSize = (byteArr[0] == (sizeof(byteArr) / sizeof(byteArr[0]))) ? true : false;
    if (equalPacketSize)
    {
#ifdef VERBOSE
      Serial.println(F("OK"));
#endif
      // add
      byteArr[sizeof(byteArr) / sizeof(byteArr[0])] = '\0';
      byteArr[byteArrLen] = '\0';
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
        Serial.print(ELECHOUSE_cc1101.getRssi());
        Serial.print(F(",LQI:"));
        Serial.println(ELECHOUSE_cc1101.getLqi());
      }
#ifdef DEBUG
      else
      {
        Serial.print(F("> [CC1101] Receive... "));
        Serial.println(F("ERR Z"));
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
      for (uint8_t i = 1; i < byteArr[0]; i++)
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
