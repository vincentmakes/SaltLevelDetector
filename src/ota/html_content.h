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
      
      /* Light theme (default) */
      --bg-primary: #f5f5f5;
      --bg-section: #ffffff;
      --bg-input: #ffffff;
      --bg-input-readonly: #eeeeee;
      --bg-tank: #f0f0f0;
      --bg-chip: #e0e0e0;
      --bg-chip-online: #e3f2fd;
      --bg-chip-muted: #eeeeee;
      --bg-tooltip: #333;
      
      --text-primary: #333;
      --text-secondary: #666;
      --text-muted: #999;
      --text-input: #333;
      --text-input-readonly: #666666;
      --text-chip: #333;
      --text-chip-online: #0d47a1;
      --text-chip-muted: #555;
      --text-tooltip: #fff;
      
      --border-color: #ccc;
      --border-tank: #444;
      --shadow: 0 1px 3px rgba(0,0,0,0.08);
      
      --accent-color: #1976d2;
      --accent-light: #e3f2fd;
      --success-color: #4caf50;
    }
    
    /* Dark theme */
    [data-theme="dark"] {
      --bg-primary: #121212;
      --bg-section: #1e1e1e;
      --bg-input: #111;
      --bg-input-readonly: #222;
      --bg-tank: #222;
      --bg-chip: #333;
      --bg-chip-online: #1b3b5a;
      --bg-chip-muted: #2a2a2a;
      --bg-tooltip: #2a2a2a;
      
      --text-primary: #e0e0e0;
      --text-secondary: #aaa;
      --text-muted: #888;
      --text-input: #eee;
      --text-input-readonly: #aaa;
      --text-chip: #eee;
      --text-chip-online: #e3f2fd;
      --text-chip-muted: #ccc;
      --text-tooltip: #fff;
      
      --border-color: #555;
      --border-tank: #888;
      --shadow: none;
      
      --accent-color: #64b5f6;
      --accent-light: #1b3b5a;
    }
    
    body {
      margin: 0;
      padding: 0;
      background: var(--bg-primary);
      color: var(--text-primary);
      transition: background 0.3s, color 0.3s;
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
      border: 1px solid var(--border-color);
      background: var(--bg-input);
      color: var(--text-input);
      font-size: 0.95rem;
      transition: background 0.3s, color 0.3s, border-color 0.3s;
    }
    input[type="number"][readonly],
    input[type="text"][readonly] {
      background: var(--bg-input-readonly);
      color: var(--text-input-readonly);
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
      background: var(--accent-color);
      color: #fff;
      cursor: pointer;
      transition: background 0.3s;
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
      background: var(--bg-chip);
      color: var(--text-chip);
      white-space: nowrap;
      transition: background 0.3s, color 0.3s;
    }
    .chip-online {
      background: var(--bg-chip-online);
      color: var(--text-chip-online);
    }
    .chip-muted {
      background: var(--bg-chip-muted);
      color: var(--text-chip-muted);
    }
    .chip .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--success-color);
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
      border: 2px solid var(--border-tank);
      border-radius: 10px;
      position: relative;
      overflow: hidden;
      background: var(--bg-tank);
      transition: background 0.3s, border-color 0.3s;
    }
    #tank_fill {
      position: absolute;
      bottom: 0;
      left: 0;
      width: 100%;
      height: 0;
      background: var(--success-color);
      transition: height 0.4s;
    }
    #prg {
      margin-top: 8px;
      font-size: 0.85rem;
    }
    .section {
      background: var(--bg-section);
      border-radius: 10px;
      padding: 12px;
      margin-top: 12px;
      box-shadow: var(--shadow);
      transition: background 0.3s, box-shadow 0.3s;
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
      color: var(--text-secondary);
      margin-top: 4px;
    }
    
    /* Theme toggle button */
    .theme-toggle {
      position: fixed;
      top: 12px;
      right: 12px;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      border: none;
      background: var(--bg-section);
      color: var(--text-primary);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 1.2rem;
      box-shadow: 0 2px 8px rgba(0,0,0,0.15);
      z-index: 1000;
      transition: background 0.3s, transform 0.2s;
      padding: 0;
      margin: 0;
      width: 40px !important;
    }
    .theme-toggle:hover {
      transform: scale(1.1);
    }
    .theme-toggle:active {
      transform: scale(0.95);
    }
    .theme-toggle .icon-sun,
    .theme-toggle .icon-moon {
      display: none;
    }
    [data-theme="light"] .theme-toggle .icon-moon {
      display: block;
    }
    [data-theme="dark"] .theme-toggle .icon-sun {
      display: block;
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
      background: var(--accent-color);
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
      background-color: var(--bg-tooltip);
      color: var(--text-tooltip);
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
    
    [data-theme="dark"] .tooltip .tooltiptext {
      border: 1px solid #444;
    }
    
    .tooltip .tooltiptext::after {
      content: "";
      position: absolute;
      top: 100%;
      left: 50%;
      margin-left: -5px;
      border-width: 5px;
      border-style: solid;
      border-color: var(--bg-tooltip) transparent transparent transparent;
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
      color: var(--success-color);
    }
  </style>
</head>
<body>
  <!-- Theme toggle button -->
  <button class="theme-toggle" id="theme-toggle" title="{{STR_TOGGLE_THEME}}">
    <span class="icon-sun">☀️</span>
    <span class="icon-moon">🌙</span>
  </button>

  <div class="container">
    <h1>{{STR_H1}} <small style="font-size:0.5em;color:var(--text-muted);">v2.3.0</small></h1>

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
          {{STR_CONSEC_HOURS}}
          <input type="number" step="1" min="1" max="48" name="consec_hours" value="{{CONSEC_HOURS}}">
          <div class="help-text">{{STR_CONSEC_HOURS_HELP}}</div>
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
    // Theme management
    (function() {
      const THEME_KEY = 'salt-monitor-theme';
      
      function getSystemTheme() {
        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
      }
      
      function getStoredTheme() {
        return localStorage.getItem(THEME_KEY);
      }
      
      function setTheme(theme) {
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem(THEME_KEY, theme);
      }
      
      function initTheme() {
        const stored = getStoredTheme();
        const theme = stored || getSystemTheme();
        setTheme(theme);
      }
      
      function toggleTheme() {
        const current = document.documentElement.getAttribute('data-theme');
        const next = current === 'dark' ? 'light' : 'dark';
        setTheme(next);
      }
      
      // Initialize theme immediately to prevent flash
      initTheme();
      
      // Set up toggle button after DOM loads
      document.addEventListener('DOMContentLoaded', function() {
        const toggleBtn = document.getElementById('theme-toggle');
        if (toggleBtn) {
          toggleBtn.addEventListener('click', toggleTheme);
        }
        
        // Listen for system theme changes
        window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function(e) {
          if (!getStoredTheme()) {
            setTheme(e.matches ? 'dark' : 'light');
          }
        });
      });
    })();

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
  <div style="text-align:center; margin-top:24px; padding:16px; color:var(--text-muted); font-size:0.8em;">
    Firmware v2.3.0 | Build: {{BUILD_TIME}}
  </div>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H
