#include "WiFiPortal.h"
#include <LittleFS.h>

#define ERROR_LED_GPIO 2  // ESP8266 LED

#define WIFI_CHANNEL 6
#define MAX_CLIENTS 4

// DNS server
const byte DNS_PORT = 53;

bool isValidDomain(String domain) {
    if (domain.length() < 1 || domain.length() > 253) return false;
    if (domain.charAt(0) == '.' || domain.charAt(0) == '-' || domain.charAt(domain.length() - 1) == '.' || domain.charAt(domain.length() - 1) == '-') return false;
    bool hasDot = false;
    String part = "";
    for (int i = 0; i < domain.length(); i++) {
        char c = domain.charAt(i);
        if (c == '.') {
            if (part.length() == 0 || part.charAt(0) == '-' || part.charAt(part.length() - 1) == '-') return false;
            part = "";
            hasDot = true;
        } else if (c == '-') {
            if (part.length() == 0) return false; // can't start with -
            part += c;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            part += c;
        } else {
            return false;
        }
    }
    if (!hasDot || part.length() == 0 || part.charAt(0) == '-' || part.charAt(part.length() - 1) == '-') return false;
    return true;
}

bool isValidPort(String port) {
    if (port.length() == 0 || port.length() > 5) return false;
    for (int i = 0; i < port.length(); i++) {
        if (port.charAt(i) < '0' || port.charAt(i) > '9') return false;
    }
    int portNum = port.toInt();
    return portNum > 0 && portNum <= 65535;
}

WiFiPortal::WiFiPortal(const char* ap_ssid) : _server(80) {
    _ap_ssid = ap_ssid;
    _cachedTemplate = nullptr;
    _cachedTemplateSize = 0;
}

WiFiPortal::~WiFiPortal() {
    if (_cachedTemplate != nullptr) {
        free(_cachedTemplate);
        _cachedTemplate = nullptr;
        _cachedTemplateSize = 0;
    }
}

bool WiFiPortal::run() {
    unsigned long portal_start_time = millis();

    delay(1000);
    LOG_I("Starting WiFi Portal...\n");

    pinMode(ERROR_LED_GPIO, OUTPUT);
    for (int i = 0; i < 10; i++) {
        digitalWrite(ERROR_LED_GPIO, LOW);
        delay(400);
        digitalWrite(ERROR_LED_GPIO, HIGH);
        delay(200);
    }

    _portal_running = true;

    if (!LittleFS.begin()) {
        LOG_E("Failed to mount LittleFS. Halting.\n");
        return false;
    }

    LOG_I("LittleFS mounted.\n");

    setup_ap();

    // Start DNS server
    _dns_server.start(DNS_PORT, "*", WiFi.softAPIP());
    delay(1000);
    LOG_I("DNS server started.\n");

    // Configure web server routes
    _server.on("/", HTTP_GET, [this]() { handle_root(); });
    _server.on("/save", HTTP_POST, [this]() { handle_save(); });
    _server.on("/clear", HTTP_POST, [this]() { handle_clear_credentials(); });

    // Add handlers for common captive portal detection routes
    _server.on("/fwlink", std::bind(&WiFiPortal::handle_root, this));       // Windows

    // Priority 1: Missing critical routes
    _server.on("/connecttest.txt", std::bind(&WiFiPortal::handle_required_pages, this)); // Windows 11 - Critical!
    _server.on("/wpad.dat", std::bind(&WiFiPortal::handle_not_found, this)); // Windows PAC file - prevent loops
    _server.on("/redirect", std::bind(&WiFiPortal::handle_root, this)); // Microsoft Edge
    _server.on("/canonical.html", std::bind(&WiFiPortal::handle_root, this)); // Firefox
    _server.on("/success.txt", std::bind(&WiFiPortal::handle_root, this)); // Firefox
    _server.on("/favicon.ico", std::bind(&WiFiPortal::handle_not_found, this)); // Explicit favicon handler

    // Priority 2: Fix HTTP responses for captive portal detection
    _server.on("/generate_204", [this]() {
        _server.send(204);
    }); // Android - send 204 No Content

    _server.on("/hotspot-detect.html", [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    }); // Apple iOS - send 302 redirect to /

    _server.on("/ncsi.txt", [this]() {
        _server.sendHeader("Content-Type", "text/plain");
        _server.send(200);
    }); // Windows NCSI - send 200 with text/plain

    _server.on("/connecttest.txt", [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    }); // Windows 11 - send 302 redirect to /
    
    _server.serveStatic("/favicon.svg", LittleFS, "/favicon.svg");
    _server.serveStatic("/favicon.png", LittleFS, "/favicon.png");
    _server.serveStatic("/style.css", LittleFS, "/style.css");
    _server.serveStatic("/script.js", LittleFS, "/script.js");
    _server.serveStatic("/success.js", LittleFS, "/success.js");

    _server.on("/scan", HTTP_GET, [&]() {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n; i++) {
            arr.add(WiFi.SSID(i));
        }
        String json;
        serializeJson(doc, json);
        _server.send(200, "application/json", json);

        LOG_I("Served WiFi scan results (%d networks).\n", n);
    });

    _server.onNotFound([this]() { handle_not_found(); });

    _server.begin();
    LOG_I("Web server started.\n");

    // Main portal loop
    while (_portal_running) {
        _dns_server.processNextRequest();
        _server.handleClient();

        // Check if WiFi connected in station mode
        if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
            LOG_I("WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            _portal_running = false;
        }

        // Check if settings were saved and timeout reached
        if (_settingsSaved && (millis() - _settingsSavedTime >= SETTINGS_SAVE_DELAY_MS)) {
            LOG_I("Settings saved and timeout reached. Stopping portal...\n");
            _portal_running = false;
        }

        // Check portal timeout (5 minutes)
        if (millis() - portal_start_time >= 300000) {
            LOG_I("Portal timeout reached (5 minutes). Stopping portal...\n");
            _portal_running = false;
        }

        delay(10);
    }

    // Cleanup
    _server.stop();
    _dns_server.stop();
    WiFi.mode(WIFI_STA);
    LOG_I("Portal stopped.\n");

    return true; // Should return true on success
}

