#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "config.h"
#include "llm_client.h"
#include "supabase_client.h"
#include "approval_system.h"
#include "whatsapp_client.h"
#include "quick_replies.h"
#include "blue_brain.h"

// =====================================================
// GLOBAL VARIABLES
// =====================================================

unsigned long bootTime = 0;
Preferences prefs;

// Web Server
WebServer server(80);

// Local display for WiFi setup (before BlueBrain initializes)
Adafruit_SH1106G display(128, 64, &Wire);

// System State
bool isSleeping = false;

// WiFi Status
String connectedSSID = "";
String connectedIP = "";

// Clients
LLMClient llm;
SupabaseClient supabase;
ApprovalManager approvals;
WhatsAppClient whatsapp;

// Sound Sensor
int soundThreshold = 100;
int soundSampleCount = 20;
int soundSamples[20];
int soundSampleIndex = 0;

// Buttons
bool lastBtnStates[4] = {false, false, false, false};
unsigned long lastBtnTimes[4] = {0, 0, 0, 0};

// =====================================================
// FORWARD DECLARATIONS
// =====================================================

void setupOLED();
void setupWiFi();
void setupServer();
void setupSound();

void handleRoot();
void handleStatus();
void handleChat();
void handleMemory();
void handleSettings();
void handleApprove();
void handleWhatsAppStatus();
void handleWhatsAppSend();
void handleWhatsAppLogin();
void handleWhatsAppLogout();
void handleWhatsAppMessages();

void showBootScreen();
void showThinkingAnimation();
void showResponseScreen(String response);
void showStatusScreen();

void connectBestWiFi();
void connectWiFi();
void checkButtons();
void checkSoundWake();
void handleButtonPress(int btn);
void processLLMRequest(String message);
void checkApprovalRequired(String action);

String getQuickReply(String context);

// =====================================================
// SETUP
// =====================================================

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n===========================================");
    Serial.println("   BLUE AI ASSISTANT v1.0.0");
    Serial.println("===========================================");
    
    // Initialize pins
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_ENTER, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);
    pinMode(VOL_UP, INPUT_PULLUP);
    pinMode(VOL_DOWN, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    pinMode(SOUND_SENSOR, INPUT);
    
    // Initialize OLED FIRST before BlueBrain
    setupOLED();
    showBootScreen();
    
    // Initialize Blue Brain with display pointer
    BlueBrain::begin(&display);
    
    // Connect WiFi (multi-network auto-connect)
    connectBestWiFi();
    
    // Initialize LLM after WiFi is connected
    llm.begin();
    
    // Setup Web Server
    setupServer();
    
    // Mark boot complete
    bootTime = millis();
    
    Serial.println("\n[READY] Blue is fully operational!");
    Serial.println("-----------------------------------");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
    // Handle web server requests
    server.handleClient();
    
    // Update Blue Brain (includes display, learning, commands)
    BlueBrain::update();
    
    // Check buttons
    checkButtons();
    
    // Check sound sensor for wake word
    checkSoundWake();
    
    // Periodic tasks
    static unsigned long lastTaskCheck = 0;
    if (millis() - lastTaskCheck > TASK_CHECK_INTERVAL) {
        lastTaskCheck = millis();
        
        // Check WhatsApp bridge status
        whatsapp.checkBridgeStatus();
        
        // Poll for new WhatsApp messages
        String messages = whatsapp.pollMessages();
        if (messages != "[]" && messages.length() > 2) {
            Serial.println("[WhatsApp] New messages: " + messages);
            
            // Process each message through Blue Brain
            DynamicJsonDocument doc(4096);
            deserializeJson(doc, messages);
            
            JsonArray msgs = doc.as<JsonArray>();
            for (JsonObject msg : msgs) {
                String from = msg["from"].as<String>();
                String text = msg["text"].as<String>();
                
                if (text.length() > 0) {
                    // Process through Blue Brain
                    String response = BlueBrain::processMessageAPI(text);
                    
                    // Send response back via WhatsApp
                    whatsapp.sendMessage(from, response);
                    
                    Serial.println("[WhatsApp] Responded to " + from);
                }
            }
        }
    }
    
    delay(10);
}

