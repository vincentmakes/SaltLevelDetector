#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../secrets.h"
#include "../constants.h"
#include "../logger.h"
#include "mqtt.h"

#if MQTT_ENABLED

static WiFiClient   espClient;
static PubSubClient mqttClient(espClient);
static unsigned long lastReconnectAttempt = 0;

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Logger::debugf("MQTT message received on topic: %s", topic);
    
    // Handle incoming messages if needed
    // For now, this is just a placeholder
    (void)payload;
    (void)length;
}

static bool connectMqtt() {
    if (mqttClient.connected()) {
        return true;
    }

    // Don't retry too frequently
    unsigned long now = millis();
    if (now - lastReconnectAttempt < Timing::MQTT_RECONNECT_DELAY_MS) {
        return false;
    }
    lastReconnectAttempt = now;

    Logger::infof("Connecting to MQTT broker: %s:%d", MQTT_HOST, MQTT_PORT);

    String clientId = "esp32-saltlevel-";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    Logger::debugf("MQTT client ID: %s", clientId.c_str());

    bool connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
    
    if (connected) {
        Logger::info("MQTT connected successfully");
        
        // Subscribe to command topics if needed
        // char topic[Limits::TOPIC_BUFFER_LENGTH];
        // snprintf(topic, sizeof(topic), "%s/command", MQTT_PREFIX);
        // mqttClient.subscribe(topic);
        
        return true;
    } else {
        Logger::errorf("MQTT connection failed, rc=%d", mqttClient.state());
        return false;
    }
}

void mqttSetup() {
    Logger::info("Initializing MQTT...");
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(10);
    
    connectMqtt();
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMqtt();
    }
    
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool mqttPublishDistance(float distanceCm) {
    // Ensure connection
    if (!mqttClient.connected()) {
        Logger::debug("MQTT not connected, attempting reconnect...");
        if (!connectMqtt()) {
            Logger::warn("Cannot publish: MQTT connection failed");
            return false;
        }
    }

    char topic[Limits::TOPIC_BUFFER_LENGTH];
    snprintf(topic, sizeof(topic), "%s/distance_cm", MQTT_PREFIX);

    char payload[32];
    dtostrf(distanceCm, 0, 2, payload);

    // Publish with QoS 0, no retain (for proper history in Home Assistant)
    bool success = mqttClient.publish(topic, payload, false);
    
    if (success) {
        Logger::infof("MQTT published: %s = %s", topic, payload);
    } else {
        Logger::errorf("MQTT publish failed: topic=%s, payload=%s", topic, payload);
    }
    
    return success;
}

bool mqttPublishStatus(const char* status) {
    if (!mqttClient.connected()) {
        if (!connectMqtt()) {
            return false;
        }
    }

    char topic[Limits::TOPIC_BUFFER_LENGTH];
    snprintf(topic, sizeof(topic), "%s/status", MQTT_PREFIX);

    bool success = mqttClient.publish(topic, status, true); // Retained
    
    if (success) {
        Logger::infof("MQTT status published: %s", status);
    } else {
        Logger::error("MQTT status publish failed");
    }
    
    return success;
}

bool isMqttConnected() {
    return mqttClient.connected();
}

#endif // MQTT_ENABLED
