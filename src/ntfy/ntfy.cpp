#include "ntfy.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "../logger.h"

#ifndef NTFY_SERVER
// Default ntfy server
#define NTFY_SERVER "https://ntfy.sh"
#endif

/**
 * Generate a unique ntfy topic based on ESP32 MAC address
 * Format: "saltlevelmonitor-XXXXXX" where XXXXXX is last 6 hex digits of MAC
 */
String generateNtfyTopic() {
    uint64_t macAddress = ESP.getEfuseMac();
    uint32_t uniqueId = (uint32_t)(macAddress & 0xFFFFFF);
    
    char hexStr[7];
    snprintf(hexStr, sizeof(hexStr), "%06x", uniqueId);
    
    return "saltlevelmonitor-" + String(hexStr);
}

bool ntfySendLowSaltNotification(const char* topic, float distanceCm, float percentFull) {
    if (!topic || topic[0] == '\0') {
        Logger::debug("ntfy: topic not set, skipping notification");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("ntfy: WiFi not connected");
        return false;
    }

    // Build notification
    String url = String(NTFY_SERVER) + "/" + topic;
    String title = "Salt Level Low";
    String message = "Distance " + String(distanceCm, 1) + "cm (" + 
                     String(percentFull, 0) + "% full)";

    Logger::infof("ntfy URL: %s", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification

    HTTPClient https;
    if (!https.begin(client, url)) {
        Logger::error("ntfy: HTTPS begin failed");
        return false;
    }

    // Set headers for better notifications
    https.addHeader("Title", title);
    https.addHeader("Priority", "high");
    https.addHeader("Tags", "droplet,warning");
    
    https.setTimeout(10000); // 10 seconds

    int httpCode = https.POST(message);
    
    if (httpCode > 0) {
        Logger::infof("ntfy HTTP status: %d", httpCode);
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = https.getString();
            Logger::debugf("ntfy response: %s", payload.c_str());
            https.end();
            return true;
        } else {
            String payload = https.getString();
            Logger::warnf("ntfy unexpected response code: %d, body: %s", httpCode, payload.c_str());
        }
    } else {
        Logger::errorf("ntfy request failed: %s", https.errorToString(httpCode).c_str());
    }

    https.end();
    return false;
}

bool ntfySendCustomNotification(const char* topic, const char* title, const char* message) {
    if (!topic || topic[0] == '\0') {
        Logger::debug("ntfy: topic not set, skipping notification");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("ntfy: WiFi not connected");
        return false;
    }

    String url = String(NTFY_SERVER) + "/" + topic;

    Logger::debugf("ntfy custom notification URL: %s", url.c_str());

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    if (!https.begin(client, url)) {
        Logger::error("ntfy: HTTPS begin failed");
        return false;
    }

    https.addHeader("Title", title);
    https.setTimeout(10000);
    
    int httpCode = https.POST(message);
    
    bool success = (httpCode == HTTP_CODE_OK);
    
    if (success) {
        Logger::info("ntfy custom notification sent successfully");
    } else {
        Logger::errorf("ntfy custom notification failed: HTTP %d", httpCode);
    }
    
    https.end();
    return success;
}