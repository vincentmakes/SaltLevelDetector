#ifndef BARK_HELPER_H
#define BARK_HELPER_H

#include <Arduino.h>

/**
 * Send a "low salt" notification via Bark
 * 
 * @param barkKey Device key from the Bark app
 * @param distanceCm Current measured distance
 * @param percentFull Computed fullness (0-100), for message text
 * @return true on HTTP 200, false otherwise
 */
bool barkSendLowSaltNotification(const char* barkKey, float distanceCm, float percentFull);

/**
 * Send a custom notification via Bark
 * 
 * @param barkKey Device key from the Bark app
 * @param title Notification title
 * @param message Notification message
 * @return true on HTTP 200, false otherwise
 */
bool barkSendCustomNotification(const char* barkKey, const char* title, const char* message);

#endif // BARK_HELPER_H
