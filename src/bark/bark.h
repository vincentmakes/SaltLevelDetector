#ifndef BARK_HELPER_H
#define BARK_HELPER_H

#include <Arduino.h>

// Send a "low salt" notification via Bark.
// - barkKey: device key from the Bark app
// - distanceCm: current measured distance
// - percentFull: computed fullness (0–100), for nice message text
//
// Returns true on HTTP 200, false otherwise.
bool barkSendLowSaltNotification(const char* barkKey, float distanceCm, float percentFull);

#endif
