#include "ota.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <math.h>
#include "../constants.h"
#include "../logger.h"

namespace saltlevel {

  // -------------------------------------------------------------------------
  // HTML UI - Responsive, bilingual (Tank â†’ Settings â†’ OTA)
  // -------------------------------------------------------------------------
  static const char serverIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>{{STR_TITLE}}</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
  <meta http-equiv="Pragma" content="no-cache">
  <meta http-equiv="Expires" content="0">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
  <style>
    :root {
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI",
                   Roboto, Helvetica, Arial, sans-serif;
      color-scheme: light dark;
    }
    body {
      margin: 0;
      padding: 0;
      background: #f5f5f5;
    }
    .container {
      max-width: 480px;
      margin: 0 auto;
      padding: 16px;
      box-sizing: border-box;
    }
    h1 {
      margin: 0 0 8px 0;
      font-size: 1.4rem;
      text-align: center;
    }
    h2 {
      margin: 16px 0 8px 0;
      font-size: 1.1rem;
    }
    label {
      display: block;
      margin-top: 10px;
      font-size: 0.9rem;
    }
    input[type="number"],
    input[type="text"],
    input[type="password"],
    select {
      width: 100%;
      box-sizing: border-box;
      padding: 8px 10px;
      margin-top: 4px;
      border-radius: 6px;
      border: 1px solid #ccc;
      font-size: 0.95rem;
    }
    input[type="number"][readonly] {
      background: #eeeeee;
      color: #666666;
    }
    input[type="submit"],
    button {
      width: 100%;
      box-sizing: border-box;
      padding: 10px 12px;
      margin-top: 12px;
      border-radius: 6px;
      border: none;
      font-size: 1rem;
      font-weight: 600;
      background: #1976d2;
      color: #fff;
      cursor: pointer;
    }
    input[type="submit"]:active,
    button:active {
      transform: scale(0.98);
    }