// =====================================================
// OLED SETUP & DISPLAY
// =====================================================

void setupOLED() {
    Wire.begin(I2C_SDA, I2C_SCL);
    display.begin(OLED_ADDR, true);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    Serial.println("[OLED] Display initialized");
}

void showBootScreen() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(36, 16);
    display.print("BLUE");
    display.setTextSize(1);
    display.setCursor(44, 36);
    display.print("Starting...");
    
    // Loading animation
    for (int i = 0; i < 5; i++) {
        display.fillRect(20 + (i * 20), 50, 12, 8, 1);
        display.display();
        delay(100);
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.print("BLUE v1.0");
    display.display();
    delay(1000);
}

void showThinkingAnimation() {
    static int frame = 0;
    static unsigned long lastFrame = 0;
    
    if (millis() - lastFrame < THINKING_ANIM_SPEED) return;
    lastFrame = millis();
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(44, 0);
    display.print("Thinking...");
    
    // Animated dots
    int dotPositions[] = {30, 64, 98};
    for (int i = 0; i < 3; i++) {
        int active = (frame + i) % 3;
        if (active == 0) {
            display.fillCircle(dotPositions[i], 32, 6, 1);
        } else {
            display.drawCircle(dotPositions[i], 32, 6, 1);
        }
    }
    
    frame = (frame + 1) % 3;
    display.display();
}

void showResponseScreen(String response) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("BLUE:");
    display.drawLine(0, 10, 127, 10, 1);
    
    // Word wrap
    display.setCursor(0, 14);
    int charsPerLine = 21;
    for (int i = 0; i < response.length(); i += charsPerLine) {
        String line = response.substring(i, min(i + charsPerLine, (int)response.length()));
        display.println(line);
    }
    
    display.display();
    
    // Buzzer notification
    tone(BUZZER, 2000, 100);
}

void showStatusScreen() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("BLUE Status");
    display.drawLine(0, 10, 127, 10, 1);
    
    int y = 14;
    
    // WiFi Status
    display.setCursor(0, y);
    if (WiFi.status() == WL_CONNECTED) {
        display.print("WiFi: Connected");
    } else {
        display.print("WiFi: Disconnected");
    }
    y += 10;
    
    // IP
    display.setCursor(0, y);
    display.print("IP: ");
    display.print(WiFi.localIP().toString());
    y += 10;
    
    // Uptime
    display.setCursor(0, y);
    display.print("Uptime: ");
    int secs = (millis() - bootTime) / 1000;
    display.print(secs / 60);
    display.print("m ");
    display.print(secs % 60);
    display.print("s");
    
    display.display();
}

// =====================================================
// MULTI WiFi SETUP - Auto-connect to any available network
// =====================================================

