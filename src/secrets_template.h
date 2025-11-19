
// ============================================================================
// WiFi Configuration
// ============================================================================
#define WIFI_SSID   "yourssid"
#define WIFI_PASS   "wifipassword"


// ============================================================================
// MQTT Configuration
// ============================================================================

// Set to false to disable MQTT altogether. MQTT is used for integration with 
// home automation systems such as Home Assistant
#define MQTT_ENABLED true

#define MQTT_HOST   "192.168.1.XXX"
#define MQTT_PORT   1883
#define MQTT_USER   "mqtt_user"
#define MQTT_PASS   "mqtt_password"
#define MQTT_PREFIX "salt_level"


// ============================================================================
// Bark Configuration
// ============================================================================

// Bark can be used as an alternative to get notifications instead of Home Assistant.
// Set MQTT_ENABLED to false if Home Assistant is not needed

#define BARK_ENABLED false

// Install Bark on your mobile and copy this from the Bark app (the "key" part of the URL)
#define BARK_KEY    "your_bark_device_key"

// Usually you don't need to change this:
#define BARK_SERVER "https://api.day.app"


// ============================================================================
// OTA Configuration
// ============================================================================

// IMPORTANT: Set a strong password for OTA updates to prevent unauthorized access
// This password is required when uploading firmware via the web interface
#define OTA_PASSWORD "secure_password"


// ============================================================================
// Debug Configuration
// ============================================================================

// Uncomment to enable debug logging (increases verbosity)
// #define DEBUG_MODE