    .status-row {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      justify-content: center;
      margin-bottom: 8px;
    }
    .chip {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 0.75rem;
      background: #e0e0e0;
      color: #333;
      white-space: nowrap;
    }
    .chip-online {
      background: #e3f2fd;
      color: #0d47a1;
    }
    .chip-muted {
      background: #eeeeee;
      color: #555;
    }
    .chip .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #4caf50;
    }

    #tank-wrapper {
      display: flex;
      justify-content: center;
      margin-top: 12px;
    }
    #tank {
      width: 80px;
      max-width: 30vw;
      height: 200px;
      max-height: 60vw;
      border: 2px solid #444;
      border-radius: 10px;
      position: relative;
      overflow: hidden;
      background: #f0f0f0;
    }
    #tank_fill {
      position: absolute;
      bottom: 0;
      left: 0;
      width: 100%;
      height: 0;
      background: #4caf50;
      transition: height 0.4s;
    }
    #prg {
      margin-top: 8px;
      font-size: 0.85rem;
    }
    .section {
      background: #ffffff;
      border-radius: 10px;
      padding: 12px;
      margin-top: 12px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.08);
    }
    .row {
      display: flex;
      justify-content: space-between;
      gap: 8px;
      margin-top: 6px;
      font-size: 0.95rem;
    }
    .row span.value {
      font-weight: 600;
    }
    .toggle-line {
      display: flex;
      align-items: center;
      gap: 8px;
      margin-top: 10px;
      font-size: 0.9rem;
    }
    .toggle-line input[type="checkbox"] {
      width: auto;
      margin-top: 0;
    }
    .help-text {
      font-size: 0.8rem;
      color: #666;
      margin-top: 4px;
    }

    @media (prefers-color-scheme: dark) {
      body {
        background: #121212;
      }
      .section {
        background: #1e1e1e;
        box-shadow: none;
      }
      #tank {
        border-color: #888;
        background: #222;
      }
      input[type="number"],
      input[type="text"],
      input[type="password"],
      select {
        border-color: #555;
        background: #111;
        color: #eee;
      }
      input[type="number"][readonly] {
        background: #222;
        color: #aaa;
      }
      .chip {
        background: #333;
        color: #eee;
      }
      .chip-online {
        background: #1b3b5a;
        color: #e3f2fd;
      }
      .chip-muted {
        background: #2a2a2a;
        color: #ccc;
      }
      .help-text {
        color: #aaa;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>{{STR_H1}} <small style="font-size:0.5em;color:#999;">v2.1.0</small></h1>

    <div class="status-row">
      <div class="chip chip-online">
        <span class="dot"></span>
        <span id="status_text">Online</span>
      </div>
      <div class="chip chip-muted">
        <span>Last:</span>
        <span id="last_meas_text">--</span>
      </div>
    </div>

    <!-- 1) Tank level / visualization -->
    <div class="section">
      <h2>{{STR_CURRENT}}</h2>
      <button id="measure_btn">{{STR_MEASURE}}</button>

      <div class="row">
        <span>{{STR_DISTANCE}}</span>
        <span class="value" id="distance_text">--</span>
      </div>
      <div class="row">
        <span>{{STR_FULLNESS}}</span>
        <span class="value" id="percent_text">--</span>
      </div>

      <div id="tank-wrapper">
        <div id="tank">
          <div id="tank_fill"></div>
        </div>
      </div>
    </div>

    <!-- 2) Settings -->
    <div class="section">
      <h2>{{STR_SETTINGS}}</h2>
      <form method="POST" action="/config">
        <label>
          {{STR_FULL}}
          <input type="number" step="0.1" name="full_cm" value="{{FULL_CM}}" readonly>
          <div class="help-text">Hardware limitation - do not change</div>
        </label>
        <label>
          {{STR_EMPTY}}
          <input type="number" step="0.1" name="empty_cm" value="{{EMPTY_CM}}">
        </label>
        <label>
          {{STR_WARN}}
          <input type="number" step="0.1" name="warn_cm" value="{{WARN_CM}}">
        </label>
        <label>
          {{STR_BARK_KEY}}
          <input type="text" name="bark_key" value="{{BARK_KEY}}">
        </label>
        <div class="toggle-line">
          <input type="checkbox" name="bark_en" {{BARK_EN_CHECKED}}>
          <span>{{STR_BARK_ENABLE}}</span>
        </div>
        <label>
          {{STR_LANG}}
          <select name="lang">
            <option value="en" {{LANG_EN_SELECTED}}>English</option>
            <option value="fr" {{LANG_FR_SELECTED}}>FranÃ§ais</option>
          </select>
        </label>
        <input type="submit" value="{{STR_SAVE}}">
      </form>
    </div>

    <!-- 3) Firmware update -->
    <div class="section">
      <h2>{{STR_OTA}}</h2>
      <form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
        <label>
          {{STR_OTA_PASSWORD}}
          <input type="password" name="password" id="ota_password" required>
        </label>
        <input type="file" name="update" style="margin-top:6px;" required>
        <input type="submit" value="Update">
      </form>
      <div id="prg">Progress: 0%</div>
    </div>
  </div>

  <script>
    // OTA upload with password
    $('#upload_form').on('submit', function(e){
      e.preventDefault();
      var form = $('#upload_form')[0];
      var data = new FormData(form);
      
      var password = $('#ota_password').val();
      
      $.ajax({
        url: '/update',
        type: 'POST',
        data: data,
        contentType: false,
        processData: false,
        beforeSend: function(xhr) {
          xhr.setRequestHeader('Authorization', 'Basic ' + btoa('admin:' + password));
        },
        xhr: function() {
          var xhr = new window.XMLHttpRequest();
          xhr.upload.addEventListener('progress', function(evt){
            if (evt.lengthComputable) {
              var per = evt.loaded / evt.total * 100;
              $('#prg').html('Progress: ' + per.toFixed(0) + '%');
            }
          }, false);
          return xhr;
        },
        success: function(d, s) {
          $('#prg').html('Update successful! Device rebooting...<br><small>Page will reload in 10 seconds</small>');
          setTimeout(function() {
            window.location.reload();
          }, 10000);
        },
        error: function(xhr, status, error) {
          // Check if it's a connection error (device rebooted during response)
          if (xhr.status === 0 && status === 'error') {
            // Likely a successful update followed by reboot
            $('#prg').html('Upload complete! Device rebooting...<br><small>Page will reload in 15 seconds</small>');
            setTimeout(function() {
              window.location.reload();
            }, 15000);
          } else if (xhr.status === 401) {
            $('#prg').html('Error: Invalid password');
          } else if (xhr.status === 500) {
            $('#prg').html('Error: Update failed - ' + error);
          } else {
            $('#prg').html('Error: ' + error + ' (Status: ' + xhr.status + ')');
          }
        }
      });
    });

    function updateView(obj) {
      if (obj.distance !== undefined) {
        document.getElementById('distance_text').textContent =
          obj.distance.toFixed(1) + ' cm';
      }
      if (obj.percent !== undefined && obj.percent >= 0) {
        var p = Math.max(0, Math.min(100, obj.percent));
        document.getElementById('percent_text').textContent =
          p.toFixed(1) + ' %';
        document.getElementById('tank_fill').style.height = p + '%';
      }
    }

    function updateLastMeasurementTime() {
      var el = document.getElementById('last_meas_text');
      if (!el) return;
      var now = new Date();
      var hh = String(now.getHours()).padStart(2, '0');
      var mm = String(now.getMinutes()).padStart(2, '0');
      el.textContent = hh + ':' + mm;
    }

    function doMeasure() {
      document.getElementById('distance_text').textContent = '...';
      fetch('/measure')
        .then(r => r.json())
        .then(obj => {
          updateView(obj);
          updateLastMeasurementTime();
        })
        .catch(err => {
          console.log(err);
          document.getElementById('distance_text').textContent = 'Error';
        });
    }

    document.getElementById('measure_btn').addEventListener('click', function(){
      doMeasure();
    });

    window.addEventListener('load', function(){
      doMeasure();
    });
  </script>
  <div style="text-align:center; margin-top:24px; padding:16px; color:#999; font-size:0.8em;">
    Firmware v2.1.0 | Build: {{BUILD_TIME}}
  </div>
