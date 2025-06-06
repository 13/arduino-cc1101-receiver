#include "helpers.h"

// Last 4 digits of ChipID
String getUniqueID()
{
  String uid = "0";
  uid = WiFi.macAddress().substring(12);
  uid.replace(":", "");
  return uid;
}

// Reboot
void reboot()
{
  Serial.println("> [System] Rebooting...");
  delay(200);
  ESP.restart();
}

// boolToString()
const char *boolToString(boolean value)
{
  return value ? "true" : "false";
}

// Turn off builtin LED
void turnOffLed()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

// Initialize LittleFS
void initFS()
{
  if (!LittleFS.begin())
  {
    Serial.println(F("> [LittleFS] ERROR "));
    reboot();
  }
}

// Initialize mDNS
void initMDNS()
{
  if (!MDNS.begin(hostname.c_str()))
  {
    Serial.println(F("> [mDNS] ERROR"));
  }
  else
  {
    Serial.println(F("> [mDNS] Starting... OK"));
    Serial.print(F("> [mDNS] http://"));
    Serial.print(hostname.c_str());
    Serial.println(F(".muh"));
    MDNS.addService("http", "tcp", 80);
  }
}

// Get state
void getState()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    myData.ip = WiFi.localIP().toString();
    myData.mac = WiFi.macAddress();
    myData.ssid = WiFi.SSID();
    myData.rssi = WiFi.RSSI();
    myData.desc = DEVICE_DESCRIPTION;
    myData.hostname = hostname;
#if defined(ESP8266)
    // myData.hostname = WiFi.hostname();
    myData.resetreason = ESP.getResetReason();
    myData.memfrag = ESP.getHeapFragmentation();
    myData.cpu = String(ESP.getChipId());
#endif
#if defined(ESP32)
    // myData.hostname = WiFi.getHostname();
    myData.resetreason = esp_reset_reason();
    myData.memfrag = ESP.getMaxAllocHeap();
    myData.cpu = String(ESP.getChipModel()) + "(v" + String(ESP.getChipRevision()) + ")" + " CPU" + String(ESP.getChipCores());
#endif
    myData.memfree = ESP.getFreeHeap();
    myData.uptime = countMsg;
    myData.version = VERSION;
    myData.timestamp = Rome.toLocal(timeClient.getEpochTime());
  }
}

// print mark
void printMARK()
{
  if (countMsg == 0)
  {
    Serial.println(F("> [MARK] Starting... OK"));
    countMsg++;
  }
  if (countMsg == UINT32_MAX)
  {
    countMsg = 1;
  }
  if (millis() - lastMillisMark >= INTERVAL_1MIN)
  {
    Serial.print(F("> [MARK] Uptime: "));

    if (countMsg >= 60)
    {
      int hours = countMsg / 60;
      int remMins = countMsg % 60;
      if (hours >= 24)
      {
        int days = hours / 24;
        hours = hours % 24;
        Serial.print(days);
        Serial.print(F("d "));
      }
      Serial.print(hours);
      Serial.print(F("h "));
      Serial.print(remMins);
      Serial.println(F("m"));
    }
    else
    {
      Serial.print(countMsg);
      Serial.println(F("m"));
    }
    countMsg++;
    lastMillisMark += INTERVAL_1MIN;

    // 1 minute status update
    if (WiFi.status() == WL_CONNECTED)
    {
      connectToMqtt();
      timeClient.update();
      notifyClients();
    }
#ifndef REQUIRES_INTERNET
    // Check every 5 minutes online status
    if (countMsg % 5 == 0)
    {
      checkWiFi();
      if (WiFi.status() != WL_CONNECTED)
      {
        reboot();
      }
    }
#endif
  }
}

// WiFi
void checkWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  else
  {
    if (WiFi.status() == WL_CONNECTED && countMsg < 1)
    {
      Serial.println("> [WiFi] Connected ");
      Serial.print("> [WiFi] IP: ");
      Serial.println(WiFi.localIP().toString());
    }
  }
}

void connectToWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("> [WiFi] Connecting...");
  for (int i = 0; i < 20; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println(" OK");
      break;
    }
    Serial.print(".");
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("> [WiFi] IP: ");
    Serial.println(WiFi.localIP().toString());

    if (WiFi.localIP() == IPAddress(0, 0, 0, 0))
    {
      Serial.println(" NO DHCP LEASE");
#ifdef REQUIRES_INTERNET
      reboot();
#endif
    }

    getState();
  }
  else
  {
    Serial.println(" ERR TIMEOUT");
#ifdef REQUIRES_INTERNET
    reboot();
#endif
  }
}

// MQTT
void checkMqtt()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqttClient.connected())
    {
      Serial.println("> [MQTT] Not connected loop");
      long mqttNow = millis();
      if (mqttNow - mqttLastReconnectAttempt > 5000)
      {
        mqttLastReconnectAttempt = mqttNow;
        // Attempt to reconnect
        if (connectToMqtt())
        {
          mqttLastReconnectAttempt = 0;
        }
      }
    }
    else
    {
      mqttClient.loop();
    }
  }
}