void connectBestWiFi() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Scanning WiFi...");
    display.display();
    
    Serial.println("[WiFi] Scanning for networks...");
    
    // Scan for available networks
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("[WiFi] No networks found!");
        display.clearDisplay();
        display.setCursor(0, 20);
        display.print("No networks found!");
        display.display();
        delay(2000);
        return;
    }
    
    Serial.println("[WiFi] Found " + String(n) + " networks");
    
    // Try each stored network
    bool connected = false;
    
    for (int retry = 0; retry < WIFI_MAX_RETRIES && !connected; retry++) {
        for (int i = 0; i < WIFI_NETWORKS_COUNT && !connected; i++) {
            const char* ssid = wifiNetworks[i].ssid;
            const char* password = wifiNetworks[i].password;
            
            Serial.println("[WiFi] Trying: " + String(ssid));
            
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Trying:");
            display.println(ssid);
            display.display();
            
            WiFi.begin(ssid, password);
            
            // Wait for connection with timeout
            unsigned long startWait = millis();
            while (WiFi.status() != WL_CONNECTED && 
                   millis() - startWait < WIFI_TIMEOUT) {
                delay(100);
                display.print(".");
                display.display();
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                Serial.println("[WiFi] Connected to: " + String(ssid));
                Serial.println("[WiFi] IP: " + WiFi.localIP().toString());
                Serial.println("[WiFi] Signal: " + String(WiFi.RSSI()) + " dBm");
                
                display.clearDisplay();
                display.setCursor(0, 0);
                display.println("Connected!");
                display.print("SSID: ");
                display.println(ssid);
                display.print("IP: ");
                display.println(WiFi.localIP().toString());
                display.print("Signal: ");
                display.print(WiFi.RSSI());
                display.println(" dBm");
                display.display();
                
                delay(2000);
                break;
            } else {
                Serial.println("[WiFi] Failed: " + String(ssid));
                WiFi.disconnect(true);
                delay(100);
            }
        }
    }
    
    if (!connected) {
        display.clearDisplay();
        display.setCursor(0, 20);
        display.println("WiFi Failed!");
        display.println("Check credentials");
        display.display();
        Serial.println("[WiFi] Could not connect to any stored network");
        delay(2000);
    }
}

// =====================================================
// WEB SERVER SETUP
// =====================================================

void setupServer() {
    // API Endpoints
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/chat", HTTP_POST, handleChat);
    server.on("/api/memory", HTTP_GET, handleMemory);
    server.on("/api/memory/search", HTTP_POST, handleMemory);
    server.on("/api/settings", HTTP_GET, handleSettings);
    server.on("/api/settings", HTTP_POST, handleSettings);
    server.on("/api/approve", HTTP_POST, handleApprove);
    
    // WhatsApp API endpoints
    server.on("/api/whatsapp/status", HTTP_GET, handleWhatsAppStatus);
    server.on("/api/whatsapp/send", HTTP_POST, handleWhatsAppSend);
    server.on("/api/whatsapp/login", HTTP_GET, handleWhatsAppLogin);
    server.on("/api/whatsapp/logout", HTTP_POST, handleWhatsAppLogout);
    server.on("/api/whatsapp/messages", HTTP_GET, handleWhatsAppMessages);
    
    // CORS headers
    server.onNotFound([]() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(404, "application/json", "{\"error\":\"Not found\"}");
    });
    
    server.begin();
    Serial.println("[HTTP] Server started on port 80");
}

// =====================================================
// HTTP HANDLERS
// =====================================================

// =====================================================
// FULL DASHBOARD - OpenClaw Style
// =====================================================

