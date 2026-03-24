#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include "esp_task_wdt.h"
#include "../secrets.h"
#include "../constants.h"
#include "../logger.h"
#include "wifi.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static Preferences wifiPrefs;
static WebServer* provisioningServer = nullptr;
static DNSServer* dnsServer = nullptr;
static bool provisioningMode = false;
static unsigned long provisioningStartTime = 0;

// ---------------------------------------------------------------------------
// NVS Management
// ---------------------------------------------------------------------------
WiFiCredentials loadWiFiCredentials() {
    WiFiCredentials creds;
    creds.isValid = false;
    creds.ssid[0] = '\0';
    creds.password[0] = '\0';
    
    // Try to open NVS namespace (may not exist on first boot)
    if (!wifiPrefs.begin("wifi", true)) {  // Read-only
        Logger::debug("No WiFi NVS namespace found (first boot)");
        return creds;
    }
    
    size_t ssidLen = wifiPrefs.getString("ssid", creds.ssid, sizeof(creds.ssid));
    size_t passLen = wifiPrefs.getString("pass", creds.password, sizeof(creds.password));
    
    wifiPrefs.end();
    
    // Validate credentials
    if (ssidLen > 0 && ssidLen < sizeof(creds.ssid)) {
        creds.ssid[ssidLen] = '\0';
        creds.password[passLen] = '\0';
        creds.isValid = true;
        Logger::infof("Loaded WiFi credentials from NVS: SSID=%s", creds.ssid);
    } else {
        Logger::debug("No valid WiFi credentials in NVS");
    }
    
    return creds;
}

bool saveWiFiCredentials(const char* ssid, const char* password) {
    if (!ssid || ssid[0] == '\0') {
        Logger::error("Cannot save empty SSID");
        return false;
    }
    
    wifiPrefs.begin("wifi", false);  // Read-write
    
    bool success = wifiPrefs.putString("ssid", ssid) > 0;
    success &= wifiPrefs.putString("pass", password ? password : "") >= 0;
    
    wifiPrefs.end();
    
    if (success) {
        Logger::infof("WiFi credentials saved to NVS: SSID=%s", ssid);
    } else {
        Logger::error("Failed to save WiFi credentials to NVS");
    }
    
    return success;
}

void clearWiFiCredentials() {
    wifiPrefs.begin("wifi", false);
    wifiPrefs.clear();
    wifiPrefs.end();
    Logger::info("WiFi credentials cleared from NVS");
}

