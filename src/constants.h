#ifndef CONSTANTS_H
#define CONSTANTS_H

// Hardware pin definitions
namespace Pins {
    constexpr int TRIG = 5;    // GPIO5  -> JSN-SR04T Trig
    constexpr int ECHO = 18;   // GPIO18 -> JSN-SR04T Echo
}

// Timing constants
namespace Timing {
    constexpr unsigned long MEASURE_INTERVAL_MS = 3600000UL;     // 1 hour
    constexpr unsigned long SENSOR_TIMEOUT_US = 30000;           // 30ms timeout
    constexpr unsigned long WIFI_CHECK_INTERVAL_MS = 300000UL;   // 5 minutes
    constexpr unsigned long MQTT_RECONNECT_DELAY_MS = 5000;      // 5 seconds
    constexpr unsigned long SENSOR_READING_DELAY_MS = 50;        // Between multiple readings
}

// Sensor configuration constants
namespace Sensor {
    constexpr float SOUND_SPEED_CM_PER_US = 0.0343f;  // Speed of sound in air
    constexpr float HYSTERESIS_CM = 3.0f;              // Hysteresis for warning reset
    constexpr float DEFAULT_FULL_DISTANCE_CM = 20.0f;  // Hardware minimum (tank full)
    constexpr float DEFAULT_EMPTY_DISTANCE_CM = 58.0f; // Maximum depth (tank empty)
    constexpr float DEFAULT_WARN_DISTANCE_CM = 45.0f;  // Warning threshold
    constexpr int MAX_READING_ATTEMPTS = 3;            // Multiple readings for accuracy
    constexpr float MAX_READING_VARIANCE_CM = 5.0f;    // Maximum acceptable variance
}

// Network constants
namespace Network {
    constexpr int WIFI_MAX_CONNECT_ATTEMPTS = 20;
    constexpr int WIFI_CONNECT_DELAY_MS = 500;
    constexpr int HTTP_PORT = 80;
    constexpr int WATCHDOG_TIMEOUT_SECONDS = 10;
}

// String length limits
namespace Limits {
    constexpr size_t BARK_KEY_LENGTH = 128;
    constexpr size_t OTA_PASSWORD_LENGTH = 64;
    constexpr size_t TOPIC_BUFFER_LENGTH = 128;
    constexpr size_t JSON_BUFFER_LENGTH = 512;
}

#endif // CONSTANTS_H
