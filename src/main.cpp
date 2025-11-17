#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "ota/ota.h"

// ---------------------------------------------------------------------------
// Pins
// ---------------------------------------------------------------------------
const int TRIG_PIN = 5;    // GPIO5  -> JSN-SR04T Trig
const int ECHO_PIN = 18;   // GPIO18 -> JSN-SR04T Echo

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
WiFiClient espClient;

#if MQTT_ENABLED
PubSubClient mqttClient(espClient);
#endif

saltlevel::OTA ota;   // our OTA helper

unsigned long lastMeasure = 0;
const unsigned long MEASURE_INTERVAL_MS = 360000;

// ---------------------------------------------------------------------------
// Distance measurement
// ---------------------------------------------------------------------------
float readDistanceCm() {
    // Ensure trigger is LOW
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // 10 µs HIGH pulse to trigger the measurement
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Measure the length of the echo pulse (in microseconds)
    // Timeout ~30 ms (30000 µs) ~ max distance ~5 m
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0) {
        // No echo received (out of range or sensor error)
        return -1.0f;
    }

    // Speed of sound ~343 m/s => 0.0343 cm/µs
    // Distance (cm) = (duration_us * 0.0343) / 2
    float distanceCm = (duration * 0.0343f) / 2.0f;
    return distanceCm;
}

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
}

// ---------------------------------------------------------------------------
// MQTT
// ---------------------------------------------------------------------------
#if MQTT_ENABLED

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // No incoming commands handled for now
    (void)topic;
    (void)payload;
    (void)length;
}

void connectMqtt() {
    if (mqttClient.connected()) {
        return;
    }

    Serial.print("Connecting to MQTT broker: ");
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    // Unique-ish client ID based on MAC
    String clientId = "esp32-saltlevel-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection... ");
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("connected");
            // Add subscriptions here if needed
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" - retrying in 5 seconds");
            delay(5000);
        }
    }
}

void publishDistance(float distanceCm) {
    if (!mqttClient.connected()) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/distance_cm", MQTT_PREFIX);

    char payload[32];
    // If distance < 0, publish -1.00
    dtostrf(distanceCm, 0, 2, payload);

    bool ok = mqttClient.publish(topic, payload, true); // retained
    if (!ok) {
        Serial.println("MQTT publish failed");
    } else {
        Serial.print("MQTT publish -> ");
        Serial.print(topic);
        Serial.print(" = ");
        Serial.println(payload);
    }
}

#endif // MQTT_ENABLED

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println("JSN-SR04T distance measurement with WiFi + MQTT");

    connectWiFi();
    ota.setup();

#if MQTT_ENABLED
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectMqtt();
#endif

        float distance = readDistanceCm();

        if (distance < 0) {
            Serial.println("Distance: out of range / no echo");
        } else {
            Serial.print("Distance: ");
            Serial.print(distance, 2);
            Serial.println(" cm");
        }

#if MQTT_ENABLED
        publishDistance(distance);
#endif

}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void loop() {
    // Keep WiFi connected
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    ota.loop();

#if MQTT_ENABLED
    // Keep MQTT connected
    if (!mqttClient.connected()) {
        connectMqtt();
    }
    mqttClient.loop();
#endif

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

#if MQTT_ENABLED
        publishDistance(distance);
#endif
    }
}
