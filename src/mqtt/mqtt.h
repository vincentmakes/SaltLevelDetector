#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include "../secrets.h"

#if MQTT_ENABLED

// Initialize MQTT connection
void mqttSetup();

// Handle MQTT loop (reconnect if needed)
void mqttLoop();

// Publish distance measurement to MQTT
bool mqttPublishDistance(float distanceCm);

// Publish status message to MQTT
bool mqttPublishStatus(const char* status);

// Check if MQTT is connected
bool isMqttConnected();

#else

// Stub implementations when MQTT is disabled
inline void mqttSetup() {}
inline void mqttLoop() {}
inline bool mqttPublishDistance(float) { return false; }
inline bool mqttPublishStatus(const char*) { return false; }
inline bool isMqttConnected() { return false; }

#endif // MQTT_ENABLED

#endif // MQTT_HELPER_H