boolean connectToMqtt()
{
  String lastWillTopic = MQTT_TOPIC_LWT;
  lastWillTopic += "/";
  lastWillTopic += hostname;
  String ipTopic = lastWillTopic;
  ipTopic += "/IP";
  String versionTopic = lastWillTopic;
  versionTopic += "/VERSION";
  lastWillTopic += "/LWT";

  if (!mqttClient.connected())
  {
    Serial.print("> [MQTT] Connecting...");
    if (mqttClient.connect(hostname.c_str(), lastWillTopic.c_str(), 1, true, "offline"))
    {
      Serial.println(" OK");
      mqttClient.publish(lastWillTopic.c_str(), "online", true);
      mqttClient.publish(ipTopic.c_str(), WiFi.localIP().toString().c_str(), true);
      mqttClient.publish(versionTopic.c_str(), VERSION, true);
#ifdef MQTT_SUBSCRIBE
      /*if (mqtt_topics[0] != NULL)
      {
        subscribeMqtt();
      }*/
#ifdef MQTT_SUBSCRIBE_TOPIC
      subscribeMqtt();
#endif
#endif
    }
    else
    {
      Serial.print(" failed, rc=");
      Serial.println(mqttClient.state());
    }
  }
  else
  {
    // Serial.println("> [MQTT] Connected");
    mqttClient.publish(lastWillTopic.c_str(), "online", true);
    mqttClient.publish(ipTopic.c_str(), WiFi.localIP().toString().c_str(), true);
    mqttClient.publish(versionTopic.c_str(), VERSION, true);
  }

  return mqttClient.connected();
}

#ifdef MQTT_SUBSCRIBE
void subscribeMqtt()
{
  // int numTopics = sizeof(mqtt_topics) / sizeof(mqtt_topics[0]);
  /*for (int i = 0; mqtt_topics[i] != NULL; i++)
  {
    Serial.print("[MQTT]: Subscribing ");
    Serial.print(mqtt_topics[i]);
    Serial.println(" ... OK");
    mqttClient.subscribe(mqtt_topics[i]);
  }*/
#ifdef MQTT_SUBSCRIBE_TOPIC
  Serial.print("> [MQTT] Subscribing... ");
  Serial.print(MQTT_SUBSCRIBE_TOPIC);
  Serial.println(" OK");
  mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC);
#endif
}
#endif

// http & websocket
String wsSerializeJson()
{
  myData.uptime = countMsg;
  myData.rssi = WiFi.RSSI();
  myData.memfree = ESP.getFreeHeap();
#if defined(ESP8266)
  myData.memfrag = ESP.getHeapFragmentation();
#endif
#if defined(ESP32)
  uint8_t fragmentationPercentage = static_cast<uint8_t>((100 * (myData.memfree - ESP.getMaxAllocHeap())) / myData.memfree);
  myData.memfrag = fragmentationPercentage;
#endif
  myData.timestamp = Rome.toLocal(timeClient.getEpochTime());
  String jsonStr = myData.toJson();
  Serial.print("> [WS] ");
  Serial.println(jsonStr);
  return jsonStr;
}

void notifyClients()
{
  if (connectedClients > 0)
  {
    ws.textAll(wsSerializeJson());
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("> [WebSocket] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    connectedClients++;
    getState();
    notifyClients();
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("> [WebSocket] Client #%u disconnected\n", client->id());
    connectedClients--;
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/css/bootstrap.min.css", "text/css"); });
  server.on("/js/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/js/bootstrap.bundle.min.js", "text/javascript"); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", myData.ip.c_str()); });
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "pong"); });
  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", wsSerializeJson()); });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
            {
          AsyncWebServerResponse *response;
            response = request->beginResponse(200, "application/json", "{\"reboot\":true,\"message\":\"Rebooting...\"}");
            response->addHeader("Connection", "close");
            request->send(response);
            Serial.println(F("> [HTTP] Rebooting..."));
            reboot(); });
  server.on("/update.html", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/update.html", "text/html"); });
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    AsyncWebServerResponse *response;

    if (!Update.hasError())
    {
        response = request->beginResponse(200, "application/json", "{\"success\":true,\"message\":\"Updated successfully!\",\"version\":\"v1.0\"}");
        Serial.println(F("> [OTA] Successful"));
    }
    else
    {
        response = request->beginResponse(500, "application/json", "{\"success\":false,\"message\":\"Update failed\",\"version\":\"v1.0\"}");
        Serial.println(F("> [OTA] Update failed"));
    }

    response->addHeader("Connection", "close");
    request->send(response); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        // otaUpdater.update(filename, index, data, len, final);
        if (!index)
        {
          Serial.print(F("> [OTA] Updating... "));
          Serial.println(filename);
#if defined(ESP8266)
          Update.runAsync(true);
#endif
          uint32_t free_space;
          int cmd;

          if (filename.indexOf("littlefs") > -1)
          {
#if defined(ESP8266)
            FSInfo fs_info;
            LittleFS.info(fs_info);
            free_space = fs_info.totalBytes;
            cmd = U_FS;
#endif
#if defined(ESP32)
            free_space = LittleFS.totalBytes();
            cmd = U_SPIFFS;
#endif
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

        if (Update.write(data, len) != len)
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
      });

  // Start server
  server.begin();
}