// ---------------------------------------------------------------------------
// Provisioning Web Interface
// ---------------------------------------------------------------------------
static const char provisioningHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Salt Level Monitor - WiFi Setup</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    :root {
      font-family: system-ui, -apple-system, sans-serif;
      color-scheme: light dark;
    }
    body {
      margin: 0;
      padding: 20px;
      background: #f5f5f5;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
    }
    .container {
      max-width: 400px;
      width: 100%;
      background: white;
      border-radius: 12px;
      padding: 24px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    }
    h1 {
      margin: 0 0 8px 0;
      font-size: 1.5rem;
      text-align: center;
      color: #1976d2;
    }
    .subtitle {
      text-align: center;
      color: #666;
      font-size: 0.9rem;
      margin-bottom: 24px;
    }
    .info-box {
      background: #e3f2fd;
      border-left: 4px solid #1976d2;
      padding: 12px;
      margin-bottom: 20px;
      border-radius: 4px;
      font-size: 0.85rem;
    }
    label {
      display: block;
      margin-top: 16px;
      font-weight: 600;
      font-size: 0.9rem;
      color: #333;
    }
    input[type="text"],
    input[type="password"],
    select {
      width: 100%;
      box-sizing: border-box;
      padding: 10px 12px;
      margin-top: 6px;
      border-radius: 6px;
      border: 2px solid #e0e0e0;
      font-size: 1rem;
      transition: border-color 0.2s;
    }
    input[type="text"]:focus,
    input[type="password"]:focus,
    select:focus {
      outline: none;
      border-color: #1976d2;
    }
    button {
      width: 100%;
      padding: 12px;
      margin-top: 20px;
      border-radius: 6px;
      border: none;
      font-size: 1rem;
      font-weight: 600;
      background: #1976d2;
      color: white;
      cursor: pointer;
      transition: background 0.2s;
    }
    button:hover {
      background: #1565c0;
    }
    button:active {
      transform: scale(0.98);
    }
    button.secondary {
      background: #757575;
      margin-top: 8px;
    }
    button.secondary:hover {
      background: #616161;
    }
    #status {
      margin-top: 16px;
      padding: 12px;
      border-radius: 6px;
      text-align: center;
      font-size: 0.9rem;
      display: none;
    }
    #status.success {
      background: #e8f5e9;
      color: #2e7d32;
      border: 1px solid #81c784;
    }
    #status.error {
      background: #ffebee;
      color: #c62828;
      border: 1px solid #e57373;
    }
    .network-list {
      max-height: 200px;
      overflow-y: auto;
      border: 2px solid #e0e0e0;
      border-radius: 6px;
      margin-top: 6px;
    }
    .network-item {
      padding: 10px 12px;
      border-bottom: 1px solid #f0f0f0;
      cursor: pointer;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: background 0.2s;
    }
    .network-item:hover {
      background: #f5f5f5;
    }
    .network-item:last-child {
      border-bottom: none;
    }
    .network-name {
      font-weight: 500;
    }
    .network-signal {
      font-size: 0.85rem;
      color: #666;
    }
    .spinner {
      border: 3px solid #f3f3f3;
      border-top: 3px solid #1976d2;
      border-radius: 50%;
      width: 24px;
      height: 24px;
      animation: spin 1s linear infinite;
      margin: 0 auto;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    @media (prefers-color-scheme: dark) {
      body {
        background: #121212;
      }
      .container {
        background: #1e1e1e;
        color: #e0e0e0;
      }
      h1 {
        color: #64b5f6;
      }
      .subtitle {
        color: #aaa;
      }
      .info-box {
        background: #1a3a52;
        border-left-color: #64b5f6;
        color: #e0e0e0;
      }
      label {
        color: #e0e0e0;
      }
      input[type="text"],
      input[type="password"],
      select {
        background: #2a2a2a;
        border-color: #555;
        color: #e0e0e0;
      }
      input[type="text"]:focus,
      input[type="password"]:focus,
      select:focus {
        border-color: #64b5f6;
      }
      .network-list {
        border-color: #555;
      }
      .network-item {
        border-bottom-color: #333;
      }
      .network-item:hover {
        background: #2a2a2a;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🧂 Salt Level Monitor</h1>
    <div class="subtitle">WiFi Configuration</div>
    
    <div class="info-box">
      <strong>First-time setup:</strong> Connect your device to your WiFi network to enable monitoring and remote access.
    </div>
    
    <form id="wifiForm">
      <button type="button" id="scanBtn" onclick="scanNetworks()">Scan for Networks</button>
      
      <div id="networkList" style="display:none;">
        <label>Available Networks:</label>
        <div class="network-list" id="networks"></div>
      </div>
      
      <label for="ssid">WiFi Network Name (SSID):</label>
      <input type="text" id="ssid" name="ssid" required placeholder="Enter network name">
      
      <label for="password">WiFi Password:</label>
      <input type="password" id="password" name="password" placeholder="Enter password (leave empty if open)">
      
      <button type="submit">Connect to WiFi</button>
      <button type="button" class="secondary" onclick="window.location.href='/reset'">Reset Credentials</button>
    </form>
    
    <div id="status"></div>
  </div>
  
  <script>
    function showStatus(message, isError) {
      const status = document.getElementById('status');
      status.textContent = message;
      status.className = isError ? 'error' : 'success';
      status.style.display = 'block';
    }
    
    function scanNetworks() {
      const btn = document.getElementById('scanBtn');
      const networkList = document.getElementById('networkList');
      const networks = document.getElementById('networks');
      
      btn.disabled = true;
      btn.textContent = 'Scanning...';
      networks.innerHTML = '<div class="spinner"></div>';
      networkList.style.display = 'block';
      
      fetch('/scan')
        .then(r => r.json())
        .then(data => {
          networks.innerHTML = '';
          if (data.networks && data.networks.length > 0) {
            data.networks.forEach(network => {
              const item = document.createElement('div');
              item.className = 'network-item';
              item.onclick = () => {
                document.getElementById('ssid').value = network.ssid;
                document.getElementById('password').focus();
              };
              
              const name = document.createElement('span');
              name.className = 'network-name';
              name.textContent = network.ssid;
              
              const signal = document.createElement('span');
              signal.className = 'network-signal';
              signal.textContent = network.rssi + ' dBm';
              
              item.appendChild(name);
              item.appendChild(signal);
              networks.appendChild(item);
            });
          } else {
            networks.innerHTML = '<div style="padding:12px;text-align:center;color:#666;">No networks found</div>';
          }
        })
        .catch(err => {
          networks.innerHTML = '<div style="padding:12px;text-align:center;color:#c62828;">Scan failed</div>';
        })
        .finally(() => {
          btn.disabled = false;
          btn.textContent = 'Scan for Networks';
        });
    }
    
    document.getElementById('wifiForm').addEventListener('submit', function(e) {
      e.preventDefault();
      
      const ssid = document.getElementById('ssid').value;
      const password = document.getElementById('password').value;
      
      showStatus('Connecting to WiFi...', false);
      
      fetch('/save', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
      })
      .then(r => r.json())
      .then(data => {
        if (data.success) {
          showStatus('WiFi configured! Device is connecting and will reboot...', false);
          setTimeout(() => {
            showStatus('Please connect to your regular WiFi and access the device at its new IP address.', false);
          }, 3000);
        } else {
          showStatus('Failed to save credentials: ' + (data.message || 'Unknown error'), true);
        }
      })
      .catch(err => {
        showStatus('Connection error: ' + err.message, true);
      });
    });
  </script>
</body>
</html>
)rawliteral";

static void handleProvisioningRoot() {
    provisioningServer->send(200, "text/html", provisioningHTML);
}

static void handleProvisioningScan() {
    Logger::info("WiFi scan requested");
    
    int n = WiFi.scanNetworks();
    
    String json = "{\"networks\":[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i)) + "}";
    }
    json += "]}";
    
    Logger::infof("Found %d networks", n);
    provisioningServer->send(200, "application/json", json);
}

