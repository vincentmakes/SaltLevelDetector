#include <Arduino.h>
#include <WiFi.h>
#include <algorithm>
#include <Preferences.h>
#include "esp_task_wdt.h"
#include "secrets.h_tmp"
#include "constants.h"
#include "logger.h"

#include "wifi/wifi.h"
#include "mqtt/mqtt.h"
#include "bark/bark.h"
#include "ntfy/ntfy.h"
#include "ota/ota.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
saltlevel::Config gConfig;
saltlevel::OTA    ota;

bool warningSent = false;
unsigned long lastMeasure = 0;
unsigned long lastWifiCheck = 0;

// Consecutive low-level tracking for notification filtering
uint8_t consecutiveLowReadings = 0;
uint8_t consecutiveHighReadings = 0;  // For reset after recovery

// Reset button state
unsigned long resetButtonPressStart = 0;
bool resetButtonPressed = false;
bool resetInProgress = false;

// ---------------------------------------------------------------------------
// Persistence for notification state (survives reboot)
// ---------------------------------------------------------------------------
static Preferences notificationPrefs;

void loadNotificationState() {
    notificationPrefs.begin("notify", true);  // Read-only
    consecutiveLowReadings = notificationPrefs.getUChar("consec_low", 0);
    consecutiveHighReadings = notificationPrefs.getUChar("consec_high", 0);
    warningSent = notificationPrefs.getBool("warn_sent", false);
    notificationPrefs.end();
    
    Logger::infof("Notification state loaded: low=%u, high=%u, warningSent=%s",
                 consecutiveLowReadings, consecutiveHighReadings,
                 warningSent ? "true" : "false");
}

void saveNotificationState() {
    notificationPrefs.begin("notify", false);  // Read-write
    notificationPrefs.putUChar("consec_low", consecutiveLowReadings);
    notificationPrefs.putUChar("consec_high", consecutiveHighReadings);
    notificationPrefs.putBool("warn_sent", warningSent);
    notificationPrefs.end();
    
    Logger::debugf("Notification state saved: low=%u, high=%u, warningSent=%s",
                  consecutiveLowReadings, consecutiveHighReadings,
                  warningSent ? "true" : "false");
}

