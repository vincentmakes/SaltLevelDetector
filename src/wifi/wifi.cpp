#include <Arduino.h>
#include <WiFi.h>
#include "../secrets.h"
#include "../constants.h"
#include "../logger.h"
#include "wifi.h"

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Logger::debug("WiFi already connected");
        return;
    }

    Logger::infof("Connecting to WiFi: %s", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < Network::WIFI_MAX_CONNECT_ATTEMPTS) {
        delay(Network::WIFI_CONNECT_DELAY_MS);
        Serial.print(".");
        attempt++;
        
        // Log every 5 attempts
        if (attempt % 5 == 0) {
            Logger::debugf("WiFi connection attempt %d/%d", attempt, Network::WIFI_MAX_CONNECT_ATTEMPTS);
        }
    }
    
    Serial.println(); // New line after dots

    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("WiFi connection failed after maximum attempts");
        Logger::warn("Restarting device...");
        delay(1000);
        ESP.restart();
    }

    Logger::info("WiFi connected successfully");
    Logger::infof("IP address: %s", WiFi.localIP().toString().c_str());
    Logger::infof("Signal strength (RSSI): %d dBm", WiFi.RSSI());
    Logger::infof("MAC address: %s", WiFi.macAddress().c_str());
}

void disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("Disconnecting from WiFi...");
        WiFi.disconnect(true);
        delay(100);
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int getWiFiRSSI() {
    return WiFi.RSSI();
}

String getWiFiSSID() {
    return WiFi.SSID();
}

String getLocalIP() {
    return WiFi.localIP().toString();
}
