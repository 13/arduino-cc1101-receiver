// wsData.h

#ifndef WSDATA_H
#define WSDATA_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../src/credentials.h"

#ifdef S_PARKING2
const int MAX_CARS = 2;
#endif
#ifdef S_CC1101
const int MAX_PACKETS = 5;
#endif

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
#ifdef S_PARKING2
    // parking sensor
    boolean cars[MAX_CARS];
    int distances[MAX_CARS];
    boolean garageFull;
#endif
#ifdef S_CC1101
    // cc1101
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
#endif

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
#ifdef S_PARKING2
        // parking sensor
        doc["garageFull"] = garageFull;
        // Add the packets array to the document
        JsonArray carsArray = doc.createNestedArray("cars");
        for (int i = 0; i < MAX_CARS; ++i)
        {
            carsArray.add(cars[i]);
        }
        JsonArray distancesArray = doc.createNestedArray("distances");
        for (int i = 0; i < MAX_CARS; ++i)
        {
            distancesArray.add(distances[i]);
        }
#endif
#ifdef S_CC1101
        // Add the packets array to the document
        JsonArray packetsArray = doc.createNestedArray("packets");
        for (int i = 0; i < MAX_PACKETS; ++i)
        {
            packetsArray.add(packets[i]);
        }
#endif
        // Serialize the document to a JSON string
        String jsonString;
        serializeJson(doc, jsonString);
        // doc.~BasicJsonDocument(); // destroy

        return jsonString;
    }
};

#endif // WSDATA_H
