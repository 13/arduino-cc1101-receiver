#include <Arduino.h>
#include <RadioLib.h>
#include <MemoryFree.h>

#define MARK
#define DEBUG

// CC1101 has the following connections:
// CS pin:    10
// GDO0 pin:  2
// RST pin:   unused
// GDO2 pin:  3 (optional)
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

// interrupt
volatile bool receivedFlag = false;
volatile bool enableInterrupt = true;

#ifdef MARK 
// 1 minute mark
#define INTERVAL_1MIN (1*60*1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 1;
#endif

// platformio fix
void printHex(uint8_t num);
void printRAM();
void setFlag(void);

void setup() {
  Serial.begin(9600);
  delay(10);
#ifdef DEBUG
  delay(1000);
#endif
  // Start Boot
  Serial.println("> ");
  Serial.println("> ");
  Serial.print("> Booting... Compiled: ");
  Serial.println(__TIMESTAMP__);

  // initialize CC1101 with default settings
  Serial.print("> [CC1101] Initializing ... ");
  //int state = cc.begin();
  int state = cc.begin(868.32, 4.8, 48.0, 325.0, 0, 4);
  //int state = cc.begin(868.325, 17.240, 48.0, 325.0, 0, 4);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
  // set the function that will be called
  // when new packet is received
  cc.setGdo0Action(setFlag);
  // start listening for packets
  Serial.println("> [CC1101] Interrupt mode");
  Serial.print("> [CC1101] Starting to listen ... ");
  state = cc.startReceive();
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
}

void loop() {
#ifdef MARK
  if (millis() - lastMillis >= INTERVAL_1MIN){
    Serial.print("> Uptime: ");
    Serial.print(countMsg);
    Serial.println(" min");
    countMsg++;
    lastMillis += INTERVAL_1MIN;
    //Serial.print("> Uptime: ");
    //Serial.print(lastMillis/1000); 
    //Serial.println(" sec");
    printRAM();
  }
#endif
  if(receivedFlag) {
    enableInterrupt = false;
    receivedFlag = false;
    /*Serial.print("> [CC1101] Standby while reading ... ");
    int state = cc.standby();
    if (state == ERR_NONE) {
      Serial.println("OK");
    } else {
      Serial.print("ERR ");
      Serial.println(state);
      while (true);
    }*/
    Serial.println("> [CC1101] Receive ...");
    byte byteArr[61];
    //Serial.print("> Packet Length: ");
    //Serial.println(sizeof(byteArr)/sizeof(byteArr[0]));
    int state = cc.readData(byteArr,sizeof(byteArr)/sizeof(byteArr[0])+1); // +1
    Serial.println("> [CC1101] Receive OK");
    if (state == ERR_NONE) {
      Serial.print("> Packet Length Received: ");
      Serial.print(byteArr[0]);
      Serial.println("");
      // check packet size
      if (byteArr[0] == (sizeof(byteArr)/sizeof(byteArr[0]))){
        // i = 1 remove first byte
        for(uint8_t i=1; i<sizeof(byteArr); i++){
          printHex(byteArr[i]);
        }
        Serial.println("");
      } else {
        Serial.println("> Packet Length Wrong");
      }
    } else if (state == ERR_CRC_MISMATCH) {
        Serial.println("CRC ERROR ");
    } else {
        Serial.print("ERROR: ");
        Serial.println(state);
    }
    Serial.print("> [CC1101] Restarting to listen ... ");
    state = cc.startReceive();
    if (state == ERR_NONE) {
      Serial.println("OK");
    } else {
      Serial.print("ERR ");
      Serial.println(state);
    }
    enableInterrupt = true;
  }
}

void setFlag(void) {
  if(!enableInterrupt) {
    return;
  }
  receivedFlag = true;
}

void printRAM(){
  Serial.print("> SRAM: ");
  Serial.print(freeMemory());
  Serial.print("/2048 ");
  Serial.print((int)((float)freeMemory()/2048*100));
  Serial.println("% free");
}

void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}