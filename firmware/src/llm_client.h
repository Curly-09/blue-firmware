#ifndef BLUE_LLM_H
#define BLUE_LLM_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// =====================================================
// LLM Client - Communicates with Ollama
// =====================================================

class LLMClient {
private:
    String baseUrl;
    String model;
    bool isReady;
    
public:
    LLMClient() {
        baseUrl = OLLAMA_BASE_URL;
        model = OLLAMA_MODEL;
        isReady = false;
    }
    
    // Initialize and check connection
    bool begin() {
        // Simple health check
        HTTPClient http;
        http.begin(String(baseUrl) + "/api/tags");
        int code = http.GET();
        http.end();
        
        isReady = (code == 200);
        Serial.println("[LLM] Initialized with model: " + model);
        return isReady;
    }
    
    // Check if LLM is ready
    bool isConnected() {
        return isReady;
    }
    
    // Send chat message and get response
    String chat(String message, String context = "") {
        if (!isReady) {
            return "LLM not connected. Please check configuration.";
        }
        
        HTTPClient http;
        http.begin(String(baseUrl) + "/api/chat");
        http.addHeader("Content-Type", "application/json");
        
        // Build request with system prompt + context
        DynamicJsonDocument doc(4096);
        doc["model"] = model;
        JsonArray messages = doc.createNestedArray("messages");
        
        // System message (Blue's personality)
        JsonObject systemMsg = messages.createNestedObject();
        systemMsg["role"] = "system";
        systemMsg["content"] = BLUE_SYSTEM_PROMPT;
        
        // Add conversation context if available
        if (context.length() > 0) {
            JsonObject contextMsg = messages.createNestedObject();
            contextMsg["role"] = "system";
            contextMsg["content"] = "Previous conversation:\n" + context;
        }
        
        // User message
        JsonObject userMsg = messages.createNestedObject();
        userMsg["role"] = "user";
        userMsg["content"] = message;
        
        doc["stream"] = false;
        doc["options"]["temperature"] = 0.7;
        doc["options"]["num_ctx"] = 4096;
        
        String requestBody;
        serializeJson(doc, requestBody);
        
        Serial.println("[LLM] Sending request...");
        int httpCode = http.POST(requestBody);
        
        String response = "";
        
        if (httpCode == 200) {
            String responseBody = http.getString();
            
            DynamicJsonDocument responseDoc(4096);
            DeserializationError error = deserializeJson(responseDoc, responseBody);
            
            if (!error) {
                response = responseDoc["message"]["content"].as<String>();
            } else {
                response = "Error parsing LLM response.";
            }
        } else {
            response = "LLM error: HTTP " + String(httpCode);
        }
        
        http.end();
        
        Serial.println("[LLM] Response received");
        return response;
    }
    
    // Generate embedding for memory (if supported)
    String generateEmbedding(String text) {
        if (!isReady) return "";
        
        HTTPClient http;
        http.begin(String(baseUrl) + "/api/embeddings");
        http.addHeader("Content-Type", "application/json");
        
        DynamicJsonDocument doc(1024);
        doc["model"] = model;
        doc["prompt"] = text;
        
        String requestBody;
        serializeJson(doc, requestBody);
        
        int httpCode = http.POST(requestBody);
        String result = "";
        
        if (httpCode == 200) {
            String responseBody = http.getString();
            DynamicJsonDocument responseDoc(2048);
            deserializeJson(responseDoc, responseBody);
            result = responseDoc["embedding"].as<String>();
        }
        
        http.end();
        return result;
    }
};

#endif // BLUE_LLM_H