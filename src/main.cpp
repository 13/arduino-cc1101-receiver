#include <Arduino.h>
#include <EEPROM.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "credentials.h"

// Edit credentials.h

// cc1101
const uint8_t byteArrSize = 61;

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
}

void loop()
{
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

#ifdef VERBOSE
      Serial.println(F("OK"));

      Serial.print(F("> [CC1101] Length: "));
      Serial.println(byteArrLen);
#endif
      // byteArr[byteArrLen] = '0'; // 0, \0
      for (uint8_t i = 0; i < byteArrLen; i++)
      {
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(ELECHOUSE_cc1101.getRssi());
      Serial.print(F(",LQI:"));
      Serial.print(ELECHOUSE_cc1101.getLqi());
      Serial.print(F(",RN:"));
      Serial.println(String(getUniqueID(), HEX));
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
      Serial.print(ELECHOUSE_cc1101.getRssi());
      Serial.print(F(",LQI:"));
      Serial.print(ELECHOUSE_cc1101.getLqi());
      Serial.print(F(",RN:"));
      Serial.println(String(getUniqueID(), HEX));
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
