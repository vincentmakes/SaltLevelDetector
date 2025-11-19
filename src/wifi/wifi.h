#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>

// Connect to WiFi network
void connectWiFi();

// Disconnect from WiFi
void disconnectWiFi();

// Check if WiFi is connected
bool isWiFiConnected();

// Get WiFi signal strength
int getWiFiRSSI();

// Get connected SSID
String getWiFiSSID();

// Get local IP address
String getLocalIP();

#endif // WIFI_HELPER_H
