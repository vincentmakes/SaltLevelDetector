
// ============================================================================
// Wifi Configuration
// ============================================================================
#define WIFI_SSID   "yourssid"
#define WIFI_PASS   "wifipassword"


// ============================================================================
// MQTT Configuration
// ============================================================================

// set to false to disable MQTT altogether. MQTT is used for integration with 
// home automation system such as Home Assistant
#define MQTT_ENABLED true

#define MQTT_HOST   "192.168.1.XXX"
#define MQTT_PORT   1883
#define MQTT_USER   "mqtt_user"
#define MQTT_PASS   "mqtt_user"
#define MQTT_PREFIX "salt_level"
