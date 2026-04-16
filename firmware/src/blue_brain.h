#ifndef BLUE_BRAIN_H
#define BLUE_BRAIN_H

#include <Arduino.h>
#include "config.h"
#include "llm_client.h"
#include "supabase_client.h"
#include "approval_system.h"
#include "whatsapp_client.h"
#include "learning.h"
#include "command_executor.h"
#include "display_engine.h"

// =====================================================
// BLUE BRAIN - Central AI Controller
// =====================================================

class BlueBrain {
private:
    static bool initialized;
    static bool isProcessing;
    static String lastUserMessage;
    static unsigned long lastInteractionTime;
    
    // Message queue for async responses
    static const int MAX_QUEUE = 5;
    static String messageQueue[MAX_QUEUE];
    static int queueWrite;
    static int queueRead;
    
public:
    static bool begin() {
        Serial.println("\n===========================================");
        Serial.println("   BLUE BRAIN INITIALIZING...");
        Serial.println("===========================================");
        
        // Initialize all subsystems
        Learning::begin();
        
        // Initialize display
        DisplayEngine::begin();
        
        // Test display
        DisplayEngine::showMessage("Blue initializing...\nAlmost ready!", 2000);
        delay(500);
        
        initialized = true;
        isProcessing = false;
        
        Serial.println("\n===========================================");
        Serial.println("   BLUE IS NOW ONLINE!");
        Serial.println("===========================================");
        
        return true;
    }
    
    // Process incoming message (main interaction handler)
    static String processMessage(String message, bool showOnOLED = true) {
        if (!initialized) {
            return "Blue not ready. Please wait...";
        }
        
        lastUserMessage = message;
        lastInteractionTime = millis();
        
        Serial.println("[Brain] Processing: " + message);
        
        // Record interaction for learning
        Learning::recordInteraction();
        
        // Show thinking on OLED
        if (showOnOLED) {
            DisplayEngine::setState(DisplayEngine::STATE_THINKING);
            DisplayEngine::setMood(DisplayEngine::MOOD_THOUGHTFUL);
        }
        
        String response = "";
        
        // Check for commands first
        if (isCommand(message)) {
            if (showOnOLED) {
                DisplayEngine::showMessage("Executing command...", 2000);
            }
            response = processCommand(message);
        }
        else {
            // Use LLM for conversation
            response = processWithLLM(message);
        }
        
        // Analyze response and update mood
        analyzeAndUpdateMood(response);
        
        // Show response on OLED if enabled
        if (showOnOLED && response.length() > 0) {
            DisplayEngine::showMessage(response, 5000);
        }
        
        // Learning: learn from interaction
        if (message.startsWith("learn ")) {
            String fact = message.substring(6);
            Learning::learnFact(fact);
            response += "\n\nGot it! I've learned: " + fact;
        }
        
        Serial.println("[Brain] Response: " + response.substring(0, 50) + "...");
        
        return response;
    }
    
    // Process message without OLED (for API calls)
    static String processMessageAPI(String message) {
        return processMessage(message, false);
    }
    
    // Update loop (call in main loop)
    static void update() {
        // Update display engine
        DisplayEngine::update();
        
        // Process any queued messages
        processQueue();
        
        // Check for time-based learning updates
        checkTimeBasedEvents();
    }
    
    // Check if message is a command
    static bool isCommand(String message) {
        message.toLowerCase();
        
        if (message.startsWith("print ")) return true;
        if (message.startsWith("help")) return true;
        if (message.startsWith("status")) return true;
        if (message.startsWith("time")) return true;
        if (message.startsWith("uptime")) return true;
        if (message.startsWith("wifi")) return true;
        if (message.startsWith("memory")) return true;
        if (message.startsWith("mood")) return true;
        if (message.startsWith("learn")) return true;
        if (message.startsWith("hello")) return true;
        if (message.startsWith("clear")) return true;
        
        return false;
    }
    
