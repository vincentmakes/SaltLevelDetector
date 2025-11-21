#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>

// Structure to hold WiFi credentials
struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool isValid;
};

// Initialize WiFi - returns true if connected, false if provisioning needed
bool initWiFi();

// Connect to WiFi network with stored or provided credentials
void connectWiFi();

// Start WiFi provisioning mode (Soft AP)
void startWiFiProvisioning();

// Check if WiFi provisioning is needed (empty credentials)
bool isWiFiProvisioningNeeded();

// Load WiFi credentials from NVS
WiFiCredentials loadWiFiCredentials();

// Save WiFi credentials to NVS
bool saveWiFiCredentials(const char* ssid, const char* password);

// Clear WiFi credentials from NVS
void clearWiFiCredentials();

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