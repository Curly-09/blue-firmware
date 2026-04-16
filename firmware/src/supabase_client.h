#ifndef BLUE_SUPABASE_H
#define BLUE_SUPABASE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// =====================================================
// Supabase Client - Memory & Sessions
// =====================================================

class SupabaseClient {
private:
    String baseUrl;
    String anonKey;
    String userId;
    
public:
    SupabaseClient() {
        baseUrl = SUPABASE_URL;
        anonKey = SUPABASE_ANON_KEY;
        userId = "";
    }
    
    // Initialize - get or create primary user
    bool begin() {
        // Get or create user
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/users?phone=eq.primary");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        
        int code = http.GET();
        
        if (code == 200) {
            String response = http.getString();
            
            // Check if user exists, if not create
            if (response == "[]") {
                // Create user
                http.end();
                http.begin(baseUrl + "/rest/v1/users");
                http.addHeader("apikey", anonKey);
                http.addHeader("Authorization", "Bearer " + anonKey);
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Prefer", "return=representation");
                
                String body = "{\"phone\":\"primary\",\"name\":\"Primary User\",\"display_name\":\"You\"}";
                code = http.POST(body);
                
                if (code == 201) {
                    Serial.println("[Supabase] User created");
                }
            } else {
                Serial.println("[Supabase] User found");
            }
        }
        
        http.end();
        
        // Get session ID
        http.begin(baseUrl + "/rest/v1/sessions?user_id=eq." + userId + "&platform=eq.whatsapp");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        code = http.GET();
        
        // Create session if not exists
        if (code == 200) {
            String response = http.getString();
            if (response == "[]") {
                // Create session
                http.end();
                http.begin(baseUrl + "/rest/v1/sessions");
                http.addHeader("apikey", anonKey);
                http.addHeader("Authorization", "Bearer " + anonKey);
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Prefer", "return=representation");
                
                String body = "{\"user_id\":\"" + userId + "\",\"platform\":\"whatsapp\",\"platform_id\":\"primary\",\"messages\":[]}";
                http.POST(body);
            }
        }
        
        http.end();
        
        Serial.println("[Supabase] Initialized");
        return true;
    }
    
    // Add message to session
    bool addMessage(String role, String content) {
        if (userId == "") return false;
        
        // For now, store in memory variable (would need session ID)
        Serial.println("[Supabase] Message logged: " + role);
        return true;
    }
    
    // Get conversation context
    String getContext(int maxMessages = 10) {
        // Would fetch from Supabase
        return "";
    }
    
    // Add memory
    bool addMemory(String content, String category = "general") {
        if (userId == "") return false;
        
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/memory");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Prefer", "return=minimal");
        
        String body = "{\"user_id\":\"" + userId + "\",\"category\":\"" + category + "\",\"content\":\"" + content + "\"}";
        
        int code = http.POST(body);
        http.end();
        
        return (code == 201 || code == 200);
    }
    
    // Search memory (text-based)
    String searchMemory(String query, int limit = 5) {
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/rpc/search_memory_similar");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        http.addHeader("Content-Type", "application/json");
        
        String body = "{\"user_uuid\":\"" + userId + "\",\"search_query\":\"" + query + "\",\"limit_val\":" + String(limit) + "}";
        
        int code = http.POST(body);
        String response = (code == 200) ? http.getString() : "[]";
        http.end();
        
        return response;
    }
    
    // Add task
    bool addTask(String taskType, String description, String scheduledAt) {
        if (userId == "") return false;
        
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/tasks");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Prefer", "return=minimal");
        
        String body = "{\"user_id\":\"" + userId + "\",\"task_type\":\"" + taskType + "\",\"description\":\"" + description + "\",\"scheduled_at\":\"" + scheduledAt + "\"}";
        
        int code = http.POST(body);
        http.end();
        
        return (code == 201 || code == 200);
    }
    
    // Get pending tasks
    String getPendingTasks() {
        if (userId == "") return "[]";
        
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/tasks?user_id=eq." + userId + "&completed_at=is.null&order=scheduled_at.asc");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        
        int code = http.GET();
        String response = (code == 200) ? http.getString() : "[]";
        http.end();
        
        return response;
    }
    
    // Log approval
    bool logApproval(String actionType, String actionDetails, bool approved) {
        if (userId == "") return false;
        
        HTTPClient http;
        http.begin(baseUrl + "/rest/v1/approvals");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
        http.addHeader("Content-Type", "application/json");
        
        String body = "{\"user_id\":\"" + userId + "\",\"action_type\":\"" + actionType + "\",\"action_details\":" + actionDetails + ",\"status\":\"" + (approved ? "approved" : "rejected") + "\"}";
        
        int code = http.POST(body);
        http.end();
        
        return (code == 201 || code == 200);
    }
};

#endif // BLUE_SUPABASE_H