static void handleProvisioningSave() {
    if (!provisioningServer->hasArg("ssid")) {
        provisioningServer->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID\"}");
        return;
    }
    
    String ssid = provisioningServer->arg("ssid");
    String password = provisioningServer->hasArg("password") ? provisioningServer->arg("password") : "";
    
    Logger::infof("Saving WiFi credentials: SSID=%s", ssid.c_str());
    
    if (saveWiFiCredentials(ssid.c_str(), password.c_str())) {
        provisioningServer->send(200, "application/json", "{\"success\":true}");
        
        Logger::info("WiFi credentials saved, restarting in 3 seconds...");
        delay(3000);
        ESP.restart();
    } else {
        provisioningServer->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save\"}");
    }
}

static void handleProvisioningReset() {
    Logger::warn("WiFi credentials reset requested");
    clearWiFiCredentials();
    provisioningServer->send(200, "text/plain", "Credentials cleared. Device will restart.");
    delay(2000);
    ESP.restart();
}

// ---------------------------------------------------------------------------
// Provisioning Mode
// ---------------------------------------------------------------------------
void startWiFiProvisioning() {
    Logger::info("===========================================");
    Logger::info("Starting WiFi Provisioning Mode");
    Logger::info("===========================================");
    
    provisioningMode = true;
    provisioningStartTime = millis();
    
    // Create unique AP name with MAC address
    String apName = "SaltMonitor-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    apName.toUpperCase();
    
    // Start soft AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), nullptr, Network::AP_CHANNEL);  // Open network
    
    IPAddress apIP = WiFi.softAPIP();
    Logger::infof("Access Point started: %s", apName.c_str());
    Logger::infof("AP IP address: %s", apIP.toString().c_str());
    Logger::info("Connect to this network and visit http://192.168.4.1");
    
    // Start DNS server for captive portal
    dnsServer = new DNSServer();
    dnsServer->start(53, "*", apIP);
    
    // Start provisioning web server
    provisioningServer = new WebServer(80);
    provisioningServer->on("/", HTTP_GET, handleProvisioningRoot);
    provisioningServer->on("/scan", HTTP_GET, handleProvisioningScan);
    provisioningServer->on("/save", HTTP_POST, handleProvisioningSave);
    provisioningServer->on("/reset", HTTP_GET, handleProvisioningReset);
    
    // Captive portal redirects
    provisioningServer->onNotFound([]() {
        provisioningServer->sendHeader("Location", "/", true);
        provisioningServer->send(302, "text/plain", "");
    });
    
    provisioningServer->begin();
    Logger::info("Provisioning server started");
    Logger::info("===========================================");
}

