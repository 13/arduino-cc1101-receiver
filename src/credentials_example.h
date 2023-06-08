#define WSTATION
#define VERBOSE
// #define DEBUG
// #define DEBUG_CRC

#if defined(WSTATION)
#define CC_FREQ 868.3
#define CC_BR 8.21
#define CC_FD 57.136417
#define CC_RxBW 270
#define CC_SM 3 // 3=30/32 7=30/32+threshold
#define CC_POWER 10
#define CC_DELAY 200
#else
#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 200
#endif
#if defined(ESP8266)
// WiFi
const char *wifi_ssid = "";
const char *wifi_pass = "";
// MQTT server credentials
const char *mqtt_user = "";
const char *mqtt_pass = "";
const char *mqtt_server = "192.168.22.5";
const char *mqtt_topic = "muh/sensors";
const char* mqtt_topic_lwt = "muh/esp";
uint16_t mqtt_port = 1883;
#endif

// GIT
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif
#ifndef GIT_VERSION_SHORT
#define GIT_VERSION_SHORT "0"
#endif