    // Process command
    static String processCommand(String message) {
        // Extract command part
        String cmd = message;
        if (message.indexOf(' ') > 0) {
            cmd = message.substring(0, message.indexOf(' '));
        }
        cmd.toLowerCase();
        
        // Execute command
        String output = CommandExecutor::executeWithOutput(message, false);
        
        // Show output on OLED
        DisplayEngine::showMessage(output, 3000);
        
        return output;
    }
    
    // Process with LLM
    static String processWithLLM(String message) {
        // Build context with learned personality
        String personality = getPersonalityContext();
        
        // Call LLM
        String response = llm.chat(message, personality);
        
        return response;
    }
    
    // Get evolving personality context
    static String getPersonalityContext() {
        String context = BLUE_SYSTEM_PROMPT;
        
        // Add learned information
        context += "\n\nPERSONAL CONTEXT:\n";
        context += Learning::getEvolvedPersonality();
        
        // Add learned facts if any
        int factCount = Learning::getLearnedFactsCount();
        if (factCount > 0) {
            context += "\nThings I've learned about you:\n";
            for (int i = 0; i < factCount; i++) {
                String fact = Learning::getLearnedFact(i);
                if (fact.length() > 0) {
                    context += "- " + fact + "\n";
                }
            }
        }
        
        // Add conversation stats
        context += "\n" + Learning::getStats();
        
        return context;
    }
    
    // Analyze response and update mood
    static void analyzeAndUpdateMood(String response) {
        // Detect emotion from response
        DisplayEngine::Mood newMood = DisplayEngine::MOOD_NEUTRAL;
        
        if (response.indexOf("!") >= 0) {
            // Exclamation = excited/happy
            newMood = DisplayEngine::MOOD_EXCITED_SPEECH;
        }
        else if (response.indexOf("?") >= 0) {
            // Question = curious
            newMood = DisplayEngine::MOOD_CURIOUS;
        }
        else if (response.length() < 20) {
            // Short response = helpful/quick
            newMood = DisplayEngine::MOOD_HELPFUL;
        }
        else if (response.indexOf("understand") >= 0 ||
                response.indexOf("got it") >= 0) {
            newMood = DisplayEngine::MOOD_THOUGHTFUL;
        }
        else {
            newMood = DisplayEngine::MOOD_HELPFUL;
        }
        
        DisplayEngine::setMood(newMood);
        DisplayEngine::setState(DisplayEngine::STATE_RESPONDING);
    }
    
    // Time-based events
    static void checkTimeBasedEvents() {
        // Morning greeting
        time_t now = time(nullptr);
        struct tm* ti = localtime(&now);
        
        // One-time morning greeting (6-9 AM)
        static bool morningGreetingDone = false;
        if (ti->tm_hour >= 6 && ti->tm_hour <= 9 && !morningGreetingDone) {
            DisplayEngine::showMessage("Good morning! Fresh start today. What shall we explore?", 5000);
            morningGreetingDone = true;
        }
        
        // Idle warning - if no interaction for a while
        if (millis() - lastInteractionTime > 300000) { // 5 minutes
            if (DisplayEngine::currentState == DisplayEngine::STATE_IDLE) {
                // Optionally show idle message
            }
        }
    }
    
    // Queue message for async processing
    static void queueMessage(String message) {
        if (queueWrite - queueRead >= MAX_QUEUE) {
            Serial.println("[Brain] Queue full!");
            return;
        }
        
        messageQueue[queueWrite % MAX_QUEUE] = message;
        queueWrite++;
        Serial.println("[Brain] Queued: " + message);
    }
    
    static void processQueue() {
        if (queueRead >= queueWrite) return;
        
        // Process next in queue
        String msg = messageQueue[queueRead % MAX_QUEUE];
        queueRead++;
        
        processMessage(msg);
    }
};

// Static definitions
bool BlueBrain::initialized = false;
bool BlueBrain::isProcessing = false;
String BlueBrain::lastUserMessage = "";
unsigned long BlueBrain::lastInteractionTime = 0;
int BlueBrain::queueWrite = 0;
int BlueBrain::queueRead = 0;

#endif // BLUE_BRAIN_H