void WiFiPortal::setup_ap() {
    WiFi.mode(WIFI_AP);
    IPAddress apIP(4, 3, 2, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(_ap_ssid, "", WIFI_CHANNEL, 0, MAX_CLIENTS);

    LOG_I("Access Point '%s' started with IP %s\n", _ap_ssid, WiFi.softAPIP().toString().c_str());
}

String makeOption(const String& value, const String& current) {
  String s = "<option value=\"" + value + "\"";
  if (value == current) {
    s += " selected";
  }
  s += ">" + value + "</option>";
  return s;
}

void WiFiPortal::handle_not_found() {
    LOG_D("Handling not found: %s\n", _server.uri().c_str());

    if (is_captive_portal()) {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    } else {
        _server.send(404, "text/plain", "Not Found");
    }
}

void WiFiPortal::handle_required_pages() {
    // This function can be used to handle any required pages for captive portal detection
    _server.send(200, "text/html", "<html><body><h1>WheelB⚙t</h1></body></html>");
}

void WiFiPortal::handle_root() {
    LOG_I("Handling root request...\n");

    String portalContent;

    File portalFile = LittleFS.open("/index.html", "r");
    if (!portalFile) {
        LOG_E("Failed to open index.html\n");
        _server.send(500, "text/plain", "ERROR: Could not load portal page.");
        return;
    }

    portalContent = portalFile.readString();
    portalFile.close();

    LOG_I("Loaded portal page.\n");

    // Load current values from config file
    DynamicJsonDocument doc(512);
    String server_ip = "dev.rightech.io";
    String server_port = "1883";
    String device_id = "";
    String password = "";
    String ssid = "";
    String shunt_resistance = "0.1";

    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
        deserializeJson(doc, configFile);
        configFile.close();
        server_ip = doc["server"] | "dev.rightech.io";
        server_port = doc["server_port"] | "1883";
        device_id = doc["device_id"] | "";
        password = doc["password"] | "";
        ssid = doc["ssid"] | "";
        shunt_resistance = String(doc["shunt_resistance"] | 0.1);
    }

    // Generate default device_id if empty
    if (device_id.length() == 0) {
        device_id = "wheelbot-";
        for (int i = 0; i < 8; i++) {
            char c = random(48, 122); // 0-9, A-Z, a-z
            if (c > 57 && c < 65) c += 7; // skip :;<=>?@
            if (c > 90 && c < 97) c += 6; // skip [\]^_`
            device_id += c;
        }
    }

    // Replace placeholders
    portalContent.replace("{ssid_val}", ssid);
    portalContent.replace("{wifi-password}", password);
    portalContent.replace("{server_ip_val}", server_ip);
    portalContent.replace("{server_port_val}", server_port);
    portalContent.replace("{device_id_val}", device_id);
    portalContent.replace("{shunt_resistance_val}", shunt_resistance);

    LOG_I("Serving portal page.");

    _server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    _server.sendHeader("Pragma", "no-cache");
    _server.sendHeader("Expires", "-1");

    _server.send(200, "text/html", portalContent);

    LOG_I("Root request handled.");
}

