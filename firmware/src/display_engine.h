#ifndef BLUE_DISPLAY_ENGINE_H
#define BLUE_DISPLAY_ENGINE_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// =====================================================
// Display Engine - Shows Blue's mind on OLED
// =====================================================

class DisplayEngine {
private:
    static Adafruit_SH1106G* displayPtr;
    static bool initialized;
    
public:
    // Blue emotional states
    enum Mood {
        MOOD_NEUTRAL = 0,
        MOOD_HAPPY = 1,
        MOOD_SAD = 2,
        MOOD_EXCITED = 3,
        MOOD_BORED = 4,
        MOOD_CURIOUS = 5,
        MOOD_THOUGHTFUL = 6,
        MOOD_SURPRISED = 7,
        MOOD_TIRED = 8,
        MOOD_EXCITED_SPEECH = 9,
        MOOD_CONFUSED = 10,
        MOOD_HELPFUL = 11
    };
    
    // Display states
    enum State {
        STATE_IDLE = 0,
        STATE_THINKING = 1,
        STATE_RESPONDING = 2,
        STATE_LISTENING = 3,
        STATE_ALERT = 4,
        STATE_ERROR = 5,
        STATE_SLEEPING = 6,
        STATE_LISTENING_SOUND = 7,
        STATE_APPROVAL = 8
    };
    
    static Mood currentMood;
    static State currentState;
    static unsigned long stateStartTime;
    static String pendingMessage;
    static bool needsRedraw;
    
    // Initialize display (using external display pointer from main.cpp)
    static bool begin(Adafruit_SH1106G* extDisplay) {
        displayPtr = extDisplay;
        
        if (!displayPtr->begin(OLED_ADDR, true)) {
            Serial.println("[Display] OLED not found!");
            return false;
        }
        
        displayPtr->clearDisplay();
        displayPtr->setTextSize(1);
        displayPtr->setTextColor(SH110X_WHITE);
        
        initialized = true;
        currentMood = MOOD_CURIOUS;
        currentState = STATE_IDLE;
        stateStartTime = millis();
        needsRedraw = true;
        
        Serial.println("[Display] Engine initialized");
        return true;
    }
    
    // Update display (call in main loop)
    static void update() {
        if (!initialized) return;
        
        // State-specific updates
        switch (currentState) {
            case STATE_IDLE:
                showIdleScreen();
                break;
            case STATE_THINKING:
                showThinkingAnimation();
                break;
            case STATE_RESPONDING:
                showResponseMessage();
                break;
            case STATE_LISTENING:
            case STATE_LISTENING_SOUND:
                showListeningAnimation();
                break;
            case STATE_ALERT:
                showAlertScreen();
                break;
            case STATE_ERROR:
                showErrorScreen();
                break;
            case STATE_SLEEPING:
                showSleepScreen();
                break;
            case STATE_APPROVAL:
                showApprovalScreen();
                break;
        }
    }
    
    // State setters
    static void setState(State newState) {
        if (currentState != newState) {
            currentState = newState;
            stateStartTime = millis();
            needsRedraw = true;
            Serial.print("[Display] State: ");
            Serial.println(newState);
        }
    }
    
    static void setMood(Mood newMood) {
        if (currentMood != newMood) {
            currentMood = newMood;
            needsRedraw = true;
        }
    }
    
    static void showMessage(String msg, int durationMs = 3000) {
        pendingMessage = msg;
        setState(STATE_RESPONDING);
    }
    
    static void showError(String error) {
        pendingMessage = error;
        setState(STATE_ERROR);
    }
    
    static void triggerListening() {
        setState(STATE_LISTENING_SOUND);
    }
    
    static void triggerApproval(String prompt) {
        pendingMessage = prompt;
        setState(STATE_APPROVAL);
    }
    
