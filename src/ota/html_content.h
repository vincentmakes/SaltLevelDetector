#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

#include <Arduino.h>

// Web interface HTML - kept separate for easier editing
// To update: edit index.html, then copy content here
static const char HTML_CONTENT[] PROGMEM = R"rawliteral(
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
    input[type="number"][readonly],
    input[type="text"][readonly] {
      background: #eeeeee;
      color: #666666;
      cursor: not-allowed;
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
    
    /* Label with info icon */
    .label-with-info {
      display: flex;
      align-items: center;
      gap: 6px;
      margin-top: 10px;
      font-size: 0.9rem;
    }
    
    /* Info icon styling */
    .info-icon {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 18px;
      height: 18px;
      border-radius: 50%;
      background: #1976d2;
      color: white;
      font-size: 0.75rem;
      font-weight: bold;
      cursor: help;
      position: relative;
      flex-shrink: 0;
    }
    
    /* Tooltip */
    .tooltip {
      position: relative;
      display: inline-block;
    }
    
    .tooltip .tooltiptext {
      visibility: hidden;
      width: 280px;
      background-color: #333;
      color: #fff;
      text-align: left;
      border-radius: 8px;
      padding: 12px;
      position: absolute;
      z-index: 1000;
      bottom: 125%;
      left: 50%;
      margin-left: -140px;
      opacity: 0;
      transition: opacity 0.3s;
      font-size: 0.85rem;
      line-height: 1.4;
      box-shadow: 0 4px 12px rgba(0,0,0,0.3);
      pointer-events: none;
    }
    
    .tooltip .tooltiptext::after {
      content: "";
      position: absolute;
      top: 100%;
      left: 50%;
      margin-left: -5px;
      border-width: 5px;
      border-style: solid;
      border-color: #333 transparent transparent transparent;
    }
    
    /* Show on hover for desktop */
    .tooltip:hover .tooltiptext {
      visibility: visible;
      opacity: 1;
    }
    
    /* Show when active class is added (for mobile tap) */
    .tooltip.active .tooltiptext {
      visibility: visible;
      opacity: 1;
    }
    
    @media (hover: none) {
      .tooltip .tooltiptext {
        width: 250px;
        margin-left: -125px;
      }
    }
    
    .tooltip-step {
      margin: 6px 0;
    }
    
    .tooltip-step strong {
      color: #4caf50;
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
      input[type="number"][readonly],
      input[type="text"][readonly] {
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
      .info-icon {
        background: #64b5f6;
      }
      .tooltip .tooltiptext {
        background-color: #2a2a2a;
        border: 1px solid #444;
      }
      .tooltip .tooltiptext::after {
        border-color: #2a2a2a transparent transparent transparent;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>{{STR_H1}} <small style="font-size:0.5em;color:#999;">v2.2.0</small></h1>

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
        <span>{{STR_LEVEL}}</span>
        <span class="value" id="percent_text">--</span>
      </div>

      <div id="tank-wrapper">
        <div id="tank">
          <div id="tank_fill"></div>
        </div>
      </div>
    </div>

    <!-- 2) Tank Settings -->
    <div class="section">
      <h2>{{STR_TANK_SETTINGS}}</h2>
      <form method="POST" action="/config" id="settings_form">
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
          {{STR_LANG}}
          <select name="lang">
            <option value="en" {{LANG_EN_SELECTED}}>English</option>
            <option value="fr" {{LANG_FR_SELECTED}}>Français</option>
          </select>
        </label>
      </form>
    </div>

    <!-- 3) Notification Settings -->
    <div class="section">
      <h2>{{STR_NOTIFICATIONS}}</h2>
      <!-- Bark Notifications -->
      <div class="label-with-info">
        <span>{{STR_BARK_KEY}}</span>
        <div class="tooltip">
          <span class="info-icon">i</span>
          <span class="tooltiptext">
            <strong>{{STR_BARK_TOOLTIP_TITLE}}</strong><br>
            <div class="tooltip-step"><strong>1.</strong> {{STR_BARK_STEP1}}</div>
            <div class="tooltip-step"><strong>2.</strong> {{STR_BARK_STEP2}}</div>
            <div class="tooltip-step"><strong>3.</strong> {{STR_BARK_STEP3}}</div>
          </span>
        </div>
      </div>
      <input type="text" name="bark_key" value="{{BARK_KEY}}" placeholder="{{STR_BARK_PLACEHOLDER}}" form="settings_form">
      <div class="toggle-line">
        <input type="checkbox" name="bark_en" {{BARK_EN_CHECKED}} form="settings_form">
        <span>{{STR_BARK_ENABLE}}</span>
      </div>
      
      <!-- ntfy Notifications -->
      <div class="label-with-info" style="margin-top: 16px;">
        <span>{{STR_NTFY_TOPIC}}</span>
        <div class="tooltip">
          <span class="info-icon">i</span>
          <span class="tooltiptext">
            <strong>{{STR_NTFY_TOOLTIP_TITLE}}</strong><br>
            <div class="tooltip-step"><strong>1.</strong> {{STR_NTFY_STEP1}}</div>
            <div class="tooltip-step"><strong>2.</strong> {{STR_NTFY_STEP2}}</div>
            <div class="tooltip-step"><strong>3.</strong> {{STR_NTFY_STEP3}}</div>
            <div class="tooltip-step"><strong>4.</strong> {{STR_NTFY_STEP4}}</div>
          </span>
        </div>
      </div>
      <input type="text" name="ntfy_topic" value="{{NTFY_TOPIC}}" readonly form="settings_form">
      <div class="help-text">{{STR_NTFY_HELP}}</div>
      <div class="toggle-line">
        <input type="checkbox" name="ntfy_en" {{NTFY_EN_CHECKED}} form="settings_form">
        <span>{{STR_NTFY_ENABLE}}</span>
      </div>
      
      <input type="submit" value="{{STR_SAVE}}" form="settings_form">
    </div>

    <!-- 4) Firmware update -->
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
      
      // Handle tooltip clicks for mobile
      const tooltips = document.querySelectorAll('.tooltip');
      
      tooltips.forEach(function(tooltip) {
        tooltip.addEventListener('click', function(e) {
          e.stopPropagation();
          
          // Close all other tooltips
          tooltips.forEach(function(t) {
            if (t !== tooltip) {
              t.classList.remove('active');
            }
          });
          
          // Toggle this tooltip
          tooltip.classList.toggle('active');
        });
      });
      
      // Close tooltips when clicking outside
      document.addEventListener('click', function() {
        tooltips.forEach(function(t) {
          t.classList.remove('active');
        });
      });
    });
  </script>
  <div style="text-align:center; margin-top:24px; padding:16px; color:#999; font-size:0.8em;">
    Firmware v2.2.0 | Build: {{BUILD_TIME}}
  </div>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H