bool isWiFiProvisioningNeeded() {
    // Check if WIFI_SSID is defined and not empty in secrets.h
#ifdef WIFI_SSID
    String ssid = String(WIFI_SSID);
    if (ssid.length() > 0 && ssid != "yourssid") {
        Logger::debug("Valid WiFi SSID found in secrets.h");
        return false;
    }
#endif
    
    // Check NVS for stored credentials
    WiFiCredentials creds = loadWiFiCredentials();
    if (creds.isValid) {
        Logger::debug("Valid WiFi credentials found in NVS");
        return false;
    }
    
    Logger::info("No WiFi credentials found - provisioning needed");
    return true;
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Logger::debug("WiFi already connected");
        return;
    }
    
    // Try to get credentials from NVS first
    WiFiCredentials creds = loadWiFiCredentials();
    
    const char* ssid = nullptr;
    const char* password = nullptr;
    
    if (creds.isValid) {
        ssid = creds.ssid;
        password = creds.password;
        Logger::info("Using WiFi credentials from NVS");
    } else {
        // Fallback to secrets.h
#ifdef WIFI_SSID
        String ssidStr = String(WIFI_SSID);
        if (ssidStr.length() > 0 && ssidStr != "yourssid") {
            ssid = WIFI_SSID;
            password = WIFI_PASS;
            Logger::info("Using WiFi credentials from secrets.h");
        }
#endif
    }
    
    if (!ssid) {
        Logger::error("No WiFi credentials available");
        return;
    }
    
    Logger::infof("Connecting to WiFi: %s", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < Network::WIFI_MAX_CONNECT_ATTEMPTS) {
        delay(Network::WIFI_CONNECT_DELAY_MS);
        Serial.print(".");
        attempt++;
        
        if (attempt % 5 == 0) {
            Logger::debugf("WiFi connection attempt %d/%d", attempt, Network::WIFI_MAX_CONNECT_ATTEMPTS);
        }
    }
    
    Serial.println();
    
    if (WiFi.status() != WL_CONNECTED) {
        Logger::error("WiFi connection failed after maximum attempts");
        
        // If these were NVS credentials, they might be wrong - clear them
        if (creds.isValid) {
            Logger::warn("Clearing invalid WiFi credentials from NVS");
            clearWiFiCredentials();
        }
        
        Logger::warn("Restarting device...");
        delay(1000);
        ESP.restart();
    }
    
    Logger::info("WiFi connected successfully");
    Logger::infof("IP address: %s", WiFi.localIP().toString().c_str());
    Logger::infof("Signal strength (RSSI): %d dBm", WiFi.RSSI());
    Logger::infof("MAC address: %s", WiFi.macAddress().c_str());
}

bool initWiFi() {
    if (isWiFiProvisioningNeeded()) {
        startWiFiProvisioning();
        
        // Stay in provisioning mode
        while (provisioningMode) {
            // Reset watchdog timer to prevent timeout
            esp_task_wdt_reset();
            
            dnsServer->processNextRequest();
            provisioningServer->handleClient();
            
            // Timeout check
            if (millis() - provisioningStartTime > Network::PROVISIONING_TIMEOUT_MS) {
                Logger::warn("Provisioning timeout - restarting");
                ESP.restart();
            }
            
            delay(10);
        }
        
        return false;  // Will restart after credentials saved
    } else {
        connectWiFi();
        return true;  // Connected successfully
    }
}

// ---------------------------------------------------------------------------
// Utility Functions
// ---------------------------------------------------------------------------
void disconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("Disconnecting from WiFi...");
        WiFi.disconnect(true);
        delay(100);
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int getWiFiRSSI() {
    return WiFi.RSSI();
}

String getWiFiSSID() {
    return WiFi.SSID();
}

String getLocalIP() {
    return WiFi.localIP().toString();
}