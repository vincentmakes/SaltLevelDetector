#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../secrets.h"
#include "mqtt.h"

#if MQTT_ENABLED

static WiFiClient   espClient;
static PubSubClient mqttClient(espClient);

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    (void)topic;
    (void)payload;
    (void)length;
}

static void connectMqtt() {
    if (mqttClient.connected()) {
        return;
    }

    Serial.print("Connecting to MQTT broker: ");
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    String clientId = "esp32-saltlevel-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection... ");
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" - retrying in 5 seconds");
            delay(5000);
        }
    }
}

void mqttSetup() {
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectMqtt();
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMqtt();
    }
    mqttClient.loop();
}

void mqttPublishDistance(float distanceCm) {
    if (!mqttClient.connected()) return;

    char topic[128];
    snprintf(topic, sizeof(topic), "%s/distance_cm", MQTT_PREFIX);

    char payload[32];
    dtostrf(distanceCm, 0, 2, payload);

    // retain = false so HA history works as expected
    bool ok = mqttClient.publish(topic, payload, false);
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
