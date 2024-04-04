#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define VERBOSE
// #define DEBUG
// #define DEBUG_CRC
#define REQUIRES_INTERNET
// #define MQTT_SUBSCRIBE

// Crypto
// openssl rand -hex 16
#define USE_CRYPTO
#ifdef USE_CRYPTO
#define AES_KEY "808639b9d210f261fefcce5a85c0cadb"
#endif

/* Sensor */
#define S_CC1101
// #define S_PARKING2

/* Device */
#define DEVICE_DESCRIPTION "Garten"

/* WiFi */
#define WIFI_SSID "network"
#define WIFI_PASS ""

/* MQTT server credentials */
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_SERVER "192.168.22.5"
#define MQTT_TOPIC "muh/sensors"
#define MQTT_TOPIC_LWT "muh/esp"
#define MQTT_PORT 1881 // 1881
#ifdef MQTT_SUBSCRIBE
#define MQTT_SUBSCRIBE_TOPIC "muh/sensors/#"
#endif

#ifdef S_CC1101
/* CC1101 */
#define CC_FREQ 868.32
#define CC_POWER 12
#define CC_DELAY 200
// #define GD0 5 /* Bug with ESP8266 */
#define WSPACKETS
#endif

#endif // CREDENTIALS_H