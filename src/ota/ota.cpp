#include "ota.h"
#include "html_content.h"  // HTML content in separate file

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <math.h>
#include "esp_task_wdt.h"
#include "../constants.h"
#include "../logger.h"
#include "../ntfy/ntfy.h"

namespace saltlevel {

  // -------------------------------------------------------------------------
  // Globals
  // -------------------------------------------------------------------------
  static WebServer       server(Network::HTTP_PORT);
  static DistanceCallback distanceCb = nullptr;
  static PublishCallback  publishCb  = nullptr;
  static Config*          cfg        = nullptr;
  static Preferences      prefs;
  static bool             otaAuthFailed = false;
  
  // -------------------------------------------------------------------------
  // Uptime Helper (overflow-safe)
  // -------------------------------------------------------------------------
  static String getUptimeString() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    hours %= 24;
    minutes %= 60;
    seconds %= 60;
    
    char buffer[64];
    if (days > 0) {
      snprintf(buffer, sizeof(buffer), "%lud %02lu:%02lu:%02lu", 
               days, hours, minutes, seconds);
    } else {
      snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", 
               hours, minutes, seconds);
    }
    return String(buffer);
  }
  
  static unsigned long getUptimeSeconds() {
    return millis() / 1000;
  }

  // -------------------------------------------------------------------------
  // Config persistence
  // -------------------------------------------------------------------------
  static void loadConfigFromNvs() {
    if (!cfg) return;

    prefs.begin("saltcfg", false);

    cfg->fullDistanceCm  = prefs.getFloat("full_cm",  cfg->fullDistanceCm);
    cfg->emptyDistanceCm = prefs.getFloat("empty_cm", cfg->emptyDistanceCm);
    cfg->warnDistanceCm  = prefs.getFloat("warn_cm",  cfg->warnDistanceCm);

    // Bark key
    char tmp[Limits::BARK_KEY_LENGTH];
    size_t len = prefs.getString("bark_key", tmp, sizeof(tmp));
    if (len > 0) {
      tmp[sizeof(tmp) - 1] = '\0';
      strncpy(cfg->barkKey, tmp, sizeof(cfg->barkKey));
      cfg->barkKey[sizeof(cfg->barkKey) - 1] = '\0';
    }

    // Clean up old OTA password from NVS
    if (prefs.isKey("ota_pass")) {
      prefs.remove("ota_pass");
      Logger::info("Removed old OTA password from NVS");
    }

    cfg->barkEnabled = prefs.getBool("bark_en", cfg->barkEnabled);

    // Load ntfy settings
    char ntfyTmp[Limits::NTFY_TOPIC_LENGTH];
    size_t ntfyLen = prefs.getString("ntfy_topic", ntfyTmp, sizeof(ntfyTmp));
    if (ntfyLen > 0) {
      ntfyTmp[sizeof(ntfyTmp) - 1] = '\0';
      strncpy(cfg->ntfyTopic, ntfyTmp, sizeof(cfg->ntfyTopic));
      cfg->ntfyTopic[sizeof(cfg->ntfyTopic) - 1] = '\0';
    }
    
    cfg->ntfyEnabled = prefs.getBool("ntfy_en", cfg->ntfyEnabled);

    uint8_t lang = prefs.getUChar("lang", static_cast<uint8_t>(cfg->language));
    if (lang > 1) lang = 0;
    cfg->language = static_cast<Language>(lang);

    prefs.end();
    
    Logger::info("Configuration loaded from NVS");
  }

  static void saveConfigToNvs() {
    if (!cfg) return;

    prefs.begin("saltcfg", false);
    prefs.putFloat("full_cm",  cfg->fullDistanceCm);
    prefs.putFloat("empty_cm", cfg->emptyDistanceCm);
    prefs.putFloat("warn_cm",  cfg->warnDistanceCm);
    prefs.putString("bark_key", String(cfg->barkKey));
    prefs.putBool("bark_en", cfg->barkEnabled);
    prefs.putString("ntfy_topic", String(cfg->ntfyTopic));
    prefs.putBool("ntfy_en", cfg->ntfyEnabled);
    prefs.putUChar("lang", static_cast<uint8_t>(cfg->language));
    prefs.end();
    
    Logger::info("Configuration saved to NVS");
  }

  // -------------------------------------------------------------------------
  // Translations
  // -------------------------------------------------------------------------
  static void applyTranslations(String& page, Language lang) {
    if (lang == Language::FRENCH) {
      // French
      page.replace("{{STR_TITLE}}",       "Surveillance du niveau de sel");
      page.replace("{{STR_H1}}",          "Surveillance du niveau de sel");
      page.replace("{{STR_OTA}}",         "Mise à jour OTA");
      page.replace("{{STR_TANK_SETTINGS}}", "Réglages du réservoir");
      page.replace("{{STR_NOTIFICATIONS}}", "Notifications");
      page.replace("{{STR_LANG}}",        "Langue de l'interface :");
      page.replace("{{STR_FULL}}",        "Distance minimale lorsque le réservoir est PLEIN (limite matérielle, cm) :");
      page.replace("{{STR_EMPTY}}",       "Distance lorsque le réservoir est VIDE (profondeur max, cm) :");
      page.replace("{{STR_WARN}}",        "Distance d'avertissement (cm) :");
      
      // Bark strings
      page.replace("{{STR_BARK_KEY}}",    "Clé Bark (iOS) :");
      page.replace("{{STR_BARK_ENABLE}}", "Activer les notifications Bark");
      page.replace("{{STR_BARK_PLACEHOLDER}}", "Optionnel - pour iOS uniquement");
      page.replace("{{STR_BARK_TOOLTIP_TITLE}}", "Configuration Bark (iOS)");
      page.replace("{{STR_BARK_STEP1}}",  "Installez l'app Bark depuis l'App Store");
      page.replace("{{STR_BARK_STEP2}}",  "Ouvrez Bark et copiez votre clé d'appareil");
      page.replace("{{STR_BARK_STEP3}}",  "Collez la clé ci-dessous et activez les notifications");
      
      // ntfy strings
      page.replace("{{STR_NTFY_TOPIC}}",   "Sujet ntfy (Android/Multi-plateforme) :");
      page.replace("{{STR_NTFY_ENABLE}}",  "Activer les notifications ntfy");
      page.replace("{{STR_NTFY_HELP}}",    "Sujet unique généré automatiquement - utilisez-le pour vous abonner");
      page.replace("{{STR_NTFY_TOOLTIP_TITLE}}", "Configuration ntfy (Android)");
      page.replace("{{STR_NTFY_STEP1}}",   "Installez l'app 'ntfy' depuis Google Play");
      page.replace("{{STR_NTFY_STEP2}}",   "Appuyez sur '+' pour vous abonner à un sujet");
      page.replace("{{STR_NTFY_STEP3}}",   "Entrez le sujet affiché ci-dessous");
      page.replace("{{STR_NTFY_STEP4}}",   "Ou visitez : https://ntfy.sh/[votre-sujet]");
      
      page.replace("{{STR_OTA_PASSWORD}}", "Mot de passe OTA :");
      page.replace("{{STR_SAVE}}",        "Enregistrer les réglages");
      page.replace("{{STR_CURRENT}}",     "Niveau actuel");
      page.replace("{{STR_MEASURE}}",     "Mesurer maintenant");
      page.replace("{{STR_DISTANCE}}",    "Distance :");
      page.replace("{{STR_LEVEL}}",       "Remplissage :");
    } else {
      // English
      page.replace("{{STR_TITLE}}",       "Salt Level Monitor");
      page.replace("{{STR_H1}}",          "Salt Level Monitor");
      page.replace("{{STR_OTA}}",         "OTA Update");
      page.replace("{{STR_TANK_SETTINGS}}", "Tank Settings");
      page.replace("{{STR_NOTIFICATIONS}}", "Notifications");
      page.replace("{{STR_LANG}}",        "UI language:");
      page.replace("{{STR_FULL}}",        "Minimum distance when tank is FULL (hardware limit, cm):");
      page.replace("{{STR_EMPTY}}",       "Distance when tank is EMPTY (max depth, cm):");
      page.replace("{{STR_WARN}}",        "Warning distance (cm):");
      
      // Bark strings
      page.replace("{{STR_BARK_KEY}}",    "Bark key (iOS):");
      page.replace("{{STR_BARK_ENABLE}}", "Enable Bark notifications");
      page.replace("{{STR_BARK_PLACEHOLDER}}", "Optional - iOS only");
      page.replace("{{STR_BARK_TOOLTIP_TITLE}}", "Bark Setup (iOS)");
      page.replace("{{STR_BARK_STEP1}}",  "Install Bark app from the App Store");
      page.replace("{{STR_BARK_STEP2}}",  "Open Bark and copy your device key");
      page.replace("{{STR_BARK_STEP3}}",  "Paste the key below and enable notifications");
      
      // ntfy strings
      page.replace("{{STR_NTFY_TOPIC}}",   "ntfy topic (Android/Multi-platform):");
      page.replace("{{STR_NTFY_ENABLE}}",  "Enable ntfy notifications");
      page.replace("{{STR_NTFY_HELP}}",    "Auto-generated unique topic - use this to subscribe");
      page.replace("{{STR_NTFY_TOOLTIP_TITLE}}", "ntfy Setup (Android)");
      page.replace("{{STR_NTFY_STEP1}}",   "Install 'ntfy' app from Google Play Store");
      page.replace("{{STR_NTFY_STEP2}}",   "Tap '+' to subscribe to a topic");
      page.replace("{{STR_NTFY_STEP3}}",   "Enter the topic shown below");
      page.replace("{{STR_NTFY_STEP4}}",   "Or visit: https://ntfy.sh/[your-topic]");
      
      page.replace("{{STR_OTA_PASSWORD}}", "OTA password:");
      page.replace("{{STR_SAVE}}",        "Save settings");
      page.replace("{{STR_CURRENT}}",     "Current Level");
      page.replace("{{STR_MEASURE}}",     "Measure now");
      page.replace("{{STR_DISTANCE}}",    "Distance:");
      page.replace("{{STR_LEVEL}}",       "Level:");
    }

    if (lang == Language::FRENCH) {
      page.replace("{{LANG_EN_SELECTED}}", "");
      page.replace("{{LANG_FR_SELECTED}}", "selected");
    } else {
      page.replace("{{LANG_EN_SELECTED}}", "selected");
      page.replace("{{LANG_FR_SELECTED}}", "");
    }
  }

  // -------------------------------------------------------------------------
  // Validation
  // -------------------------------------------------------------------------
  bool OTA::validateConfig(const Config* config) {
    if (!config) {
      Logger::error("Validation: config is null");
      return false;
    }

    if (config->fullDistanceCm < 10.0f || config->fullDistanceCm > 100.0f) {
      Logger::errorf("Validation failed: full distance %.1f out of range [10-100]",
                    config->fullDistanceCm);
      return false;
    }

    if (config->emptyDistanceCm <= config->fullDistanceCm) {
      Logger::errorf("Validation failed: empty distance %.1f must be > full distance %.1f",
                    config->emptyDistanceCm, config->fullDistanceCm);
      return false;
    }

    if (config->emptyDistanceCm > 200.0f) {
      Logger::errorf("Validation failed: empty distance %.1f too large", 
                    config->emptyDistanceCm);
      return false;
    }

    if (config->warnDistanceCm < config->fullDistanceCm || 
        config->warnDistanceCm > config->emptyDistanceCm) {
      Logger::errorf("Validation failed: warn distance %.1f not in range [%.1f-%.1f]",
                    config->warnDistanceCm, config->fullDistanceCm, config->emptyDistanceCm);
      return false;
    }

    Logger::debug("Configuration validation passed");
    return true;
  }

  // -------------------------------------------------------------------------
  // Route Handlers
  // -------------------------------------------------------------------------
  static void handleRoot() {
    String page = String(HTML_CONTENT);

    String buildTime = String(__DATE__) + " " + String(__TIME__);
    page.replace("{{BUILD_TIME}}", buildTime);

    if (cfg) {
      page.replace("{{FULL_CM}}",  String(cfg->fullDistanceCm, 1));
      page.replace("{{EMPTY_CM}}", String(cfg->emptyDistanceCm, 1));
      page.replace("{{WARN_CM}}",  String(cfg->warnDistanceCm, 1));
      page.replace("{{BARK_KEY}}", String(cfg->barkKey));
      page.replace("{{BARK_EN_CHECKED}}", cfg->barkEnabled ? "checked" : "");
      page.replace("{{NTFY_TOPIC}}", String(cfg->ntfyTopic));
      page.replace("{{NTFY_EN_CHECKED}}", cfg->ntfyEnabled ? "checked" : "");
      applyTranslations(page, cfg->language);
    } else {
      page.replace("{{FULL_CM}}",  String(Sensor::DEFAULT_FULL_DISTANCE_CM, 1));
      page.replace("{{EMPTY_CM}}", String(Sensor::DEFAULT_EMPTY_DISTANCE_CM, 1));
      page.replace("{{WARN_CM}}",  String(Sensor::DEFAULT_WARN_DISTANCE_CM, 1));
      page.replace("{{BARK_KEY}}", "");
      page.replace("{{BARK_EN_CHECKED}}", "");
      page.replace("{{NTFY_TOPIC}}", "");
      page.replace("{{NTFY_EN_CHECKED}}", "");
      applyTranslations(page, Language::ENGLISH);
    }

    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.sendHeader("ETag", String(millis()));
    server.send(200, "text/html", page);
  }

  static void handleConfig() {
    if (!cfg) {
      server.send(500, "text/plain", "No config bound");
      Logger::error("Config POST failed: no config bound");
      return;
    }

    Logger::info("Processing configuration update...");

    if (server.hasArg("full_cm")) {
      cfg->fullDistanceCm = server.arg("full_cm").toFloat();
    }
    if (server.hasArg("empty_cm")) {
      cfg->emptyDistanceCm = server.arg("empty_cm").toFloat();
    }
    if (server.hasArg("warn_cm")) {
      cfg->warnDistanceCm = server.arg("warn_cm").toFloat();
    }
    if (server.hasArg("bark_key")) {
      String key = server.arg("bark_key");
      key.trim();
      key.toCharArray(cfg->barkKey, sizeof(cfg->barkKey));
      cfg->barkKey[sizeof(cfg->barkKey) - 1] = '\0';
    }
    
    cfg->barkEnabled = server.hasArg("bark_en");
    
    if (server.hasArg("ntfy_topic")) {
      String topic = server.arg("ntfy_topic");
      topic.trim();
      topic.toCharArray(cfg->ntfyTopic, sizeof(cfg->ntfyTopic));
      cfg->ntfyTopic[sizeof(cfg->ntfyTopic) - 1] = '\0';
    }
    
    cfg->ntfyEnabled = server.hasArg("ntfy_en");

    if (server.hasArg("lang")) {
      String l = server.arg("lang");
      l.toLowerCase();
      cfg->language = (l == "fr") ? Language::FRENCH : Language::ENGLISH;
    }

    if (!OTA::validateConfig(cfg)) {
      server.send(400, "text/plain", "Invalid configuration - check serial logs");
      return;
    }

    saveConfigToNvs();

    Logger::infof("Configuration updated: Tank %.1f-%.1f cm, Warn %.1f cm, Bark %s, ntfy %s",
                 cfg->fullDistanceCm, cfg->emptyDistanceCm, cfg->warnDistanceCm,
                 cfg->barkEnabled ? "ON" : "OFF",
                 cfg->ntfyEnabled ? "ON" : "OFF");

    server.sendHeader("Location", "/");
    server.send(303);
  }

  static void handleMeasure() {
    if (!distanceCb) {
      server.send(500, "application/json", "{\"error\":\"no_callback\"}");
      Logger::error("Measure failed: no distance callback");
      return;
    }

    float d = distanceCb();
    Logger::debugf("Measure endpoint called: %.2f cm", d);

    if (publishCb) {
      publishCb(d);
    }

    float percent = -1.0f;
    if (cfg && cfg->emptyDistanceCm != cfg->fullDistanceCm) {
      float full  = cfg->fullDistanceCm;
      float empty = cfg->emptyDistanceCm;
      percent = (empty - d) / (empty - full) * 100.0f;
      if (percent < 0)   percent = 0;
      if (percent > 100) percent = 100;
    }

    char json[128];
    snprintf(json, sizeof(json),
             "{\"distance\":%.2f,\"percent\":%.1f}", d, percent);

    server.send(200, "application/json", json);
  }

  static void handleApiStatus() {
    if (!distanceCb || !cfg) {
      server.send(500, "application/json", "{\"error\":\"no_config_or_callback\"}");
      return;
    }

    float d = distanceCb();
    float percent = -1.0f;
    if (cfg->emptyDistanceCm != cfg->fullDistanceCm) {
      float full  = cfg->fullDistanceCm;
      float empty = cfg->emptyDistanceCm;
      percent = (empty - d) / (empty - full) * 100.0f;
      if (percent < 0)   percent = 0;
      if (percent > 100) percent = 100;
    }

    char json[Limits::JSON_BUFFER_LENGTH];
    snprintf(json, sizeof(json),
      "{"
        "\"distance\":%.2f,"
        "\"percent\":%.1f,"
        "\"full_cm\":%.2f,"
        "\"empty_cm\":%.2f,"
        "\"warn_cm\":%.2f,"
        "\"bark_enabled\":%s,"
        "\"ntfy_enabled\":%s,"
        "\"ntfy_topic\":\"%s\","
        "\"language\":\"%s\","
        "\"wifi_rssi\":%d,"
        "\"uptime_seconds\":%lu,"
        "\"uptime\":\"%s\""
      "}",
      d,
      percent,
      cfg->fullDistanceCm,
      cfg->emptyDistanceCm,
      cfg->warnDistanceCm,
      cfg->barkEnabled ? "true" : "false",
      cfg->ntfyEnabled ? "true" : "false",
      cfg->ntfyTopic,
      cfg->language == Language::FRENCH ? "fr" : "en",
      WiFi.RSSI(),
      getUptimeSeconds(),
      getUptimeString().c_str()
    );

    server.send(200, "application/json", json);
  }

  static void handleApiConfig() {
    if (!cfg) {
      server.send(500, "application/json", "{\"error\":\"no_config\"}");
      return;
    }

    if (server.method() == HTTP_GET) {
      char json[Limits::JSON_BUFFER_LENGTH];
      snprintf(json, sizeof(json),
        "{"
        "\"full_cm\":%.2f,"
        "\"empty_cm\":%.2f,"
        "\"warn_cm\":%.2f,"
        "\"bark_enabled\":%s,"
        "\"ntfy_enabled\":%s,"
        "\"ntfy_topic\":\"%s\","
        "\"language\":\"%s\""
        "}",
        cfg->fullDistanceCm,
        cfg->emptyDistanceCm,
        cfg->warnDistanceCm,
        cfg->barkEnabled ? "true" : "false",
        cfg->ntfyEnabled ? "true" : "false",
        cfg->ntfyTopic,
        cfg->language == Language::FRENCH ? "fr" : "en"
      );
      server.send(200, "application/json", json);
    } else {
      server.send(405, "text/plain", "Method not allowed");
    }
  }

  static void handleUpdate() {
    if (otaAuthFailed) {
      server.sendHeader("Connection", "close");
      server.send(401, "text/plain", "Unauthorized: Invalid password");
      otaAuthFailed = false;
      Logger::warn("OTA update rejected: invalid password");
      return;
    }
    
    if (!Update.hasError()) {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", "OK");
      server.client().flush();
      
      Logger::info("OTA update completed, rebooting in 2 seconds...");
      delay(2000);
      ESP.restart();
    } else {
      server.sendHeader("Connection", "close");
      server.send(500, "text/plain", "FAIL");
      Logger::error("OTA update failed");
    }
  }

  static void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
      esp_task_wdt_reset(); 
      otaAuthFailed = false;
      
      if (!server.authenticate("admin", cfg->otaPassword)) {
        Logger::warn("OTA authentication failed - invalid password");
        otaAuthFailed = true;
        Update.abort();
        return;
      }
      
      Logger::infof("OTA Update started: %s", upload.filename.c_str());
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
        Logger::error("OTA begin failed");
      }
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
      esp_task_wdt_reset(); 
      if (!otaAuthFailed && Update.isRunning()) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
          Update.printError(Serial);
          Logger::errorf("OTA write failed: expected %u, wrote %u", 
                        upload.currentSize, written);
        }
      }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
      esp_task_wdt_reset(); 
      if (!otaAuthFailed && Update.isRunning()) {
        if (Update.end(true)) {
          Logger::infof("OTA Update Success: %u bytes", upload.totalSize);
        } else {
          Update.printError(Serial);
          Logger::error("OTA end failed");
        }
      }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
      Update.end();
      Logger::warn("OTA Update aborted");
    }
  }

  // -------------------------------------------------------------------------
  // Public methods
  // -------------------------------------------------------------------------
  void OTA::setDistanceCallback(DistanceCallback cb) {
    distanceCb = cb;
    Logger::debug("Distance callback registered");
  }

  void OTA::setPublishCallback(PublishCallback cb) {
    publishCb = cb;
    Logger::debug("Publish callback registered");
  }

  void OTA::setConfig(Config* c) {
    cfg = c;
    Logger::debug("Config pointer registered");
  }

  void OTA::setup() {
    Logger::info("Initializing OTA system...");
    
    loadConfigFromNvs();

    if (!MDNS.begin("saltlevel-esp32")) {
      Logger::error("mDNS startup failed");
    } else {
      Logger::info("mDNS responder started: http://saltlevel-esp32.local/");
      MDNS.addService("http", "tcp", 80);
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/measure", HTTP_GET, handleMeasure);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/update", HTTP_POST, handleUpdate, handleUpdateUpload);
    
    server.on("/version", HTTP_GET, []() {
      String buildTime = String(__DATE__) + " " + String(__TIME__);
      char json[256];
      snprintf(json, sizeof(json),
        "{"
          "\"version\":\"2.2.0\","
          "\"build\":\"%s\","
          "\"uptime_seconds\":%lu,"
          "\"uptime\":\"%s\""
        "}",
        buildTime.c_str(),
        getUptimeSeconds(),
        getUptimeString().c_str()
      );
      server.send(200, "application/json", json);
    });
    
    server.on("/debug/html", HTTP_GET, []() {
      String page = String(HTML_CONTENT);
      server.send(200, "text/plain", page);
    });

    server.onNotFound([]() {
      Logger::warnf("404 Not Found: %s", server.uri().c_str());
      server.send(404, "text/plain", "Not found");
    });

    server.begin();
    Logger::infof("HTTP server started on port %d", Network::HTTP_PORT);
    Logger::info("OTA system ready");
  }

  void OTA::loop() {
    server.handleClient();
  }

} // namespace saltlevel