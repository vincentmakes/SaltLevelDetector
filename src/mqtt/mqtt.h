#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include "../secrets.h"

#if MQTT_ENABLED
void mqttSetup();
void mqttLoop();
void mqttPublishDistance(float distanceCm);
#else
inline void mqttSetup() {}
inline void mqttLoop() {}
inline void mqttPublishDistance(float) {}
#endif

#endif
