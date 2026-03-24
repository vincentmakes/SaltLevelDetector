#include "bark.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "../secrets.h"
#include "../logger.h"

#ifndef BARK_SERVER
// Default Bark server if not defined in secrets.h
#define BARK_SERVER "https://api.day.app"
#endif

bool barkSendLowSaltNotification(const char* barkKey, float distanceCm, float percentFull) {
    // Validate inputs
    if (!barkKey || barkKey[0] == '\0') {
        Logger::debug("Bark: key not set, skipping notification");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("Bark: WiFi not connected");
        return false;
    }

    // Build notification URL
    // Example: https://api.day.app/<key>/Salt%20Level%20Low/Distance%2045.0cm%20(30%25%20full)
    String url = String(BARK_SERVER) + "/" + barkKey +
                 "/Salt%20Level%20Low/" +
                 "Distance%20" + String(distanceCm, 1) + "cm%20(" +
                 String(percentFull, 0) + "%25%20full)";

    Logger::infof("Bark URL: %s", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification (acceptable for home LAN)

    HTTPClient https;
    if (!https.begin(client, url)) {
        Logger::error("Bark: HTTPS begin failed");
        return false;
    }

    // Set timeout
    https.setTimeout(10000); // 10 seconds

    int httpCode = https.GET();
    
    if (httpCode > 0) {
        Logger::infof("Bark HTTP status: %d", httpCode);
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = https.getString();
            Logger::debugf("Bark response: %s", payload.c_str());
            https.end();
            return true;
        } else {
            String payload = https.getString();
            Logger::warnf("Bark unexpected response code: %d, body: %s", httpCode, payload.c_str());
        }
    } else {
        Logger::errorf("Bark request failed: %s", https.errorToString(httpCode).c_str());
    }

    https.end();
    return false;
}

bool barkSendCustomNotification(const char* barkKey, const char* title, const char* message) {
    if (!barkKey || barkKey[0] == '\0') {
        Logger::debug("Bark: key not set, skipping notification");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("Bark: WiFi not connected");
        return false;
    }

    // URL encode the title and message
    String encodedTitle = String(title);
    encodedTitle.replace(" ", "%20");
    
    String encodedMessage = String(message);
    encodedMessage.replace(" ", "%20");

    String url = String(BARK_SERVER) + "/" + barkKey + 
                 "/" + encodedTitle + "/" + encodedMessage;

    Logger::debugf("Bark custom notification URL: %s", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    if (!https.begin(client, url)) {
        Logger::error("Bark: HTTPS begin failed");
        return false;
    }

    https.setTimeout(10000);
    int httpCode = https.GET();
    
    bool success = (httpCode == HTTP_CODE_OK);
    
    if (success) {
        Logger::info("Bark custom notification sent successfully");
    } else {
        Logger::errorf("Bark custom notification failed: HTTP %d", httpCode);
    }
    
    https.end();
    return success;
}
