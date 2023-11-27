#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

class OTAUpdater {
public:
  static void update(const String& filename, const uint8_t* data, size_t len, bool final);
};

#endif // OTA_UPDATE_H
