#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

#include "wifi/wifi.h"
#include "mqtt/mqtt.h"
#include "bark/bark.h"
#include "ota/ota.h"

// ---------------------------------------------------------------------------
// Pins
// ---------------------------------------------------------------------------
const int TRIG_PIN = 5;    // GPIO5  -> JSN-SR04T Trig
const int ECHO_PIN = 18;   // GPIO18 -> JSN-SR04T Echo

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
saltlevel::Config gConfig;
saltlevel::OTA    ota;

bool warningSent = false;
unsigned long lastMeasure = 0;
const unsigned long MEASURE_INTERVAL_MS = 3600000UL;  // 1 hour

// ---------------------------------------------------------------------------
// Distance measurement
// ---------------------------------------------------------------------------
float readDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) {
        return -1.0f;
    }

    // speed of sound ~0.0343 cm/us, divide by 2 for round trip
    return (duration * 0.0343f) / 2.0f;
}

// ---------------------------------------------------------------------------
// Fullness calculation
// ---------------------------------------------------------------------------
float computeFullPercent(float distanceCm) {
    if (gConfig.emptyDistanceCm == gConfig.fullDistanceCm) {
        return -1.0f;
    }
    float full  = gConfig.fullDistanceCm;
    float empty = gConfig.emptyDistanceCm;

    float percent = (empty - distanceCm) / (empty - full) * 100.0f;
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println("Salt Level Monitor (WiFi + MQTT + Bark + OTA)");

    // Defaults – these will be overridden by NVS (OTA) if values are stored
    gConfig.fullDistanceCm  = 20.0f;  // hardware/min distance when FULL
    gConfig.emptyDistanceCm = 58.0f;  // EMPTY (bottom)
    gConfig.warnDistanceCm  = 45.0f;
    gConfig.language        = 0;      // 0 = EN, 1 = FR

#ifdef BARK_KEY
    strncpy(gConfig.barkKey, BARK_KEY, sizeof(gConfig.barkKey));
    gConfig.barkKey[sizeof(gConfig.barkKey) - 1] = '\0';
#else
    gConfig.barkKey[0] = '\0';
#endif

    // IMPORTANT: use BARK_ENABLED only as a *default* (runtime),
    // NVS/OTA will override via gConfig.barkEnabled.
#ifdef BARK_ENABLED
    gConfig.barkEnabled = BARK_ENABLED;   // true/false from secrets.h
#else
    gConfig.barkEnabled = false;
#endif

    connectWiFi();

    // Bind config + callbacks BEFORE ota.setup()
    ota.setConfig(&gConfig);
    ota.setDistanceCallback(readDistanceCm);
    ota.setPublishCallback(mqttPublishDistance);
    ota.setup();

    mqttSetup();

    // Initial measurement at boot
    float distance = readDistanceCm();
    if (distance < 0) {
        Serial.println("Distance: out of range / no echo");
    } else {
        Serial.print("Distance at boot: ");
        Serial.print(distance, 2);
        Serial.println(" cm");
    }
    mqttPublishDistance(distance);
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    ota.loop();
    mqttLoop();

    unsigned long now = millis();
    if (now - lastMeasure >= MEASURE_INTERVAL_MS) {
        lastMeasure = now;

        float distance = readDistanceCm();

        if (distance < 0) {
            Serial.println("Distance: out of range / no echo");
        } else {
            Serial.print("Distance: ");
            Serial.print(distance, 2);
            Serial.println(" cm");
        }

        mqttPublishDistance(distance);

        // ---------------- Bark logic: purely runtime, no #if BARK_ENABLED ----------------
        float percent = computeFullPercent(distance);

        if (gConfig.barkEnabled &&                 // from secrets.h default + OTA + NVS
            distance > 0 &&
            gConfig.warnDistanceCm > 0 &&
            percent >= 0.0f) {

            if (!warningSent && distance >= gConfig.warnDistanceCm) {
                Serial.println("Threshold reached -> sending Bark notification...");
                if (barkSendLowSaltNotification(gConfig.barkKey, distance, percent)) {
                    warningSent = true;
                    Serial.println("Bark notification sent, warningSent = true");
                } else {
                    Serial.println("Bark notification failed");
                }
            } else if (warningSent && distance <= (gConfig.warnDistanceCm - 3.0f)) {
                warningSent = false;
                Serial.println("Salt refilled / level ok again, warningSent reset = false");
            }
        }
        // -------------------------------------------------------------------
    }
}
