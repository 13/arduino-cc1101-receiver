// wsData.h

#ifndef WSDATA_H
#define WSDATA_H

#include <Arduino.h>
#include <ArduinoJson.h>

const int MAX_PACKETS = 5;

struct wsData {
    int uptime;
    int rssi;
    int memfree;
    int memfrag;
    String ssid;
    String ip;
    String mac;
    String hostname; 
    String reset;
    String version;
    String packets[MAX_PACKETS];

    // Function to add packets at the first position and shift the rest to the right
    void addPacket(String newPacket) {
        for (int i = MAX_PACKETS - 1; i > 0; --i) {
            packets[i] = packets[i - 1];
        }
        packets[0] = newPacket;
    }

    String toJson() {
        // Create a JSON document
        DynamicJsonDocument doc(2048);

        // Add fields to the document
        doc["uptime"] = uptime;
        doc["rssi"] = rssi;
        doc["memfree"] = memfree;
        doc["memfrag"] = memfrag;
        doc["ssid"] = ssid;
        doc["ip"] = ip;
        doc["mac"] = mac;
        doc["hostname"] = hostname;
        doc["reset"] = reset;
        doc["version"] = version;

        // Add the packets array to the document
        JsonArray packetsArray = doc.createNestedArray("packets");
        for (int i = 0; i < MAX_PACKETS; ++i) {
            packetsArray.add(packets[i]);
        }

        // Serialize the document to a JSON string
        String jsonString;
        serializeJson(doc, jsonString);

        return jsonString;
    }
};

#endif // WSDATA_H
