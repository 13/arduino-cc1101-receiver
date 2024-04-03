#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define VERBOSE
// #define DEBUG
// #define DEBUG_CRC
#define REQUIRES_INTERNET
// #define MQTT_SUBSCRIBE
#define USE_CRYPTO // Decryption
#ifdef USE_CRYPTO
byte aeskey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
#endif

/* Sensor */
#define S_CC1101
// #define S_PARKING2

/* Device */
#define DEVICE_DESCRIPTION "Dachboden"

/* WiFi */
#define WIFI_SSID "muhxnetwork"
#define WIFI_PASS ""

/* MQTT server credentials */
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_SERVER "192.168.22.5"
#define MQTT_TOPIC "muh/sensors"
#define MQTT_TOPIC_LWT "muh/esp"
#define MQTT_PORT 1883
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