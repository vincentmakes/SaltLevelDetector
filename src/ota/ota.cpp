#include "ota.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <math.h>

namespace saltlevel {

  // -------------------------------------------------------------------------
  // Responsive, bilingual HTML UI (Tank → Settings → OTA)
  // -------------------------------------------------------------------------
  static const char serverIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>{{STR_TITLE}}</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
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
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>{{STR_H1}}</h1>

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
            <option value="fr" {{LANG_FR_SELECTED}}>Français</option>
          </select>
        </label>
        <input type="submit" value="{{STR_SAVE}}">
      </form>
    </div>

    <!-- 3) Firmware update -->
    <div class="section">
      <h2>{{STR_OTA}}</h2>
      <form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
        <input type="file" name="update" style="margin-top:6px;">
        <input type="submit" value="Update">
      </form>
      <div id="prg">Progress: 0%</div>
    </div>
  </div>

  <script>
    // OTA upload
    $('#upload_form').on('submit', function(e){
      e.preventDefault();
      var form = $('#upload_form')[0];
      var data = new FormData(form);
      $.ajax({
        url: '/update',
        type: 'POST',
        data: data,
        contentType: false,
        processData: false,
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
        success: function(d, s) { console.log('OTA success'); },
        error: function(a, b, c) { console.log('OTA error'); }
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
</body>
</html>
)rawliteral";

  // -------------------------------------------------------------------------
  // Globals
  // -------------------------------------------------------------------------
  static WebServer       server(80);
  static DistanceCallback distanceCb = nullptr;
  static PublishCallback  publishCb  = nullptr;
  static Config*          cfg        = nullptr;
  static Preferences      prefs;

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
    char tmp[128];
    size_t len = prefs.getString("bark_key", tmp, sizeof(tmp));
    if (len > 0) {
      tmp[sizeof(tmp) - 1] = '\0';
      strncpy(cfg->barkKey, tmp, sizeof(cfg->barkKey));
      cfg->barkKey[sizeof(cfg->barkKey) - 1] = '\0';
    }

    cfg->barkEnabled = prefs.getBool("bark_en", cfg->barkEnabled);

    uint8_t lang = prefs.getUChar("lang", cfg->language);
    if (lang > 1) lang = 0;
    cfg->language = lang;

    prefs.end();
  }

  static void saveConfigToNvs() {
    if (!cfg) return;

    prefs.begin("saltcfg", false);
    prefs.putFloat("full_cm",  cfg->fullDistanceCm);
    prefs.putFloat("empty_cm", cfg->emptyDistanceCm);
    prefs.putFloat("warn_cm",  cfg->warnDistanceCm);
    prefs.putString("bark_key", String(cfg->barkKey));
    prefs.putBool("bark_en", cfg->barkEnabled);
    prefs.putUChar("lang", cfg->language);
    prefs.end();
  }

  // -------------------------------------------------------------------------
  // Translations
  // -------------------------------------------------------------------------
  static void applyTranslations(String& page, uint8_t lang) {
    if (lang == 1) {
      // French
      page.replace("{{STR_TITLE}}",       "Surveillance du niveau de sel");
      page.replace("{{STR_H1}}",          "Surveillance du niveau de sel");
      page.replace("{{STR_OTA}}",         "Mise à jour OTA");
      page.replace("{{STR_SETTINGS}}",    "Réglages du réservoir et de Bark");
      page.replace("{{STR_LANG}}",        "Langue de l&#39;interface :");
      page.replace("{{STR_FULL}}",        "Distance minimale lorsque le réservoir est PLEIN (limite matérielle, cm) :");
      page.replace("{{STR_EMPTY}}",       "Distance lorsque le réservoir est VIDE (profondeur max, cm) :");
      page.replace("{{STR_WARN}}",        "Distance d&#39;avertissement Bark (cm) :");
      page.replace("{{STR_BARK_KEY}}",    "Clé Bark :");
      page.replace("{{STR_BARK_ENABLE}}", "Activer les notifications Bark");
      page.replace("{{STR_SAVE}}",        "Enregistrer les réglages");
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
      page.replace("{{STR_SAVE}}",        "Save settings");
      page.replace("{{STR_CURRENT}}",     "Current Level");
      page.replace("{{STR_MEASURE}}",     "Measure now");
      page.replace("{{STR_DISTANCE}}",    "Distance:");
      page.replace("{{STR_FULLNESS}}",    "Fullness:");
    }

    if (lang == 1) {
      page.replace("{{LANG_EN_SELECTED}}", "");
      page.replace("{{LANG_FR_SELECTED}}", "selected");
    } else {
      page.replace("{{LANG_EN_SELECTED}}", "selected");
      page.replace("{{LANG_FR_SELECTED}}", "");
    }
  }

  // -------------------------------------------------------------------------
  // Public methods
  // -------------------------------------------------------------------------
  void OTA::setDistanceCallback(DistanceCallback cb) {
    distanceCb = cb;
  }

  void OTA::setPublishCallback(PublishCallback cb) {
    publishCb = cb;
  }

  void OTA::setConfig(Config* c) {
    cfg = c;
  }

  void OTA::setup() {
    // Load persisted config (overriding defaults from main, incl. Bark enabled/disabled)
    loadConfigFromNvs();

    if (!MDNS.begin("saltlevel-esp32")) {
      Serial.println("Error starting mDNS");
    } else {
      Serial.println("mDNS responder started: http://saltlevel-esp32.local/");
    }

    // Root page
    server.on("/", HTTP_GET, []() {
      String page = String(serverIndex);

      if (cfg) {
        page.replace("{{FULL_CM}}",  String(cfg->fullDistanceCm, 1));
        page.replace("{{EMPTY_CM}}", String(cfg->emptyDistanceCm, 1));
        page.replace("{{WARN_CM}}",  String(cfg->warnDistanceCm, 1));
        page.replace("{{BARK_KEY}}", String(cfg->barkKey));
        page.replace("{{BARK_EN_CHECKED}}", cfg->barkEnabled ? "checked" : "");
        applyTranslations(page, cfg->language);
      } else {
        page.replace("{{FULL_CM}}",  "20.0");
        page.replace("{{EMPTY_CM}}", "58.0");
        page.replace("{{WARN_CM}}",  "45.0");
        page.replace("{{BARK_KEY}}", "");
        page.replace("{{BARK_EN_CHECKED}}", "");
        applyTranslations(page, 0);
      }

      server.sendHeader("Connection", "close");
      server.send(200, "text/html", page);
    });

    // Settings POST
    server.on("/config", HTTP_POST, []() {
      if (!cfg) {
        server.send(500, "text/plain", "No config bound");
        return;
      }

      if (server.hasArg("full_cm")) {
        // still accept, even though field is readonly; but normally unchanged
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
      // checkbox: present when checked, absent when not
      cfg->barkEnabled = server.hasArg("bark_en");

      if (server.hasArg("lang")) {
        String l = server.arg("lang");
        l.toLowerCase();
        cfg->language = (l == "fr") ? 1 : 0;
      }

      saveConfigToNvs();

      server.sendHeader("Location", "/");
      server.send(303);
    });

    // Distance measurement (also publishes to MQTT via callback)
    server.on("/measure", HTTP_GET, []() {
      if (!distanceCb) {
        server.send(500, "application/json", "{\"error\":\"no_callback\"}");
        return;
      }

      float d = distanceCb();

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
    });

    // Simple API status endpoint (for integrations)
    server.on("/api/status", HTTP_GET, []() {
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

      char json[256];
      snprintf(json, sizeof(json),
        "{"
          "\"distance\":%.2f,"
          "\"percent\":%.1f,"
          "\"full_cm\":%.2f,"
          "\"empty_cm\":%.2f,"
          "\"warn_cm\":%.2f,"
          "\"bark_enabled\":%s,"
          "\"language\":%u"
        "}",
        d,
        percent,
        cfg->fullDistanceCm,
        cfg->emptyDistanceCm,
        cfg->warnDistanceCm,
        cfg->barkEnabled ? "true" : "false",
        (unsigned)cfg->language
      );

      server.send(200, "application/json", json);
    });

    // OTA update
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });

    server.begin();
    Serial.println("HTTP OTA server started");
  }

  void OTA::loop() {
    server.handleClient();
    delay(1);
  }

} // namespace saltlevel
