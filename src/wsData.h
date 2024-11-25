// wsData.h

#ifndef WSDATA_H
#define WSDATA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "credentials.h"

const int MAX_PACKETS = 5;

struct wsData
{
    int uptime;
    int rssi;
    int memfree;
    int memfrag;
    String ssid;
    String ip;
    String mac;
    String cpu;
    String hostname;
    String desc;
    String resetreason;
    String version;
    time_t boottime;
    time_t timestamp;
    // LoRa
    String packets[MAX_PACKETS];
    // Function to add packets at the first position and shift the rest to the right
    void addPacket(String newPacket)
    {
        for (int i = MAX_PACKETS - 1; i > 0; --i)
        {
            packets[i] = packets[i - 1];
        }
        packets[0] = newPacket;
    }

    String toJson()
    {
        // Create a JSON document
        DynamicJsonDocument doc(3072);

        // Add fields to the document
        doc["uptime"] = uptime;
        doc["rssi"] = rssi;
        doc["memfree"] = memfree;
        doc["memfrag"] = memfrag;
        doc["ssid"] = ssid;
        doc["ip"] = ip;
        doc["mac"] = mac;
        doc["cpu"] = cpu;
        doc["hostname"] = hostname;
        doc["desc"] = desc;
        doc["resetreason"] = resetreason;
        doc["version"] = version;
        doc["boottime"] = boottime;
        doc["timestamp"] = timestamp;

        // Add the packets array to the document
        JsonArray packetsArray = doc.createNestedArray("packets");
        for (int i = 0; i < MAX_PACKETS; ++i)
        {
            packetsArray.add(packets[i]);
        }

        // Serialize the document to a JSON string
        String jsonString;
        serializeJson(doc, jsonString);
        // doc.~BasicJsonDocument(); // destroy

        return jsonString;
    }
};

#endif // WSDATA_H
