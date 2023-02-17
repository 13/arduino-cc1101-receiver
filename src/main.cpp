#include <Arduino.h>
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
    for (uint8_t i = 0; i < sizeof(byteArr); i++)
    {
      Serial.print((char)byteArr[i]);
    }
    Serial.print(F(",RSSI:"));
    Serial.print(radio.getRSSI());
    Serial.print(F(",LQI:"));
    Serial.println(radio.getLQI());
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