    // Get mood emoji
    static String getMoodEmoji(Mood mood) {
        switch (mood) {
            case MOOD_NEUTRAL: return "-";
            case MOOD_HAPPY: return ":)";
            case MOOD_SAD: return ":(";
            case MOOD_EXCITED: return "!!";
            case MOOD_BORED: return "..";
            case MOOD_CURIOUS: return "?";
            case MOOD_THOUGHTFUL: return ":";
            case MOOD_SURPRISED: return "o";
            case MOOD_TIRED: return "Z";
            case MOOD_EXCITED_SPEECH: return ">";
            case MOOD_CONFUSED: return "~";
            case MOOD_HELPFUL: return "*";
            default: return "-";
        }
    }
    
private:
    static void showIdleScreen() {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate < 1000 && !needsRedraw) return;
        lastUpdate = millis();
        
        displayPtr->clearDisplay();
        
        // Draw mood icon
        displayPtr->setTextSize(2);
        displayPtr->setCursor(0, 0);
        displayPtr->print(getMoodEmoji(currentMood));
        
        // Draw time
        time_t now = time(nullptr);
        struct tm* ti = localtime(&now);
        char timeBuf[20];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", ti);
        
        displayPtr->setTextSize(2);
        displayPtr->setCursor(40, 0);
        displayPtr->print(timeBuf);
        
        // Draw separator line
        displayPtr->drawLine(0, 24, 127, 24, 1);
        
        // Draw status indicators
        displayPtr->setTextSize(1);
        displayPtr->setCursor(0, 30);
        
        // WiFi status
        displayPtr->print(WiFi.status() == WL_CONNECTED ? "●" : "○");
        displayPtr->print(" WiFi ");
        
        // WhatsApp status
        displayPtr->print("● WhatsApp ");
        
        // Uptime
        unsigned long secs = millis() / 1000;
        displayPtr->print("up ");
        displayPtr->print(secs / 60);
        displayPtr->print("m");
        
        // Bottom status - current mood
        displayPtr->setCursor(0, 56);
        displayPtr->print("Mood: ");
        displayPtr->print(currentMood == MOOD_CURIOUS ? "Curious" :
                      currentMood == MOOD_HAPPY ? "Happy" :
                      currentMood == MOOD_THOUGHTFUL ? "Thoughtful" :
                      currentMood == MOOD_HELPFUL ? "Helpful" : "Neutral");
        
