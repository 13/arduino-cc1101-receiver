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

// Get state
void getState()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    myData.ip = WiFi.localIP().toString();
    myData.mac = WiFi.macAddress();
    myData.ssid = WiFi.SSID();
    myData.rssi = WiFi.RSSI();
    myData.hostname = WiFi.hostname();
    myData.reset = ESP.getResetReason();
    myData.uptime = countMsg;
    myData.memfree = ESP.getFreeHeap();
    myData.memfrag = ESP.getHeapFragmentation();
    myData.version = VERSION;
    myData.timestamp = timeClient.getEpochTime();
  }
}

// WiFi
void checkWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
}

void connectToWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(hostname);
  WiFi.begin(wifi_ssid, wifi_pass);
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
      reboot();
    }

    getState();
  }
  else
  {
    Serial.println(" ERR TIMEOUT");
    reboot();
  }
}

// MQTT
void checkMqtt()
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

boolean connectToMqtt()
{
  String lastWillTopic = mqtt_topic_lwt;
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

// http & websocket
String wsSerializeJson()
{
  myData.uptime = countMsg;
  myData.rssi = WiFi.RSSI();
  myData.memfree = ESP.getFreeHeap();
  myData.memfrag = ESP.getHeapFragmentation();
  myData.timestamp = timeClient.getEpochTime();
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
          if (myData.uptime > 2) {
            response = request->beginResponse(200, "application/json", "{\"reboot\":true,\"message\":\"Rebooting...\"}");
            response->addHeader("Connection", "close");
            request->send(response);
            Serial.println(F("> [HTTP] Rebooting..."));
            reboot();
          } else {
            response = request->beginResponse(200, "application/json", "{\"reboot\":false,\"message\":\"Uptime less than or equal to 2, not rebooting.\"}");
            response->addHeader("Connection", "close");
            request->send(response);
          } });
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