</body>
</html>
)rawliteral";

  // -------------------------------------------------------------------------
  // Globals
  // -------------------------------------------------------------------------
  static WebServer       server(Network::HTTP_PORT);
  static DistanceCallback distanceCb = nullptr;
  static PublishCallback  publishCb  = nullptr;
  static Config*          cfg        = nullptr;
  static Preferences      prefs;
  static bool             otaAuthFailed = false;  // Track auth failure
  
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

    // Use current struct values as defaults (so secrets.h defaults work on first boot)
    cfg->fullDistanceCm  = prefs.getFloat("full_cm",  cfg->fullDistanceCm);
    cfg->emptyDistanceCm = prefs.getFloat("empty_cm", cfg->emptyDistanceCm);
    cfg->warnDistanceCm  = prefs.getFloat("warn_cm",  cfg->warnDistanceCm);

    // Bark key: only override if something stored
    char tmp[Limits::BARK_KEY_LENGTH];
    size_t len = prefs.getString("bark_key", tmp, sizeof(tmp));
    if (len > 0) {
      tmp[sizeof(tmp) - 1] = '\0';
      strncpy(cfg->barkKey, tmp, sizeof(cfg->barkKey));
      cfg->barkKey[sizeof(cfg->barkKey) - 1] = '\0';
    }

    // Note: OTA password is NOT loaded from NVS - it must be set in secrets.h
    // Clean up any old ota_pass that might be in NVS from previous versions
    if (prefs.isKey("ota_pass")) {
      prefs.remove("ota_pass");
      Logger::info("Removed old OTA password from NVS");
    }

    cfg->barkEnabled = prefs.getBool("bark_en", cfg->barkEnabled);

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
    // Note: OTA password is NOT saved to NVS - it must be set in secrets.h
    prefs.putBool("bark_en", cfg->barkEnabled);
    prefs.putUChar("lang", static_cast<uint8_t>(cfg->language));
    prefs.end();
    
    Logger::info("Configuration saved to NVS");
  }

  // Continued in Part 2...
  // -------------------------------------------------------------------------
  // Translations
  // -------------------------------------------------------------------------
  static void applyTranslations(String& page, Language lang) {
    if (lang == Language::FRENCH) {
      // French
      page.replace("{{STR_TITLE}}",       "Surveillance du niveau de sel");
      page.replace("{{STR_H1}}",          "Surveillance du niveau de sel");
      page.replace("{{STR_OTA}}",         "Mise Ã  jour OTA");
      page.replace("{{STR_SETTINGS}}",    "RÃ©glages du rÃ©servoir et de Bark");
      page.replace("{{STR_LANG}}",        "Langue de l&#39;interface :");
      page.replace("{{STR_FULL}}",        "Distance minimale lorsque le rÃ©servoir est PLEIN (limite matÃ©rielle, cm) :");
      page.replace("{{STR_EMPTY}}",       "Distance lorsque le rÃ©servoir est VIDE (profondeur max, cm) :");
      page.replace("{{STR_WARN}}",        "Distance d&#39;avertissement Bark (cm) :");
      page.replace("{{STR_BARK_KEY}}",    "ClÃ© Bark :");
      page.replace("{{STR_BARK_ENABLE}}", "Activer les notifications Bark");
      page.replace("{{STR_OTA_PASSWORD}}", "Mot de passe OTA :");
      page.replace("{{STR_SAVE}}",        "Enregistrer les rÃ©glages");
      page.replace("{{STR_CURRENT}}",     "Niveau actuel");
      page.replace("{{STR_MEASURE}}",     "Mesurer maintenant");
      page.replace("{{STR_DISTANCE}}",    "Distance :");
      page.replace("{{STR_FULLNESS}}",    "Remplissage :");
    } else {
      // English
      page.replace("{{STR_TITLE}}",       "Salt Level Monitor");
      page.replace("{{STR_H1}}",          "Salt Level Monitor");
      page.replace("{{STR_OTA}}",         "OTA Update");
      page.replace("{{STR_SETTINGS}}",    "Tank & Bark Settings");
      page.replace("{{STR_LANG}}",        "UI language:");
      page.replace("{{STR_FULL}}",        "Minimum distance when tank is FULL (hardware limit, cm):");
      page.replace("{{STR_EMPTY}}",       "Distance when tank is EMPTY (max depth, cm):");
      page.replace("{{STR_WARN}}",        "Bark warning distance (cm):");
      page.replace("{{STR_BARK_KEY}}",    "Bark key:");
      page.replace("{{STR_BARK_ENABLE}}", "Enable Bark notifications");
      page.replace("{{STR_OTA_PASSWORD}}", "OTA password:");
      page.replace("{{STR_SAVE}}",        "Save settings");
      page.replace("{{STR_CURRENT}}",     "Current Level");
      page.replace("{{STR_MEASURE}}",     "Measure now");
      page.replace("{{STR_DISTANCE}}",    "Distance:");
      page.replace("{{STR_FULLNESS}}",    "Fullness:");
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

    // Validate full distance
    if (config->fullDistanceCm < 10.0f || config->fullDistanceCm > 100.0f) {
      Logger::errorf("Validation failed: full distance %.1f out of range [10-100]",
                    config->fullDistanceCm);
      return false;
    }

    // Validate empty distance
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

    // Validate warn distance
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
    String page = String(serverIndex);

    // Inject compile time (C preprocessor macros work here)
    String buildTime = String(__DATE__) + " " + String(__TIME__);
    page.replace("{{BUILD_TIME}}", buildTime);

    if (cfg) {
      page.replace("{{FULL_CM}}",  String(cfg->fullDistanceCm, 1));
      page.replace("{{EMPTY_CM}}", String(cfg->emptyDistanceCm, 1));
      page.replace("{{WARN_CM}}",  String(cfg->warnDistanceCm, 1));
      page.replace("{{BARK_KEY}}", String(cfg->barkKey));
      page.replace("{{BARK_EN_CHECKED}}", cfg->barkEnabled ? "checked" : "");
      applyTranslations(page, cfg->language);
    } else {
      page.replace("{{FULL_CM}}",  String(Sensor::DEFAULT_FULL_DISTANCE_CM, 1));
      page.replace("{{EMPTY_CM}}", String(Sensor::DEFAULT_EMPTY_DISTANCE_CM, 1));
      page.replace("{{WARN_CM}}",  String(Sensor::DEFAULT_WARN_DISTANCE_CM, 1));
      page.replace("{{BARK_KEY}}", "");
      page.replace("{{BARK_EN_CHECKED}}", "");
      applyTranslations(page, Language::ENGLISH);
    }

    // Add cache-busting headers
    server.sendHeader("Connection", "close");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.sendHeader("ETag", String(millis()));  // Unique ETag every request
    server.send(200, "text/html", page);
  }

  static void handleConfig() {
    if (!cfg) {
      server.send(500, "text/plain", "No config bound");
      Logger::error("Config POST failed: no config bound");
      return;
    }

    Logger::info("Processing configuration update...");

    // Parse form data
    if (server.hasArg("full_cm")) {
      // Accept but normally unchanged (readonly field)
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
    
    // Checkbox: present when checked, absent when not
    cfg->barkEnabled = server.hasArg("bark_en");

    if (server.hasArg("lang")) {
      String l = server.arg("lang");
      l.toLowerCase();
      cfg->language = (l == "fr") ? Language::FRENCH : Language::ENGLISH;
    }

    // Validate before saving
    if (!OTA::validateConfig(cfg)) {
      server.send(400, "text/plain", "Invalid configuration - check serial logs");
      return;
    }

    saveConfigToNvs();

    Logger::infof("Configuration updated: Tank %.1f-%.1f cm, Warn %.1f cm, Bark %s",
                 cfg->fullDistanceCm, cfg->emptyDistanceCm, cfg->warnDistanceCm,
                 cfg->barkEnabled ? "ON" : "OFF");

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
      // GET /api/config - return current config
      char json[Limits::JSON_BUFFER_LENGTH];
      snprintf(json, sizeof(json),
        "{"
        "\"full_cm\":%.2f,"
        "\"empty_cm\":%.2f,"
        "\"warn_cm\":%.2f,"
        "\"bark_enabled\":%s,"
        "\"language\":\"%s\""
        "}",
        cfg->fullDistanceCm,
        cfg->emptyDistanceCm,
        cfg->warnDistanceCm,
        cfg->barkEnabled ? "true" : "false",
        cfg->language == Language::FRENCH ? "fr" : "en"
      );
      server.send(200, "application/json", json);
    } else {
      server.send(405, "text/plain", "Method not allowed");
    }
  }

  static void handleUpdate() {
    // Check if authentication failed during upload
    if (otaAuthFailed) {
      server.sendHeader("Connection", "close");
      server.send(401, "text/plain", "Unauthorized: Invalid password");
      otaAuthFailed = false;  // Reset flag
      Logger::warn("OTA update rejected: invalid password");
      return;
    }
    
    if (!Update.hasError()) {
      // Send success response
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", "OK");
      server.client().flush();  // Ensure response is sent
      
      Logger::info("OTA update completed, rebooting in 2 seconds...");
      delay(2000);  // Give time for response to reach browser
      ESP.restart();
    } else {
      // Send failure response
      server.sendHeader("Connection", "close");
      server.send(500, "text/plain", "FAIL");
      Logger::error("OTA update failed");
    }
  }

  static void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
      // Reset auth flag at start
      otaAuthFailed = false;
      
      // Authenticate with password
      if (!server.authenticate("admin", cfg->otaPassword)) {
        Logger::warn("OTA authentication failed - invalid password");
        otaAuthFailed = true;  // Set flag for handleUpdate
        Update.abort();  // Abort any pending update
        return;
      }
      
      Logger::infof("OTA Update started: %s", upload.filename.c_str());
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
        Logger::error("OTA begin failed");
      }
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
      // Only write if authentication passed
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

  // Continued in Part 3...
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
    
    // Load persisted config (overriding defaults from main)
    loadConfigFromNvs();

    // Start mDNS
    if (!MDNS.begin("saltlevel-esp32")) {
      Logger::error("mDNS startup failed");
    } else {
      Logger::info("mDNS responder started: http://saltlevel-esp32.local/");
      MDNS.addService("http", "tcp", 80);
    }

    // Register routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/config", HTTP_POST, handleConfig);
    server.on("/measure", HTTP_GET, handleMeasure);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/config", HTTP_GET, handleApiConfig);
    server.on("/update", HTTP_POST, handleUpdate, handleUpdateUpload);
    
    // Version check endpoint
    server.on("/version", HTTP_GET, []() {
      String buildTime = String(__DATE__) + " " + String(__TIME__);
      char json[256];
      snprintf(json, sizeof(json),
        "{"
          "\"version\":\"2.1.0\","
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
    
    // Debug endpoint - shows what HTML is actually in PROGMEM
    server.on("/debug/html", HTTP_GET, []() {
      String page = String(serverIndex);
      server.send(200, "text/plain", page);
    });

    // 404 handler
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
    // Note: MDNS.update() not needed for ESP32 (only for ESP8266)
  }

} // namespace saltlevel