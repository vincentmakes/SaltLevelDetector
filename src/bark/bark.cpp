#include "bark.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "../secrets.h"

#ifndef BARK_SERVER
// Default Bark server if not defined in secrets.h
#define BARK_SERVER "https://api.day.app"
#endif

bool barkSendLowSaltNotification(const char* barkKey, float distanceCm, float percentFull) {
    if (!barkKey || barkKey[0] == '\0') {
        Serial.println("Bark: key not set, skipping notification");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Bark: WiFi not connected");
        return false;
    }

    // Example:
    // https://api.day.app/<key>/Salt%20Level%20Low/Distance%2045.0cm%20(30%25%20full)
    String url = String(BARK_SERVER) + "/" + barkKey +
                 "/Salt%20Level%20Low/" +
                 "Distance%20" + String(distanceCm, 1) + "cm%20(" +
                 String(percentFull, 0) + "%25%20full)";

    Serial.print("Bark URL: ");
    Serial.println(url);

    WiFiClientSecure client;
    client.setInsecure();  // skip certificate verification (fine on home LAN)

    HTTPClient https;
    if (!https.begin(client, url)) {
        Serial.println("Bark: HTTPS begin failed");
        return false;
    }

    int httpCode = https.GET();
    if (httpCode > 0) {
        Serial.print("Bark HTTP status: ");
        Serial.println(httpCode);
        String payload = https.getString();
        Serial.print("Bark response: ");
        Serial.println(payload);
    } else {
        Serial.print("Bark request failed: ");
        Serial.println(https.errorToString(httpCode));
    }

    https.end();
    return (httpCode == 200);
}
