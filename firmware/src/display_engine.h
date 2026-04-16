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
    static Adafruit_SH1106G display;
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
    
    // Initialize display
    static bool begin() {
        Wire.begin(I2C_SDA, I2C_SCL);
        if (!display.begin(OLED_ADDR, true)) {
            Serial.println("[Display] OLED not found!");
            return false;
        }
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        
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
        
        display.clearDisplay();
        
        // Draw mood icon
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.print(getMoodEmoji(currentMood));
        
        // Draw time
        time_t now = time(nullptr);
        struct tm* ti = localtime(&now);
        char timeBuf[20];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", ti);
        
        display.setTextSize(2);
        display.setCursor(40, 0);
        display.print(timeBuf);
        
        // Draw separator line
        display.drawLine(0, 24, 127, 24, 1);
        
        // Draw status indicators
        display.setTextSize(1);
        display.setCursor(0, 30);
        
        // WiFi status
        display.print(WiFi.status() == WL_CONNECTED ? "●" : "○");
        display.print(" WiFi ");
        
        // WhatsApp status
        display.print("● WhatsApp ");
        
        // Uptime
        unsigned long secs = millis() / 1000;
        display.print("up ");
        display.print(secs / 60);
        display.print("m");
        
        // Bottom status - current mood
        display.setCursor(0, 56);
        display.print("Mood: ");
        display.print(currentMood == MOOD_CURIOUS ? "Curious" :
                      currentMood == MOOD_HAPPY ? "Happy" :
                      currentMood == MOOD_THOUGHTFUL ? "Thoughtful" :
                      currentMood == MOOD_HELPFUL ? "Helpful" : "Neutral");
        
        display.display();
        needsRedraw = false;
    }
    
    static void showThinkingAnimation() {
        static int frame = 0;
        static unsigned long lastFrame = 0;
        
        if (millis() - lastFrame < 150) return;
        lastFrame = millis();
        
        display.clearDisplay();
        
        // Thinking text
        display.setTextSize(1);
        display.setCursor(40, 0);
        display.print("Thinking...");
        
        // Animated dots
        int dotY = 32;
        int dotX[3] = {30, 64, 98};
        
        for (int i = 0; i < 3; i++) {
            int active = (frame + i) % 3;
            if (active == 0) {
                display.fillCircle(dotX[i], dotY, 6, 1);
            } else {
                display.drawCircle(dotX[i], dotY, 6, 1);
            }
        }
        
        // Show mood emoji
        display.setTextSize(2);
        display.setCursor(48, 48);
        display.print(getMoodEmoji(MOOD_THOUGHTFUL));
        
        frame = (frame + 1) % 3;
        display.display();
    }
    
    static void showResponseMessage() {
        if (pendingMessage.length() == 0) {
            setState(STATE_IDLE);
            return;
        }
        
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Blue:");
        display.drawLine(0, 10, 127, 10, 1);
        
        // Word wrap message
        display.setCursor(0, 14);
        
        String displayMsg = pendingMessage;
        if (displayMsg.length() > 40) {
            displayMsg = displayMsg.substring(0, 40) + "...";
        }
        
        // For longer messages, show scrolling or truncated
        if (pendingMessage.length() > 128) {
            // Show first part with indicator
            displayMsg = pendingMessage.substring(0, 120);
            display.drawLine(0, 54, 127, 54, 1);
            display.setCursor(100, 56);
            display.print("▼");
        }
        
        display.print(displayMsg);
        
        // Show mood
        display.setTextSize(2);
        display.setCursor(48, 56);
        
        // Mood based on response
        Mood respMood = MOOD_HELPFUL;
        if (pendingMessage.indexOf("!") >= 0) respMood = MOOD_EXCITED;
        else if (pendingMessage.indexOf("?") >= 0) respMood = MOOD_CURIOUS;
        else if (pendingMessage.length() < 20) respMood = MOOD_HAPPY;
        
        display.print(getMoodEmoji(respMood));
        
        display.display();
        
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
        
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(32, 0);
        display.print("Listening...");
        
        // Sound wave animation
        int baseY = 32;
        for (int i = 0; i < 5; i++) {
            int height = random(10, 25);
            display.fillRect(20 + i * 24, baseY - height/2, 15, height, 1);
        }
        
        display.setTextSize(2);
        display.setCursor(48, 50);
        display.print("👂");
        
        frame++;
        display.display();
        
        // Return to idle after timeout
        if (millis() - stateStartTime > 3000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showAlertScreen() {
        display.clearDisplay();
        
        display.setTextSize(2);
        display.setCursor(0, 20);
        display.print("🔔 New!");
        
        display.setTextSize(1);
        display.setCursor(0, 44);
        display.print("Message received");
        
        display.display();
        
        if (millis() - stateStartTime > 3000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showErrorScreen() {
        display.clearDisplay();
        
        display.setTextSize(2);
        display.setCursor(0, 20);
        display.print("❌ Error");
        
        display.setTextSize(1);
        display.setCursor(0, 44);
        display.print(pendingMessage);
        
        display.display();
        
        if (millis() - stateStartTime > 5000) {
            setState(STATE_IDLE);
        }
    }
    
    static void showSleepScreen() {
        display.clearDisplay();
        
        display.setTextSize(2);
        display.setCursor(24, 24);
        display.print("Zzz...");
        
        display.display();
    }
    
    static void showApprovalScreen() {
        display.clearDisplay();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("⚠️ Confirm:");
        display.drawLine(0, 10, 127, 10, 1);
        
        display.setCursor(0, 14);
        
        String displayMsg = pendingMessage;
        if (displayMsg.length() > 80) {
            displayMsg = displayMsg.substring(0, 80) + "...";
        }
        display.print(displayMsg);
        
        display.setCursor(0, 54);
        display.print("Say YES or NO");
        
        display.display();
    }
};

// Static variable definitions
Adafruit_SH1106G DisplayEngine::display(128, 64, &Wire);
bool DisplayEngine::initialized = false;
DisplayEngine::Mood DisplayEngine::currentMood = DisplayEngine::MOOD_CURIOUS;
DisplayEngine::State DisplayEngine::currentState = DisplayEngine::STATE_IDLE;
unsigned long DisplayEngine::stateStartTime = 0;
String DisplayEngine::pendingMessage = "";
bool DisplayEngine::needsRedraw = false;

#endif // BLUE_DISPLAY_ENGINE_H