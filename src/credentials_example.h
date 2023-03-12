#define VERBOSE
// #define VERBOSE_FW // Firmware Version
// #define DEBUG
// #define DEBUG_CRC

#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 200
#if defined(ESP8266)
const char* wifi_ssid = "";
const char* wifi_pass  = "";
#define GD0 4 // GDO0 GPIO4 = D2
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