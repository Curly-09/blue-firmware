#ifndef BLUE_WHATSAPP_BRIDGE_H
#define BLUE_WHATSAPP_BRIDGE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include "config.h"

class WhatsAppClient {
private:
    String bridgeURL;
    bool isBridgeConnected;
    bool isWhatsAppConnected;
    String connectedNumber;
    String lastMessageId;
    unsigned long lastPollTime;
    Preferences preferences;

    static const int MAX_QUEUE = 10;
    String sendQueue[MAX_QUEUE];
    int queueHead = 0;
    int queueTail = 0;

public:
    WhatsAppClient() : bridgeURL("https://bluwp.onrender.com") {
        isBridgeConnected = false;
        isWhatsAppConnected = false;
        connectedNumber = "";
        lastMessageId = "";
        lastPollTime = 0;
    }

    bool begin() {
        preferences.begin("blue-wa", false);
        Serial.println("[WhatsApp] Bridge client initialized");
        Serial.print("[WhatsApp] Bridge URL: ");
        Serial.println(bridgeURL);
        return true;
    }

    void setBridgeURL(String url) {
        bridgeURL = url;
    }

    bool checkBridgeStatus() {
        HTTPClient http;
        String url = bridgeURL + "/health";
        
        http.begin(url);
        int code = http.GET();
        
        if (code == 200) {
            String response = http.getString();
            JsonDocument doc;
            deserializeJson(doc, response);
            
            isBridgeConnected = true;
            isWhatsAppConnected = doc["whatsapp"].as<bool>();
            
            Serial.println("[WhatsApp] Bridge: Connected");
            Serial.print("[WhatsApp] WhatsApp: ");
            Serial.println(isWhatsAppConnected ? "Connected" : "Not connected");
            
            http.end();
            return true;
        }
        
        Serial.println("[WhatsApp] Bridge not reachable");
        isBridgeConnected = false;
        http.end();
        return false;
    }

    String getQRCode() {
        if (!isBridgeConnected) {
            checkBridgeStatus();
        }
        
        if (!isBridgeConnected) {
            return "{\"error\": \"Bridge not connected\"}";
        }
        
        HTTPClient http;
        http.begin(bridgeURL + "/qr");
        int code = http.GET();
        
        String response = "";
        if (code == 200) {
            response = http.getString();
        } else {
            response = "{\"error\": \"Failed to get QR\"}";
        }
        
        http.end();
        return response;
    }

    bool isConnected() {
        return isBridgeConnected && isWhatsAppConnected;
    }

    bool sendMessage(String to, String message) {
        if (!isConnected()) {
            Serial.println("[WhatsApp] Not connected - queueing");
            queueMessage(to, message);
            return false;
        }
        
        HTTPClient http;
        http.begin(bridgeURL + "/api/send");
        http.addHeader("Content-Type", "application/json");
        
        JsonDocument doc;
        doc["to"] = to;
        doc["message"] = message;
        
        String body;
        serializeJson(doc, body);
        
        int code = http.POST(body);
        http.end();
        
        if (code == 200) {
            Serial.println("[WhatsApp] Message sent via bridge");
            return true;
        }
        
        Serial.println("[WhatsApp] Failed to send message");
        return false;
    }

    void queueMessage(String to, String message) {
        if (queueHead - queueTail >= MAX_QUEUE) {
            Serial.println("[WhatsApp] Queue full!");
            return;
        }
        
        sendQueue[queueHead % MAX_QUEUE] = to + "|" + message;
        queueHead++;
    }

    void processQueue() {
        if (!isConnected() || queueTail >= queueHead) return;
        
        String item = sendQueue[queueTail % MAX_QUEUE];
        int sep = item.indexOf('|');
        if (sep < 0) return;
        
        String to = item.substring(0, sep);
        String message = item.substring(sep + 1);
        
        sendMessage(to, message);
        queueTail++;
    }

    String pollMessages() {
        if (!isBridgeConnected) {
            checkBridgeStatus();
            return "[]";
        }
        
        if (millis() - lastPollTime < WHATSAPP_POLL_INTERVAL) {
            return "[]";
        }
        lastPollTime = millis();
        
        processQueue();
        
        HTTPClient http;
        http.begin(bridgeURL + "/api/messages");
        int code = http.GET();
        
        String response = "[]";
        if (code == 200) {
            response = http.getString();
        }
        
        http.end();
        return response;
    }

    String getMessagesJson() {
        JsonDocument doc;
        String pollResult = pollMessages();
        
        deserializeJson(doc, doc["messages"].as<JsonArray>(), pollResult);
        doc["bridge_connected"] = isBridgeConnected;
        doc["whatsapp_connected"] = isWhatsAppConnected;
        doc["number"] = connectedNumber;
        
        String output;
        serializeJson(doc, output);
        return output;
    }

    String getStatusJson() {
        JsonDocument doc;
        doc["bridge_connected"] = isBridgeConnected;
        doc["whatsapp_connected"] = isWhatsAppConnected;
        doc["number"] = connectedNumber;
        
        String output;
        serializeJson(doc, output);
        return output;
    }

    String getStatus() {
        if (!isBridgeConnected) return "WhatsApp: Bridge not connected";
        if (!isWhatsAppConnected) return "WhatsApp: Scan QR at /qr";
        return "WhatsApp: Connected as " + connectedNumber;
    }
};

#endif