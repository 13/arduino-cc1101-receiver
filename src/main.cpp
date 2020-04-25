#include <Arduino.h>
#include <RadioLib.h>
#include <MemoryFree.h>

//#define ENABLE_NODE_ADDRESS
//#define ENABLE_ENCODING
//#define ENABLE_SYNC_WORD
//#define DISABLE_SYNC_WORD_FILTERING
//#define DISABLE_CRC
//#define ENABLE_INTERRUPT
#define KEEP_ALIVE_MSG
#define DEBUG

// CC1101 has the following connections:
// CS pin:    10
// GDO0 pin:  2
// RST pin:   unused
// GDO2 pin:  3 (optional)
CC1101 cc = new Module(10, 2, RADIOLIB_NC);

#ifdef ENABLE_INTERRUPT
// interrupt
volatile bool receivedFlag = false;
volatile bool enableInterrupt = true;
#endif

#ifdef KEEP_ALIVE_MSG
// debug every minute keepalive
#define INTERVAL_1MIN (1*60*1000L)
unsigned long lastMillis = 0L;
uint32_t countMsg = 1;
#endif

// platformio fix
#ifdef ENABLE_INTERRUPT
void setFlag(void);
#endif

void setup() {
  Serial.begin(9600);
  delay(10);
#ifdef DEBUG
  delay(4000);
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
#ifdef ENABLE_ENCODING
  // set encoding 
  Serial.print("> [CC1101] Setting Encoding ... ");
  state = cc.setEncoding(0);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef ENABLE_NODE_ADDRESS
  // set node address
  Serial.print("> [CC1101] Setting Node Address ... ");
  state = cc.setNodeAddress(0x22,0);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef ENABLE_SYNC_WORD 
  // 2 bytes can be set as sync word
  Serial.print("> [CC1101] Setting Sync Word ... ");
  state = cc.setSyncWord(0xd4, 0x2d);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef DISABLE_SYNC_WORD_FILTERING 
  // disable sync word filtering
  Serial.print("> [CC1101] Disable Sync Word Filtering ... ");
  state = cc.disableSyncWordFiltering();
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef DISABLE_CRC 
  // disable crc 
  Serial.print("> [CC1101] Disabling CRC ... ");
  state = cc.setCrcFiltering(false);
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
#ifdef ENABLE_INTERRUPT
  // set the function that will be called
  // when new packet is received
  cc.setGdo0Action(setFlag);
  // start listening for packets
  Serial.print("> [CC1101] Starting to listen ... ");
  state = cc.startReceive();
  if (state == ERR_NONE) {
    Serial.println("OK");
  } else {
    Serial.print("ERR ");
    Serial.println(state);
    while (true);
  }
#endif
}

void loop() {
#ifdef KEEP_ALIVE_MSG
  //if (millis() - lastMillis >= 1*60*1000UL){
  if (millis() - lastMillis >= INTERVAL_1MIN){
    Serial.print("> Keep-Alive: ");
    Serial.println(countMsg);
    countMsg++;
    lastMillis += INTERVAL_1MIN;
    Serial.print("> Uptime: ");
    Serial.print(lastMillis/1000); 
    Serial.println(" sec");
    Serial.print("freeMemory()=");
    Serial.println(freeMemory());
  }
#endif
#ifdef ENABLE_INTERRUPT
  if(receivedFlag) {
    //Serial.println("[CC1101] Received transmission ... ");
    enableInterrupt = false;
    receivedFlag = false;
#endif

    String str;
#ifdef ENABLE_INTERRUPT
    int state = cc.readData(str);
#else
    int state = cc.receive(str);
#endif

    if (state == ERR_NONE) {
      str.trim();
      //if (str.length() < 64){
      //if (str.charAt(0) == 'M' && str.charAt(1) == 'U' && str.charAt(2) == 'H') {
      //if (str.charAt(0) == 'M') {
        //Serial.println("");
        Serial.print(str);
        Serial.print(",RSSI:");
        Serial.print(cc.getRSSI());
        Serial.print(",LQI:");
        Serial.print(cc.getLQI());
        Serial.println("");
      /*} else {
        Serial.print(str);
        Serial.println(" success but not!");
      }*/
      //}

    } else if (state == ERR_CRC_MISMATCH) {
        //Serial.println("");
        Serial.println("CRC ERROR");
    } else {
        //Serial.println("");
        Serial.print("ERROR ");
        Serial.println(state);
    }
#ifdef ENABLE_INTERRUPT
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
#endif
}

#ifdef ENABLE_INTERRUPT
void setFlag(void) {
  if(!enableInterrupt) {
    return;
  }
  receivedFlag = true;
}
#endif