void handleRoot() {
    String html = R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Blue AI - Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', system-ui, sans-serif; 
            background: #0d1117; 
            color: #c9d1d9; 
            min-height: 100vh;
        }
        .header { 
            background: #161b22; 
            padding: 15px 20px; 
            border-bottom: 1px solid #30363d;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .header h1 { color: #58a6ff; font-size: 1.5em; }
        .header .logo { display: flex; align-items: center; gap: 10px; }
        .header .emoji { font-size: 1.5em; }
        
        .nav { 
            background: #161b22; 
            padding: 10px 20px; 
            display: flex; 
            gap: 10px;
            border-bottom: 1px solid #30363d;
            overflow-x: auto;
        }
        .nav a { 
            color: #8b949e; 
            text-decoration: none; 
            padding: 8px 16px; 
            border-radius: 6px;
            white-space: nowrap;
        }
        .nav a:hover, .nav a.active { 
            background: #21262d; 
            color: #58a6ff; 
        }
        
        .container { padding: 20px; max-width: 1200px; margin: 0 auto; }
        
        .grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
            gap: 20px; 
            margin-bottom: 20px;
        }
        
        .card { 
            background: #161b22; 
            border: 1px solid #30363d; 
            border-radius: 6px; 
            padding: 20px;
        }
        .card h2 { 
            color: #f0f6fc; 
            font-size: 1.1em; 
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid #30363d;
        }
        
        .status-item { 
            display: flex; 
            justify-content: space-between; 
            padding: 8px 0;
            border-bottom: 1px solid #21262d;
        }
        .status-item:last-child { border-bottom: none; }
        .status-label { color: #8b949e; }
        .status-value { font-weight: 600; }
        .status-value.online { color: #3fb950; }
        .status-value.offline { color: #f85149; }
        .status-value.warning { color: #d29922; }
        
        .btn { 
            background: #238636; 
            color: #fff; 
            border: none; 
            padding: 10px 20px; 
            border-radius: 6px; 
            cursor: pointer;
            font-size: 14px;
            margin: 5px 5px 5px 0;
            display: inline-block;
        }
        .btn:hover { background: #2ea043; }
        .btn.secondary { background: #21262d; color: #c9d1d9; }
        .btn.secondary:hover { background: #30363d; }
        .btn.danger { background: #da3633; }
        .btn.danger:hover { background: #f85149; }
        
        input, textarea, select { 
            width: 100%; 
            padding: 10px; 
            margin: 5px 0 15px 0;
            background: #0d1117; 
            color: #c9d1d9; 
            border: 1px solid #30363d; 
            border-radius: 6px;
        }
        input:focus, textarea:focus { outline: none; border-color: #58a6ff; }
        
        .log { 
            background: #0d1117; 
            padding: 15px; 
            border-radius: 6px; 
            max-height: 300px; 
            overflow-y: auto;
            font-family: monospace;
            font-size: 12px;
            line-height: 1.6;
        }
        .log-entry { padding: 5px 0; border-bottom: 1px solid #21262d; }
        .log-time { color: #8b949e; margin-right: 10px; }
        .log-msg { color: #c9d1d9; }
        
        .section-title { 
            color: #f0f6fc; 
            font-size: 1.3em; 
            margin: 30px 0 15px 0;
            padding-bottom: 10px;
            border-bottom: 1px solid #30363d;
        }
        
        .form-group { margin-bottom: 15px; }
        .form-group label { display: block; color: #8b949e; margin-bottom: 5px; }
        
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        
        .mood-indicator { 
            font-size: 2em; 
            text-align: center; 
            padding: 20px; 
        }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .connecting { animation: pulse 1.5s infinite; }
        
        .qr-display { 
            text-align: center; 
            padding: 20px; 
            background: #fff; 
            border-radius: 10px; 
            margin: 15px 0;
        }
        .qr-display img { max-width: 250px; }
        
        .message-form { display: flex; gap: 10px; }
        .message-form input { flex: 1; margin: 0; }
        .message-form button { margin: 0; }
    </style>
</head>
<body>
    <div class="header">
        <div class="logo">
            <span class="emoji">🔵</span>
            <h1>Blue AI Assistant</h1>
        </div>
        <div id="mood">😐</div>
    </div>
    
    <nav class="nav">
        <a href="#" class="active" onclick="showTab('dashboard')">📊 Dashboard</a>
        <a href="#" onclick="showTab('whatsapp')">📱 WhatsApp</a>
        <a href="#" onclick="showTab('llm')">🧠 LLM</a>
        <a href="#" onclick="showTab('brain')">🧬 Brain</a>
        <a href="#" onclick="showTab('settings')">⚙️ Settings</a>
        <a href="#" onclick="showTab('logs')">📋 Logs</a>
    </nav>
    
    <div class="container">
        <!-- Dashboard Tab -->
        <div id="dashboard" class="tab-content active">
            <div class="grid">
                <div class="card">
                    <h2>📶 System Status</h2>
                    <div class="status-item">
                        <span class="status-label">Status</span>
                        <span class="status-value online" id="sys-status">● Online</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Uptime</span>
                        <span class="status-value" id="uptime">0s</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">IP Address</span>
                        <span class="status-value" id="ip">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Free Memory</span>
                        <span class="status-value" id="memory">-</span>
                    </div>
                </div>
                
                <div class="card">
                    <h2>🌐 Network</h2>
                    <div class="status-item">
                        <span class="status-label">WiFi</span>
                        <span class="status-value" id="wifi-status">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">SSID</span>
                        <span class="status-value" id="ssid">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Signal</span>
                        <span class="status-value" id="rssi">-</span>
                    </div>
                </div>
                
                <div class="card">
                    <h2>🤖 Blue Brain</h2>
                    <div class="status-item">
                        <span class="status-label">LLM</span>
                        <span class="status-value" id="llm-status">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Model</span>
                        <span class="status-value" id="llm-model">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Interactions</span>
                        <span class="status-value" id="interactions">0</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Current Mood</span>
                        <span class="status-value" id="current-mood">-</span>
                    </div>
                </div>
                
                <div class="card">
                    <h2>📱 WhatsApp Bridge</h2>
                    <div class="status-item">
                        <span class="status-label">Bridge</span>
                        <span class="status-value" id="wa-bridge">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">WhatsApp</span>
                        <span class="status-value" id="wa-status">-</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">Connected Number</span>
                        <span class="status-value" id="wa-number">-</span>
                    </div>
                    <div style="margin-top: 15px;">
                        <button class="btn" onclick="refreshStatus()">🔄 Refresh</button>
                    </div>
                </div>
            </div>
        </div>
        
        <!-- WhatsApp Tab -->
        <div id="whatsapp" class="tab-content">
            <div class="card">
                <h2>📱 WhatsApp Connection</h2>
                <div id="wa-connect-status">
                    <div class="status-item">
                        <span class="status-label">Status</span>
                        <span class="status-value" id="wa-full-status">Checking...</span>
                    </div>
                </div>
                <div id="wa-qr-section" style="display:none;">
                    <p style="margin: 15px 0;">Scan this QR code with WhatsApp:</p>
                    <div class="qr-display">
                        <img id="wa-qr-img" src="" alt="QR Code">
                    </div>
                    <p id="wa-qr-msg" class="connecting">Generating QR...</p>
                </div>
                <div style="margin-top: 15px;">
                    <button class="btn" onclick="refreshWhatsApp()">🔄 Refresh Status</button>
                </div>
            </div>
            
            <div class="card" style="margin-top: 20px;">
                <h2>💬 Send Message (Test)</h2>
                <div class="message-form">
                    <input type="text" id="test-msg" placeholder="Enter test message...">
                    <button class="btn" onclick="sendTestMessage()">Send</button>
                </div>
            </div>
        </div>
        
        <!-- LLM Tab -->
        <div id="llm" class="tab-content">
            <div class="card">
                <h2>🧠 LLM Configuration</h2>
                <div class="form-group">
                    <label>Ollama Server URL</label>
                    <input type="text" id="ollama-url" value=")" HTML + String(OLLAMA_BASE_URL) + R"HTML(" readonly>
                </div>
                <div class="form-group">
                    <label>Model</label>
                    <input type="text" id="ollama-model" value=")" HTML + String(OLLAMA_MODEL) + R"HTML(" readonly>
                </div>
                <div class="status-item">
                    <span class="status-label">Connection</span>
                    <span class="status-value" id="llm-conn">Checking...</span>
                </div>
                <div style="margin-top: 15px;">
                    <button class="btn" onclick="testLLM()">🧪 Test Connection</button>
                </div>
            </div>
        </div>
        
        <!-- Brain Tab -->
        <div id="brain" class="tab-content">
            <div class="card">
                <h2>🧬 Blue Brain & Learning</h2>
                <div class="status-item">
                    <span class="status-label">Total Interactions</span>
                    <span class="status-value" id="brain-total">0</span>
                </div>
                <div class="status-item">
                    <span class="status-label">Happy Interactions</span>
                    <span class="status-value" id="brain-happy">0</span>
                </div>
                <div class="status-item">
                    <span class="status-label">Conversation Streak</span>
                    <span class="status-value" id="brain-streak">0</span>
                </div>
                <div class="status-item">
                    <span class="status-label">Learned Facts</span>
                    <span class="status-value" id="brain-facts">0</span>
                </div>
                <div style="margin-top: 15px;">
                    <button class="btn" onclick="getBrainStats()">📊 Get Stats</button>
                </div>
            </div>
        </div>
        
        <!-- Settings Tab -->
        <div id="settings" class="tab-content">
            <div class="card">
                <h2>⚙️ System Settings</h2>
                <div class="form-group">
                    <label>WhatsApp Bridge URL</label>
                    <input type="text" id="wa-bridge-url" value="https://bluwp.onrender.com">
                </div>
                <div class="form-group">
                    <label>Task Check Interval (seconds)</label>
                    <input type="number" id="task-interval" value="5">
                </div>
                <div style="margin-top: 15px;">
                    <button class="btn" onclick="saveSettings()">💾 Save Settings</button>
                </div>
            </div>
        </div>
        
        <!-- Logs Tab -->
        <div id="logs" class="tab-content">
            <div class="card">
                <h2>📋 System Logs</h2>
                <button class="btn secondary" onclick="clearLogs()">Clear</button>
                <button class="btn secondary" onclick="refreshLogs()">Refresh</button>
                <div class="log" id="system-logs">
                    <div class="log-entry">Waiting for logs...</div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let lastLogCount = 0;
        
        function showTab(tabId) {
            document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.nav a').forEach(a => a.classList.remove('active'));
            document.getElementById(tabId).classList.add('active');
            event.target.classList.add('active');
            
            if (tabId === 'whatsapp') refreshWhatsApp();
            if (tabId === 'llm') testLLM();
            if (tabId === 'brain') getBrainStats();
            if (tabId === 'dashboard') refreshStatus();
        }
        
        async function refreshStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                document.getElementById('ip').textContent = data.wifi?.ip || '-';
                document.getElementById('uptime').textContent = data.uptime ? Math.floor(data.uptime/60) + 'm ' + (data.uptime%60) + 's' : '-';
                document.getElementById('memory').textContent = data.memory?.free ? data.memory.free + ' bytes' : '-';
                
                document.getElementById('wifi-status').textContent = data.wifi?.ssid ? 'Connected' : 'Disconnected';
                document.getElementById('wifi-status').className = 'status-value ' + (data.wifi?.ssid ? 'online' : 'offline');
                document.getElementById('ssid').textContent = data.wifi?.ssid || '-';
                document.getElementById('rssi').textContent = data.wifi?.rssi ? data.wifi.rssi + ' dBm' : '-';
                
                document.getElementById('llm-status').textContent = data.llm?.connected ? 'Connected' : 'Not Connected';
                document.getElementById('llm-status').className = 'status-value ' + (data.llm?.connected ? 'online' : 'offline');
                document.getElementById('llm-model').textContent = data.llm?.model || '-';
                
                document.getElementById('current-mood').textContent = data.mood || '😐';
                document.getElementById('mood').textContent = data.mood || '😐';
                
            } catch (e) {
                console.error('Status error:', e);
            }
        }
        
        async function refreshWhatsApp() {
            try {
                const res = await fetch('https://bluwp.onrender.com/api/status');
                const data = await res.json();
                
                document.getElementById('wa-bridge').textContent = 'Connected';
                document.getElementById('wa-bridge').className = 'status-value online';
                
                document.getElementById('wa-status').textContent = data.connected ? 'Connected' : 'Not Connected';
                document.getElementById('wa-status').className = 'status-value ' + (data.connected ? 'online' : 'warning');
                document.getElementById('wa-number').textContent = data.number || '-';
                
                document.getElementById('wa-full-status').textContent = data.connected ? '✅ Connected as ' + data.number : '❌ Not connected';
                
                if (!data.connected) {
                    document.getElementById('wa-qr-section').style.display = 'block';
                    // Poll for QR
                    checkQR();
                } else {
                    document.getElementById('wa-qr-section').style.display = 'none';
                }
                
            } catch (e) {
                document.getElementById('wa-bridge').textContent = 'Offline';
                document.getElementById('wa-bridge').className = 'status-value offline';
            }
        }
        
        async function checkQR() {
            try {
                const res = await fetch('https://bluwp.onrender.com/qr');
                const data = await res.json();
                
                if (data.qr) {
                    document.getElementById('wa-qr-img').src = data.qr;
                    document.getElementById('wa-qr-msg').textContent = '📸 Scan with WhatsApp';
                    document.getElementById('wa-qr-msg').className = '';
                } else if (data.connected) {
                    document.getElementById('wa-qr-section').style.display = 'none';
                    refreshWhatsApp();
                } else {
                    document.getElementById('wa-qr-msg').textContent = data.message || 'Generating...';
                    setTimeout(checkQR, 2000);
                }
            } catch (e) {
                document.getElementById('wa-qr-msg').textContent = 'Error: ' + e.message;
            }
        }
        
        async function testLLM() {
            try {
                const res = await fetch('/api/chat', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({message: 'test'})
                });
                const data = await res.json();
                document.getElementById('llm-conn').textContent = data.response ? 'Working!' : 'Error';
                document.getElementById('llm-conn').className = 'status-value ' + (data.response ? 'online' : 'offline');
            } catch (e) {
                document.getElementById('llm-conn').textContent = 'Error: ' + e.message;
            }
        }
        
        async function getBrainStats() {
            try {
                const res = await fetch('/api/memory');
                const data = await res.json();
                document.getElementById('brain-total').textContent = data.total || 0;
                document.getElementById('brain-happy').textContent = data.happy || 0;
                document.getElementById('brain-streak').textContent = data.streak || 0;
                document.getElementById('brain-facts').textContent = data.facts || 0;
            } catch (e) {
                console.error('Brain stats error:', e);
            }
        }
        
        async function sendTestMessage() {
            const msg = document.getElementById('test-msg').value;
            if (!msg) return;
            
            try {
                const res = await fetch('https://bluwp.onrender.com/api/send', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({message: msg})
                });
                alert('Message sent!');
            } catch (e) {
                alert('Error: ' + e.message);
            }
        }
        
        function saveSettings() {
            alert('Settings saved!');
        }
        
        function refreshLogs() {
            document.getElementById('system-logs').innerHTML = '<div class="log-entry"><span class="log-time">' + new Date().toLocaleTimeString() + '</span><span class="log-msg">System running normally</span></div>';
        }
        
        function clearLogs() {
            document.getElementById('system-logs').innerHTML = '';
        }
        
        // Auto-refresh dashboard every 10 seconds
        setInterval(refreshStatus, 10000);
        
        // Initial load
        refreshStatus();
    </script>
</body>
</html>
)HTML";
    
    server.send(200, "text/html", html);
}

void handleStatus() {
    DynamicJsonDocument doc(1024);
    doc["status"] = "online";
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["uptime"] = (millis() - bootTime) / 1000;
    doc["memory"]["free"] = ESP.getFreeHeap();
    doc["model"] = OLLAMA_MODEL;
    doc["llm"]["connected"] = llm.isConnected();
    doc["llm"]["model"] = OLLAMA_MODEL;
    doc["mood"] = DisplayEngine::getMoodEmoji(DisplayEngine::currentMood);
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleChat() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No message\"}");
        return;
    }
    
    DynamicJsonDocument incoming(1024);
    deserializeJson(incoming, server.arg("plain"));
    String message = incoming["message"].as<String>();
    
    // Process message through Blue Brain
    String response = BlueBrain::processMessageAPI(message);
    
    DynamicJsonDocument doc(1024);
    doc["response"] = response;
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleMemory() {
    DynamicJsonDocument doc(1024);
    doc["total"] = Learning::getTotalInteractions();
    doc["happy"] = Learning::getHappyInteractions();
    doc["streak"] = Learning::getConversationStreak();
    doc["facts"] = Learning::getLearnedFactsCount();
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleSettings() {
    DynamicJsonDocument doc(1024);
    doc["wake_word_enabled"] = true;
    doc["sound_sensor_enabled"] = true;
    doc["thinking_animation"] = "dots";
    doc["approval_required"] = JsonArray();
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

void handleApprove() {
    DynamicJsonDocument incoming(1024);
    deserializeJson(incoming, server.arg("plain"));
    
    String actionId = incoming["action_id"].as<String>();
    bool approved = incoming["approved"].as<bool>();
    
    DynamicJsonDocument doc(1024);
    doc["status"] = approved ? "approved" : "rejected";
    doc["action_id"] = actionId;
    
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}

// =====================================================
// SOUND SENSOR
// =====================================================

void setupSound() {
    // Initialize sound samples
    for (int i = 0; i < soundSampleCount; i++) {
        soundSamples[i] = 0;
    }
    Serial.println("[Sound] Sensor initialized on D27");
}

void checkSoundWake() {
    int soundValue = analogRead(SOUND_SENSOR);
    
    // Add to rolling average
    soundSamples[soundSampleIndex++] = soundValue;
    if (soundSampleIndex >= soundSampleCount) {
        soundSampleIndex = 0;
    }
    
    // Calculate average
    int sum = 0;
    for (int i = 0; i < soundSampleCount; i++) {
        sum += soundSamples[i];
    }
    int avg = sum / soundSampleCount;
    
    // Detect spike (wake word or loud sound)
    if (soundValue > avg + soundThreshold) {
        Serial.println("[Sound] Sound spike detected!");
        
        // Alert buzzer
        tone(BUZZER, 2000, 50);
        delay(60);
        tone(BUZZER, 2000, 50);
    }
}

// =====================================================
// BUTTON HANDLING
// =====================================================

void checkButtons() {
    int pins[] = {BTN_UP, BTN_DOWN, BTN_ENTER, BTN_BACK};
    unsigned long now = millis();
    
    for (int i = 0; i < 4; i++) {
        bool pressed = (digitalRead(pins[i]) == LOW);
        
        // Debounce
        if (pressed && !lastBtnStates[i] && (now - lastBtnTimes[i]) > 200) {
            lastBtnTimes[i] = now;
            lastBtnStates[i] = pressed;
            
            Serial.print("[Button] Pressed: ");
            Serial.println(i);
            
            // Button feedback
            tone(BUZZER, 1500, 20);
            
            // Handle button press
            handleButtonPress(i);
        } else if (!pressed) {
            lastBtnStates[i] = false;
        }
    }
}

void handleButtonPress(int btn) {
    switch(btn) {
        case 0: // UP - scroll up / quick reply Yes
            break;
        case 1: // DOWN - scroll down / quick reply No
            break;
        case 2: // ENTER - activate Blue / confirm
            showThinkingAnimation();
            break;
        case 3: // BACK - dismiss / go back
            break;
    }
}

// =====================================================
// WHATSAPP HTTP HANDLERS
// =====================================================

void handleWhatsAppStatus() {
    server.send(200, "application/json", whatsapp.getStatusJson());
}

void handleWhatsAppSend() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No data\"}");
        return;
    }
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    String to = doc["to"].as<String>();
    String message = doc["message"].as<String>();
    
    bool result = whatsapp.sendMessage(to, message);
    
    DynamicJsonDocument response(1024);
    response["success"] = result;
    response["queued"] = true;
    
    String output;
    serializeJson(response, output);
    server.send(200, "application/json", output);
}

void handleWhatsAppLogin() {
    String instructions = whatsapp.getLoginInstructions();
    server.send(200, "application/json", instructions);
}

void handleWhatsAppLogout() {
    whatsapp.logout();
    server.send(200, "application/json", "{\"status\":\"logged_out\"}");
}

void handleWhatsAppMessages() {
    String output = whatsapp.getMessagesJson();
    server.send(200, "application/json", output);
}