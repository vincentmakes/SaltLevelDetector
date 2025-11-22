#include <Arduino.h>
#include <WiFi.h>
#include <algorithm>
#include <Preferences.h>
#include "esp_task_wdt.h"
#include "secrets.h"
#include "constants.h"
#include "logger.h"

#include "wifi/wifi.h"
#include "mqtt/mqtt.h"
#include "bark/bark.h"
#include "ota/ota.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
saltlevel::Config gConfig;
saltlevel::OTA    ota;

bool warningSent = false;
unsigned long lastMeasure = 0;
unsigned long lastWifiCheck = 0;

// Reset button state
unsigned long resetButtonPressStart = 0;
bool resetButtonPressed = false;
bool resetInProgress = false;

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
// Bark notification logic
// ---------------------------------------------------------------------------
void handleBarkNotifications(float distance, float percent) {
    if (!gConfig.barkEnabled) {
        return;
    }
    
    if (distance < 0 || gConfig.warnDistanceCm <= 0 || percent < 0.0f) {
        return;
    }
    
    // Check if we should send a warning
    if (!warningSent && distance >= gConfig.warnDistanceCm) {
        Logger::infof("Threshold reached (%.1f >= %.1f), sending Bark notification...",
                     distance, gConfig.warnDistanceCm);
        
        if (barkSendLowSaltNotification(gConfig.barkKey, distance, percent)) {
            warningSent = true;
            Logger::info("Bark notification sent successfully");
        } else {
            Logger::error("Bark notification failed");
        }
    }
    // Check if level has recovered (with hysteresis)
    else if (warningSent && distance <= (gConfig.warnDistanceCm - Sensor::HYSTERESIS_CM)) {
        warningSent = false;
        Logger::infof("Level recovered (%.1f <= %.1f), warning reset",
                     distance, gConfig.warnDistanceCm - Sensor::HYSTERESIS_CM);
    }
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
    Logger::info("Version: 2.1.1 (WiFi Provisioning)");
    Logger::info("Features: WiFi + MQTT + Bark + OTA");
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
    
    // Set default configuration
    gConfig.fullDistanceCm  = Sensor::DEFAULT_FULL_DISTANCE_CM;
    gConfig.emptyDistanceCm = Sensor::DEFAULT_EMPTY_DISTANCE_CM;
    gConfig.warnDistanceCm  = Sensor::DEFAULT_WARN_DISTANCE_CM;
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
            
            // Handle Bark notifications
            handleBarkNotifications(distance, percent);
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