// ---------------------------------------------------------------------------
// Reset Button Handler
// ---------------------------------------------------------------------------
void checkResetButton() {
    int buttonState = digitalRead(Pins::RESET_BTN);
    
    // Button is pressed (LOW on ESP32 boot button)
    if (buttonState == LOW) {
        if (!resetButtonPressed) {
            // Button just pressed
            resetButtonPressed = true;
            resetButtonPressStart = millis();
            Logger::info("Reset button pressed - hold for 5 seconds to clear settings");
        } else {
            // Button held down - check duration
            unsigned long holdDuration = millis() - resetButtonPressStart;
            
            if (holdDuration >= Timing::RESET_BUTTON_HOLD_MS && !resetInProgress) {
                resetInProgress = true;
                Logger::warn("===========================================");
                Logger::warn("RESET BUTTON HELD - CLEARING ALL SETTINGS");
                Logger::warn("===========================================");
                
                // Clear WiFi credentials
                clearWiFiCredentials();
                Logger::info("WiFi credentials cleared");
                
                // Clear application settings
                Preferences prefs;
                prefs.begin("saltcfg", false);
                prefs.clear();
                prefs.end();
                Logger::info("Application settings cleared");
                
                // Clear notification state
                notificationPrefs.begin("notify", false);
                notificationPrefs.clear();
                notificationPrefs.end();
                Logger::info("Notification state cleared");
                
                Logger::warn("All settings cleared. Device will restart in 2 seconds...");
                delay(2000);
                ESP.restart();
            }
        }
    } else {
        // Button released
        if (resetButtonPressed) {
            unsigned long holdDuration = millis() - resetButtonPressStart;
            if (holdDuration < Timing::RESET_BUTTON_HOLD_MS) {
                Logger::debug("Reset button released (not held long enough)");
            }
            resetButtonPressed = false;
            resetInProgress = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Distance measurement with median filtering
// ---------------------------------------------------------------------------
float readDistanceCm() {
    const int attempts = Sensor::MAX_READING_ATTEMPTS;
    float readings[attempts];
    int validReadings = 0;
    
    for (int i = 0; i < attempts; i++) {
        digitalWrite(Pins::TRIG, LOW);
        delayMicroseconds(2);
        
        digitalWrite(Pins::TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(Pins::TRIG, LOW);
        
        unsigned long duration = pulseIn(Pins::ECHO, HIGH, Timing::SENSOR_TIMEOUT_US);
        
        if (duration > 0) {
            float distance = (duration * Sensor::SOUND_SPEED_CM_PER_US) / 2.0f;
            readings[validReadings++] = distance;
            Logger::debugf("Reading %d: %.2f cm", i + 1, distance);
        } else {
            Logger::debug("Reading timeout");
        }
        
        if (i < attempts - 1) {
            delay(Timing::SENSOR_READING_DELAY_MS);
        }
    }
    
    if (validReadings == 0) {
        Logger::warn("All sensor readings failed");
        return -1.0f;
    }
    
    if (validReadings == 1) {
        Logger::infof("Single valid reading: %.2f cm", readings[0]);
        return readings[0];
    }
    
    // Calculate median for robustness
    std::sort(readings, readings + validReadings);
    float median = readings[validReadings / 2];
    
    Logger::infof("Median of %d readings: %.2f cm", validReadings, median);
    return median;
}

// ---------------------------------------------------------------------------
// Level calculation
// ---------------------------------------------------------------------------
float computeFullPercent(float distanceCm) {
    if (gConfig.emptyDistanceCm == gConfig.fullDistanceCm) {
        Logger::error("Invalid config: empty == full distance");
        return -1.0f;
    }
    
    float full  = gConfig.fullDistanceCm;
    float empty = gConfig.emptyDistanceCm;
    
    float percent = (empty - distanceCm) / (empty - full) * 100.0f;
    
    // Clamp to valid range
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    
    return percent;
}

// ---------------------------------------------------------------------------
// Notification logic (both Bark and ntfy) with consecutive hours filter
// 
// Distance interpretation:
//   - Higher distance = less salt = LOW level = needs refill
//   - Lower distance = more salt = OK level = tank refilled
//
// Notification sent after N consecutive hours with distance >= threshold (low salt)
// Notification reset after N consecutive hours with distance < threshold (refilled)
// ---------------------------------------------------------------------------
void handleNotifications(float distance, float percent) {
    if (distance < 0 || gConfig.warnDistanceCm <= 0 || percent < 0.0f) {
        return;
    }
    
    // Check if salt level is low (distance >= threshold means less salt)
    bool isLowLevel = (distance >= gConfig.warnDistanceCm);
    
    if (isLowLevel) {
        // Reset high counter when level is low
        if (consecutiveHighReadings > 0) {
            Logger::debugf("Level dropped again, resetting high counter from %u to 0",
                          consecutiveHighReadings);
            consecutiveHighReadings = 0;
        }
        
        // Increment consecutive low readings counter
        if (consecutiveLowReadings < 255) {
            consecutiveLowReadings++;
        }
        
        Logger::infof("Low level detected (%.1f cm >= %.1f cm). Consecutive low: %u/%u",
                     distance, gConfig.warnDistanceCm,
                     consecutiveLowReadings, gConfig.consecutiveHoursThreshold);
        
        // Check if we should send a warning (consecutive hours threshold met)
        if (!warningSent && consecutiveLowReadings >= gConfig.consecutiveHoursThreshold) {
            Logger::infof("Threshold met for %u consecutive hours, sending notifications...",
                         consecutiveLowReadings);
            
            // Try Bark (iOS)
            if (gConfig.barkEnabled && gConfig.barkKey[0] != '\0') {
                if (barkSendLowSaltNotification(gConfig.barkKey, distance, percent)) {
                    Logger::info("Bark notification sent successfully");
                } else {
                    Logger::error("Bark notification failed");
                }
            }
            
            // Try ntfy (Android/Multi-platform)
            if (gConfig.ntfyEnabled && gConfig.ntfyTopic[0] != '\0') {
                if (ntfySendLowSaltNotification(gConfig.ntfyTopic, distance, percent)) {
                    Logger::info("ntfy notification sent successfully");
                } else {
                    Logger::error("ntfy notification failed");
                }
            }
            
            // Set warning flag if any notification was enabled
            if (gConfig.barkEnabled || gConfig.ntfyEnabled) {
                warningSent = true;
            }
        } else if (!warningSent) {
            Logger::infof("Waiting for %u more consecutive low readings before notification",
                         gConfig.consecutiveHoursThreshold - consecutiveLowReadings);
        }
    } else {
        // Salt level is OK (distance < threshold means tank was refilled)
        
        // Reset low counter when salt level is OK
        if (consecutiveLowReadings > 0) {
            Logger::debugf("Salt refilled, resetting low counter from %u to 0",
                          consecutiveLowReadings);
            consecutiveLowReadings = 0;
        }
        
        // Increment consecutive OK readings counter
        if (consecutiveHighReadings < 255) {
            consecutiveHighReadings++;
        }
        
        Logger::infof("Salt OK (%.1f cm < %.1f cm threshold). Consecutive OK: %u/%u",
                     distance, gConfig.warnDistanceCm,
                     consecutiveHighReadings, gConfig.consecutiveHoursThreshold);
        
        // Reset warning only after N consecutive hours with OK level
        if (warningSent && consecutiveHighReadings >= gConfig.consecutiveHoursThreshold) {
            warningSent = false;
            Logger::infof("Salt OK for %u consecutive hours, warning reset - ready for new alerts",
                         consecutiveHighReadings);
        } else if (warningSent) {
            Logger::infof("Waiting for %u more consecutive OK readings before reset",
                         gConfig.consecutiveHoursThreshold - consecutiveHighReadings);
        }
    }
    
    // Save state to survive reboot
    saveNotificationState();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    // Initialize logger first
    Logger::init(115200);
    
#ifdef DEBUG_MODE
    Logger::setLevel(LogLevel::DEBUG);
#else
    Logger::setLevel(LogLevel::INFO);
#endif
    
    Logger::info("===========================================");
    Logger::info("Salt Level Monitor");
    Logger::info("Version: 2.3.0 (Consecutive Hours Filter)");
    Logger::info("Features: WiFi + MQTT + Bark + ntfy + OTA");
    Logger::info("Reset: Hold BOOT button for 5s to clear");
    Logger::info("===========================================");
    
    // Initialize watchdog
    Logger::info("Enabling watchdog timer...");
    esp_task_wdt_init(Network::WATCHDOG_TIMEOUT_SECONDS, true);
    esp_task_wdt_add(NULL);
    
    // Initialize hardware pins
    pinMode(Pins::TRIG, OUTPUT);
    pinMode(Pins::ECHO, INPUT);
    pinMode(Pins::RESET_BTN, INPUT_PULLUP);  // Use internal pull-up
    Logger::info("GPIO pins configured");
    
    // Load notification state from NVS (survives reboot)
    loadNotificationState();
    
    // Set default configuration
    gConfig.fullDistanceCm  = Sensor::DEFAULT_FULL_DISTANCE_CM;
    gConfig.emptyDistanceCm = Sensor::DEFAULT_EMPTY_DISTANCE_CM;
    gConfig.warnDistanceCm  = Sensor::DEFAULT_WARN_DISTANCE_CM;
    gConfig.consecutiveHoursThreshold = Notification::CONSECUTIVE_LOW_THRESHOLD;
    gConfig.language        = saltlevel::Language::ENGLISH;
    
    // Set OTA password from secrets.h or default
#ifdef OTA_PASSWORD
    strncpy(gConfig.otaPassword, OTA_PASSWORD, sizeof(gConfig.otaPassword));
    gConfig.otaPassword[sizeof(gConfig.otaPassword) - 1] = '\0';
#else
    strncpy(gConfig.otaPassword, "admin", sizeof(gConfig.otaPassword));
    Logger::warn("Using default OTA password - please change in secrets.h");
#endif
    
    // Set Bark key from secrets.h
#ifdef BARK_KEY
    strncpy(gConfig.barkKey, BARK_KEY, sizeof(gConfig.barkKey));
    gConfig.barkKey[sizeof(gConfig.barkKey) - 1] = '\0';
#else
    gConfig.barkKey[0] = '\0';
#endif
    
    // Set Bark enabled flag
#ifdef BARK_ENABLED
    gConfig.barkEnabled = BARK_ENABLED;
#else
    gConfig.barkEnabled = false;
#endif
    
    // Initialize ntfy topic (will be loaded from NVS if exists)
    gConfig.ntfyTopic[0] = '\0';
    gConfig.ntfyEnabled = false;
    
    Logger::infof("Bark notifications: %s", gConfig.barkEnabled ? "ENABLED" : "DISABLED");
    
    // Initialize WiFi (with provisioning if needed)
    if (!initWiFi()) {
        // Device will restart after provisioning
        Logger::info("Waiting for WiFi provisioning...");
        while (true) {
            delay(1000);
        }
    }
    
    // Initialize OTA (will load config from NVS, overriding defaults)
    ota.setConfig(&gConfig);
    ota.setDistanceCallback(readDistanceCm);
    ota.setPublishCallback(mqttPublishDistance);
    ota.setup();
    
    
    Logger::infof("ntfy notifications: %s (topic: %s)", 
                 gConfig.ntfyEnabled ? "ENABLED" : "DISABLED",
                 gConfig.ntfyTopic);
    
    Logger::infof("Configuration loaded - Tank: %.1f-%.1f cm, Warn: %.1f cm",
                 gConfig.fullDistanceCm, gConfig.emptyDistanceCm, gConfig.warnDistanceCm);
    
    // Initialize MQTT
    mqttSetup();
    
    // Initial measurement at boot
    Logger::info("Performing initial measurement...");
    float distance = readDistanceCm();
    
    if (distance < 0) {
        Logger::error("Initial measurement failed: out of range / no echo");
    } else {
        float percent = computeFullPercent(distance);
        Logger::infof("Initial reading: %.2f cm (%.1f%% full)", distance, percent);
    }
    
    mqttPublishDistance(distance);
    Logger::info("Setup complete - entering main loop");
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
    // Reset watchdog timer
    esp_task_wdt_reset();
    
    // Check reset button (hold for 5 seconds to clear all settings)
    checkResetButton();
    
    // Periodic WiFi check
    // NOTE: This timing pattern is overflow-safe. When millis() overflows after
    // ~49 days, unsigned arithmetic wraps correctly: if now=10 and last=4294967290,
    // then (now - last) = (10 - 4294967290) = 4294967296 = wraps to correct value
    unsigned long now = millis();
    if (now - lastWifiCheck >= Timing::WIFI_CHECK_INTERVAL_MS) {
        lastWifiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Logger::warn("WiFi disconnected, reconnecting...");
            connectWiFi();
        }
    }
    
    // Handle OTA and MQTT
    ota.loop();
    mqttLoop();
    
    // Periodic measurement
    if (now - lastMeasure >= Timing::MEASURE_INTERVAL_MS) {
        lastMeasure = now;
        
        Logger::info("--- Periodic measurement ---");
        float distance = readDistanceCm();
        
        if (distance < 0) {
            Logger::error("Measurement failed: out of range / no echo");
        } else {
            float percent = computeFullPercent(distance);
            Logger::infof("Distance: %.2f cm, Level: %.1f%%", distance, percent);
            
            // Handle notifications (Bark and ntfy)
            handleNotifications(distance, percent);
        }
        
        // Publish to MQTT
        if (mqttPublishDistance(distance)) {
            Logger::debug("MQTT publish successful");
        } else {
            Logger::warn("MQTT publish failed");
        }
    }
    
    // Small delay to prevent tight loop
    delay(10);
}