        displayPtr->display();
        needsRedraw = false;
    }
    
    static void showThinkingAnimation() {
        static int frame = 0;
        static unsigned long lastFrame = 0;
        
        if (millis() - lastFrame < 150) return;
        lastFrame = millis();
        
        displayPtr->clearDisplay();
        
        // Thinking text
        displayPtr->setTextSize(1);
        displayPtr->setCursor(40, 0);
        displayPtr->print("Thinking...");
        
        // Animated dots
        int dotY = 32;
        int dotX[3] = {30, 64, 98};
        
        for (int i = 0; i < 3; i++) {
            int active = (frame + i) % 3;
            if (active == 0) {
                displayPtr->fillCircle(dotX[i], dotY, 6, 1);
            } else {
                displayPtr->drawCircle(dotX[i], dotY, 6, 1);
            }
        }
        
        // Show mood emoji
        displayPtr->setTextSize(2);
        displayPtr->setCursor(48, 48);
        displayPtr->print(getMoodEmoji(MOOD_THOUGHTFUL));
        
        frame = (frame + 1) % 3;
        displayPtr->display();
    }
    
    static void showResponseMessage() {
        if (pendingMessage.length() == 0) {
            setState(STATE_IDLE);
            return;
        }
        
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(1);
        displayPtr->setCursor(0, 0);
        displayPtr->print("Blue:");
        displayPtr->drawLine(0, 10, 127, 10, 1);
        
        // Word wrap message
        displayPtr->setCursor(0, 14);
        
        String displayMsg = pendingMessage;
        if (displayMsg.length() > 40) {
            displayMsg = displayMsg.substring(0, 40) + "...";
        }
        
        // For longer messages, show scrolling or truncated
        if (pendingMessage.length() > 128) {
            // Show first part with indicator
            displayMsg = pendingMessage.substring(0, 120);
            displayPtr->drawLine(0, 54, 127, 54, 1);
            displayPtr->setCursor(100, 56);
            displayPtr->print("▼");
        }
        
        displayPtr->print(displayMsg);
        
        // Show mood
        displayPtr->setTextSize(2);
        displayPtr->setCursor(48, 56);
        
        // Mood based on response
        Mood respMood = MOOD_HELPFUL;
        if (pendingMessage.indexOf("!") >= 0) respMood = MOOD_EXCITED;
        else if (pendingMessage.indexOf("?") >= 0) respMood = MOOD_CURIOUS;
        else if (pendingMessage.length() < 20) respMood = MOOD_HAPPY;
        
        displayPtr->print(getMoodEmoji(respMood));
        
        displayPtr->display();
        
        // Auto-return to idle after duration
        if (millis() - stateStartTime > 5000) {
            setState(STATE_IDLE);
            pendingMessage = "";
        }
    }
    
    static void showListeningAnimation() {
        static int frame = 0;
        static unsigned long lastFrame = 0;
        
        if (millis() - lastFrame < 100) return;
        lastFrame = millis();
        
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(1);
        displayPtr->setCursor(32, 0);
        displayPtr->print("Listening...");
        
        // Sound wave animation
        int baseY = 32;
        for (int i = 0; i < 5; i++) {
            int height = random(10, 25);
            displayPtr->fillRect(20 + i * 24, baseY - height/2, 15, height, 1);
        }
        
        displayPtr->setTextSize(2);
        displayPtr->setCursor(48, 50);
        displayPtr->print("👂");
        
        frame++;
        displayPtr->display();
        
        // Return to idle after timeout
        if (millis() - stateStartTime > 3000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showAlertScreen() {
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(2);
        displayPtr->setCursor(0, 20);
        displayPtr->print("🔔 New!");
        
        displayPtr->setTextSize(1);
        displayPtr->setCursor(0, 44);
        displayPtr->print("Message received");
        
        displayPtr->display();
        
        if (millis() - stateStartTime > 3000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showErrorScreen() {
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(2);
        displayPtr->setCursor(0, 20);
        displayPtr->print("❌ Error");
        
        displayPtr->setTextSize(1);
        displayPtr->setCursor(0, 44);
        displayPtr->print(pendingMessage);
        
        displayPtr->display();
        
        if (millis() - stateStartTime > 5000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showSleepScreen() {
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(2);
        displayPtr->setCursor(24, 24);
        displayPtr->print("Zzz...");
        
        displayPtr->display();
    }
    
    static void showApprovalScreen() {
        displayPtr->clearDisplay();
        
        displayPtr->setTextSize(1);
        displayPtr->setCursor(0, 0);
        displayPtr->print("⚠️ Confirm:");
        displayPtr->drawLine(0, 10, 127, 10, 1);
        
        displayPtr->setCursor(0, 14);
        
        String displayMsg = pendingMessage;
        if (displayMsg.length() > 80) {
            displayMsg = displayMsg.substring(0, 80) + "...";
        }
        displayPtr->print(displayMsg);
        
        displayPtr->setCursor(0, 54);
        displayPtr->print("Say YES or NO");
        
        displayPtr->display();
    }
};

// Static variable definitions
Adafruit_SH1106G* DisplayEngine::displayPtr = nullptr;
bool DisplayEngine::initialized = false;
DisplayEngine::Mood DisplayEngine::currentMood = DisplayEngine::MOOD_CURIOUS;
DisplayEngine::State DisplayEngine::currentState = DisplayEngine::STATE_IDLE;
unsigned long DisplayEngine::stateStartTime = 0;
String DisplayEngine::pendingMessage = "";
bool DisplayEngine::needsRedraw = false;

#endif // BLUE_DISPLAY_ENGINE_H