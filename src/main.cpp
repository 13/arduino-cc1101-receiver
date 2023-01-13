#include <Arduino.h>
#include <RadioLib.h>

// #define VERBOSE
// #define DEBUG
// #define CHECKSIZE_ALT

#define CC_FREQ 868.32
#define CC_POWER 10
#define GD0 2

// CC1101
// CS pin:    10
// GDO0 pin:  2
CC1101 cc = new Module(10, GD0, RADIOLIB_NC);

// receive
const uint8_t byteArrSize = 61;
byte byteArr[byteArrSize] = {0};

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
  Serial.print(F("> [CC1101] Initializing... "));
  int state = cc.begin(CC_FREQ, 48.0, 48.0, 135.0, CC_POWER, 16);
  if (state == ERR_NONE)
  {
    Serial.println(F("OK"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true)
      ;
  }
}

void loop()
{
#ifdef MARK
  printMARK();
#endif
#ifdef DEBUG
  Serial.print(F("> [CC1101] Receive... "));
#endif
  int state = cc.receive(byteArr, sizeof(byteArr) / sizeof(byteArr[0]) + 1); // +1
  if (state == ERR_NONE)
  {
#ifndef CHECKSIZE_ALT
    // check packet size
    boolean equalPacketSize = (byteArr[0] == (sizeof(byteArr) / sizeof(byteArr[0]))) ? true : false;
#elif
    boolean equalPacketSize = (byteArr[0] == byteArr[3]) ? true : false;
#endif
#ifdef DEBUG
    Serial.print((char)byteArr[0]);
    Serial.print(" - ");
    Serial.println((char)byteArr[3]);
#endif
    if (equalPacketSize)
    {
#ifdef DEBUG
      Serial.println(F("OK"));
      Serial.print(F("> [CC1101] Received packet size: "));
      Serial.println(byteArr[0]);
#endif
      // add
      byteArr[sizeof(byteArr) / sizeof(byteArr[0])] = '\0';
      // i = 1 remove length byte
      // print char
      if ((char)byteArr[1] == 'Z')
      {
#ifndef CHECKSIZE_ALT
        for (uint8_t i = 1; i < sizeof(byteArr); i++)
#elif
        for (uint8_t i = 1; i < byteArr[0]; i++)
#endif
        {
          Serial.print((char)byteArr[i]);
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
      Serial.println(F("ERR LENGTH MISMATCH"));
    }
#endif
  }
#ifdef DEBUG
  else if (state == ERR_CRC_MISMATCH)
  {
    Serial.println(F("ERR CRC MISMATCH"));
  }
  else if (state == ERR_RX_TIMEOUT)
  {
    Serial.println(F("ERR RX TIMEOUT"));
  }
  else
  {
    Serial.print(F("ERR "));
    Serial.println(state);
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
