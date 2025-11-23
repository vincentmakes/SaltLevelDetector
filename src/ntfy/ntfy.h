#ifndef NTFY_HELPER_H
#define NTFY_HELPER_H

#include <Arduino.h>

/**
 * Generate a unique ntfy topic name based on ESP32 MAC address
 * Format: "saltlevelmonitor-XXXXXX" where XXXXXX is last 6 hex digits of MAC
 * 
 * @return String containing the unique topic name
 */
String generateNtfyTopic();

/**
 * Send a "low salt" notification via ntfy
 * 
 * @param topic ntfy topic name (auto-generated from MAC address)
 * @param distanceCm Current measured distance
 * @param percentFull Computed fullness (0-100), for message text
 * @return true on HTTP 200, false otherwise
 */
bool ntfySendLowSaltNotification(const char* topic, float distanceCm, float percentFull);

/**
 * Send a custom notification via ntfy
 * 
 * @param topic ntfy topic name
 * @param title Notification title
 * @param message Notification message
 * @return true on HTTP 200, false otherwise
 */
bool ntfySendCustomNotification(const char* topic, const char* title, const char* message);

#endif // NTFY_HELPER_H