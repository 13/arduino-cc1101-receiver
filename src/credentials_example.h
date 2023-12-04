#define VERBOSE
//#define DEBUG
//#define DEBUG_CRC

#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 200

/* WiFi */
const char* wifi_ssid = "network";
const char* wifi_pass  = "";
/* MQTT server credentials */
const char* mqtt_user = "";
const char* mqtt_pass = "";
const char* mqtt_server = "192.168.22.5";
const char* mqtt_topic = "muh/sensors";
const char* mqtt_topic_lwt = "muh/esp";
uint16_t mqtt_port = 1883;
// #define GD0 5 /* Bug with ESP8266 */
#define WSPACKETS