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

void handleRoot() {
    String html = R"PAGE(<!DOCTYPE html>
<html>
<head>
    <title>Blue AI Assistant</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: monospace; padding: 20px; background: #1a1a2e; color: #eee; }
        h1 { color: #00d4ff; }
        .card { background: #16213e; padding: 15px; margin: 10px 0; border-radius: 8px; }
        .status { color: #00ff88; }
        .btn { background: #00d4ff; color: #000; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }
        input, textarea { width: 100%; padding: 10px; margin: 5px 0; background: #0f0f23; color: #fff; border: 1px solid #333; }
    </style>
</head>
<body>
    <h1>🤖 Blue AI Assistant</h1>
    <div class="card">
        <h3>Status</h3>
        <p class="status">● Online</p>
        <p>IP: )PAGE" + WiFi.localIP().toString() + R"PAGE(</p>
        <p>Uptime: <span id="uptime">0</span>s</p>
    </div>
    <div class="card">
        <h3>Chat</h3>
        <textarea id="message" placeholder="Type your message..." rows="3"></textarea>
        <button class="btn" onclick="sendMessage()">Send</button>
        <pre id="response" style="background: #0f0f23; padding: 10px; margin-top: 10px;"></pre>
    </div>
    <div class="card">
        <h3>Quick Actions</h3>
        <button class="btn" onclick="showStatus()">Status</button>
        <button class="btn" onclick="showMemory()">Memory</button>
    </div>
    <script>
        function sendMessage() {
            fetch('/api/chat', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({message: document.getElementById('message').value})
            })
            .then(r => r.json())
            .then(d => {
                document.getElementById('response').textContent = d.response || d.error;
                document.getElementById('message').value = '';
            });
        }
        setInterval(() => {
            document.getElementById('uptime').innerText = Math.floor(Date.now()/1000);
        }, 1000);
    </script>
</body>
</html>)PAGE";
    
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
    doc["memories"] = JsonArray();
    doc["count"] = 0;
    
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