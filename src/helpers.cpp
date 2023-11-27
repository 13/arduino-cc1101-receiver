#include "helpers.h"

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
    myData.version = GIT_VERSION;
  }
}

// WiFi
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
      mqttClient.publish(versionTopic.c_str(), GIT_VERSION, true);
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
    mqttClient.publish(versionTopic.c_str(), GIT_VERSION, true);
  }
  return mqttClient.connected();
}