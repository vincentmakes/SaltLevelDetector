
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

// ============================================================================
// Bark Configuration
// ============================================================================

// Bark can be used as an alternative to get notifications instead of Home Assistant.
// set MQTT_ENABLED to false if Home Assistant is not needed

#define BARK_ENABLED false

// Install Bark on your mobile and copy this from the Bark app (the "key" part of the URL)
#define BARK_KEY    "your_bark_device_key"

// Usually you don’t need to change this:
#define BARK_SERVER "https://api.day.app"

// Distance (cm) at which to send “low salt” alert
#define SALT_WARN_DISTANCE_CM 45.0f