#include "OTAUpdate.h"
#include <Updater.h>
#include <FS.h>
#define SPIFFS LittleFS
#include <LittleFS.h>

void OTAUpdater::update(const String &filename, size_t index, const uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    Serial.print(F("> [OTA] Updating... "));
    Serial.println(filename);
    Update.runAsync(true);
    uint32_t free_space;
    int cmd;

    if (filename.indexOf("littlefs") > -1)
    {
      FSInfo fs_info;
      LittleFS.info(fs_info);
      free_space = fs_info.totalBytes;
      cmd = U_FS;
    }
    else
    {
      free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      cmd = U_FLASH;
    }

    if (!Update.begin(free_space, cmd))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(const_cast<uint8_t *>(data), len) != len)
  {
    Update.printError(Serial);
  }

  if (final)
  {
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      Serial.println(F("> [OTA] Successful"));
    }
  }
}