void WiFiPortal::handle_save() {
    LOG_I("Handling save request...");

    // Extract data from form
    String ssid = _server.arg("ssid");
    String password = _server.arg("password");
    String server_ip = _server.arg("server");
    String server_port = _server.arg("server_port");
    String device_id = _server.arg("device_id");
    String shunt_resistance_str = _server.arg("shunt_resistance");

    // Validate SSID
    if (ssid.length() == 0) {
        LOG_I("Error: SSID cannot be empty");
        send_error_page("SSID cannot be empty.");
        return;
    }

    if (ssid.length() > 32) {
        LOG_I("Error: SSID too long");
        send_error_page("SSID too long (max 32 characters).");
        return;
    }

    // Validate password
    if (password.length() == 0) {
        LOG_I("Error: Password cannot be empty");
        send_error_page("Password cannot be empty.");
        return;
    }

    if (password.length() > 64) {
        LOG_I("Error: Password too long");
        send_error_page("Password too long (max 64 characters).");
        return;
    }

    // Validate domain and port
    if (!isValidDomain(server_ip)) {
        LOG_I("Error: Invalid domain");
        send_error_page("Invalid domain format. Please enter a valid domain (e.g., dev.rightech.io).");
        return;
    }

    if (!isValidPort(server_port)) {
        LOG_I("Error: Invalid port");
        send_error_page("Invalid port number. Please enter a value between 1 and 65535.");
        return;
    }

    // Validate device_id
    if (device_id.length() == 0) {
        LOG_I("Error: Device ID cannot be empty");
        send_error_page("Device ID cannot be empty.");
        return;
    }

    if (device_id.length() > 50) {
        LOG_I("Error: Device ID too long");
        send_error_page("Device ID too long (max 50 characters).");
        return;
    }

    // Validate shunt resistance
    float shunt_resistance = shunt_resistance_str.toFloat();
    if (shunt_resistance <= 0 || shunt_resistance > 10) {
        LOG_I("Error: Invalid shunt resistance");
        send_error_page("Invalid shunt resistance. Please enter a value between 0.01 and 10 Ohm.");
        return;
    }

    // Save to config file
    DynamicJsonDocument doc(512);
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["server"] = server_ip;
    doc["server_port"] = server_port;
    doc["device_id"] = device_id;
    doc["shunt_resistance"] = shunt_resistance;

    File configFile = LittleFS.open("/config.json", "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
    }

    LOG_I("Credentials saved");
    LOG_I("Server settings - IP: %s:%s, Device ID: %s\n", server_ip.c_str(), server_port.c_str(), device_id.c_str());
    // Set flag that settings were saved
    _settingsSaved = true;
    _settingsSavedTime = millis();

    // Send success page
    String html;
    File successFile = LittleFS.open("/success.html", "r");
    if (successFile) {
        html = successFile.readString();
        
        // Replace {ssid} placeholder
        html.replace("{ssid}", ssid);
        
        successFile.close();
        
        _server.send(200, "text/html", html);
    } else {
        // Fallback if file not found
        String fallbackHtml = "<!DOCTYPE html><html><head><title>Success</title></head>"
            "<body style='color:#aaffaa;background:#000;display:flex;"
            "justify-content:center;align-items:center;height:100vh;"
            "font-family:monospace;font-size:18px;'>"
            "<div style='text-align:center;'>"
            "<div style='font-size:64px;'>✓</div>"
            "<h1>Settings Saved!</h1>"
            "<p>Network: <strong>" + ssid + "</strong></p>"
            "<p>Rebooting in 3 seconds...</p>"
            "</div></body></html>";
        
        _server.send(200, "text/html", fallbackHtml);
    }
}

bool WiFiPortal::is_captive_portal() {
    // A simple check to see if the request is for a specific file or the root
    if (_server.uri().endsWith(".css") || _server.uri().endsWith(".js") || _server.uri().endsWith(".ico")) {
        return false;
    }
    return true;
}

void WiFiPortal::handle_clear_credentials() {
    LOG_I("Clearing WiFi credentials...");

    LittleFS.remove("/config.json");

    WiFi.disconnect();
    delay(1000);

    LOG_I("WiFi credentials cleared. Restarting...");

    _server.send(200, "text/html", "<html><body><h1>Credentials Cleared</h1><p>WiFi credentials have been removed. Restarting...</p></body></html>");

    delay(2000);
    ESP.restart();
}

void WiFiPortal::send_error_page(const String& error_message) {
    String html;
    File errorFile = LittleFS.open("/error.html", "r");
    if (errorFile) {
        html = errorFile.readString();

        html.replace("{error_message}", error_message);

        errorFile.close();

        _server.send(400, "text/html", html);
    } else {
        String fallbackHtml = "<!DOCTYPE html><html><head><title>Error</title></head>"
            "<body style='color:#ffaaaa;background:#000;display:flex;"
            "justify-content:center;align-items:center;height:100vh;"
            "font-family:monospace;font-size:18px;'>"
            "<div style='text-align:center;'>"
            "<div style='font-size:64px;'>✗</div>"
            "<h1>Error</h1>"
            "<p>" + error_message + "</p>"
            "<button onclick='history.back()'>Back</button>"
            "</div></body></html>";

        _server.send(400, "text/html", fallbackHtml);
    }
}
