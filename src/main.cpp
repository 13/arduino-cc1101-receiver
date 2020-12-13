#include <Arduino.h>
#include <RadioLib.h>

#define MARK
#define VERBOSE
//#define DEBUG

#define CC_FREQ     868.32
#define CC_POWER    10

// CC1101
// CS pin:    10
// GDO0 pin:  2
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// receive
const uint8_t byteArrSize = 61;
byte byteArr[byteArrSize] = {0};

#ifdef MARK
  // one minute mark
  #define INTERVAL_1MIN (1*60*1000L)
  unsigned long lastMillis = 0L;
  uint32_t countMsg = 0;
#endif

// platformio fix
void printMARK();
void printHex(uint8_t num);
void setFlag(void);

void setup() {
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
  // Start CC1101
  Serial.print(F("> [CC1101] Initializing... "));
  int state = cc.begin(CC_FREQ, 48.0, 48.0, 135.0, CC_POWER, 16);
  if (state == ERR_NONE) {
    Serial.println(F("OK"));
  } else {
    Serial.print(F("ERR "));
    Serial.println(state);
    while (true);
  }
}

void loop(){
#ifdef MARK
  printMARK();
#endif
  Serial.print(F("> [CC1101] Receive... "));
  int state = cc.receive(byteArr,sizeof(byteArr)/sizeof(byteArr[0])+1); // +1
  if (state == ERR_NONE) {
    // check packet size
    boolean equalPacketSize = (byteArr[0] == (sizeof(byteArr)/sizeof(byteArr[0]))) ? true : false;
    if (equalPacketSize){
      Serial.println(F("OK"));
      byteArr[sizeof(byteArr)/sizeof(byteArr[0])] = '\0';
      // i = 1 remove length byte
      // print char
      for(uint8_t i=1; i<sizeof(byteArr); i++){
        Serial.print((char)byteArr[i]);
      }
      Serial.print(F(",RSSI:"));
      Serial.print(cc.getRSSI());
      Serial.print(F(",LQI:"));
      Serial.println(cc.getLQI());
    } else {
      Serial.println(F("ERR LENGTH MISMATCH"));
    }
  } else if (state == ERR_CRC_MISMATCH) {
      Serial.println(F("ERR CRC MISMATCH"));
  } else if (state == ERR_RX_TIMEOUT) {
      Serial.println(F("ERR RX TIMEOUT"));
  } else {
      Serial.print(F("ERR "));
      Serial.println(state);
  }
}

#ifdef MARK
void printMARK() {
  if (countMsg == 0){
    Serial.println(F("> Running... OK"));
    countMsg++;
  }
  if (millis() - lastMillis >= INTERVAL_1MIN){
    Serial.print(F("> Uptime: "));
    Serial.print(countMsg);
    Serial.println(F(" min"));
    countMsg++;
    lastMillis += INTERVAL_1MIN;
  }
}
#endif

void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}
