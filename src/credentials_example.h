#define VERBOSE
// #define VERBOSE_FW // Firmware Version
// #define DEBUG
// #define DEBUG_CRC

#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 200
#if defined(ESP8266)
// WiFi
const char* wifi_ssid = "";
const char* wifi_pass  = "";
// MQTT server credentials
const char* mqtt_user = "";
const char* mqtt_pass = "";
const char* mqtt_server = "192.168.22.5";
uint16_t mqtt_port = 1883;
#define GD0 4 // for esp8266! GDO0 on gpio4 = D2
#else
#define GD0 2
#endif

// GIT
#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif
#ifndef GIT_VERSION_SHORT
#define GIT_VERSION_SHORT "0"
#endif