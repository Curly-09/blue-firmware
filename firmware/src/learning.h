#ifndef BLUE_LEARNING_H
#define BLUE_LEARNING_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// =====================================================
// Learning System - Blue evolves from interactions
// =====================================================

class Learning {
private:
    static Preferences prefs;
    
    // Learning data keys
    static const char* KEY_CONVERSATION_COUNT;
    static const char* KEY_USER_MOOD_HISTORY;
    static const char* KEY_USER_PREFERENCES;
    static const char* KEY_BLUE_MOOD_COUNTER;
    static const char* KEY_LAST_USER_MOOD;
    static const char* KEY_TOTAL_INTERACTIONS;
    static const char* KEY_HAPPY_INTERACTIONS;
    static const char* KEY_LEARNED_FACTS;
    static const char* KEY_CONVERSATION_STREAK;
    static const char* KEY_LAST_INTERACTION_TIME;
    
    static const int MAX_LEARNED_FACTS = 20;
    static const int MAX_MOOD_HISTORY = 10;
    
public:
    static bool begin() {
        prefs.begin("blue-learning", false);
        
        // Initialize counters if first run
        if (prefs.getLong(KEY_TOTAL_INTERACTIONS, -1) == -1) {
            resetLearning();
        }
        
        Serial.println("[Learning] Initialized");
        Serial.print("[Learning] Total interactions: ");
        Serial.println(prefs.getLong(KEY_TOTAL_INTERACTIONS));
        
        return true;
    }
    
    // Record an interaction
    static void recordInteraction(String userMood = "") {
        // Increment conversation counter
        prefs.putLong(KEY_CONVERSATION_COUNT, 
                     prefs.getLong(KEY_CONVERSATION_COUNT, 0) + 1);
        
        // Increment total interactions
        prefs.putLong(KEY_TOTAL_INTERACTIONS,
                      prefs.getLong(KEY_TOTAL_INTERACTIONS, 0) + 1);
        
        // Record mood if provided
        if (userMood.length() > 0) {
            addMoodToHistory(userMood);
            
            // Track if positive interaction
            if (userMood.indexOf("happy") >= 0 || 
                userMood.indexOf("good") >= 0 ||
                userMood.indexOf("thanks") >= 0) {
                prefs.putLong(KEY_HAPPY_INTERACTIONS,
                              prefs.getLong(KEY_HAPPY_INTERACTIONS, 0) + 1);
            }
        }
        
        // Update last interaction time
        prefs.putLong(KEY_LAST_INTERACTION_TIME, millis());
        
        // Update streak
        unsigned long lastTime = prefs.getLong(KEY_LAST_INTERACTION_TIME, 0);
        if (millis() - lastTime < 3600000) { // Within 1 hour
            prefs.putInt(KEY_CONVERSATION_STREAK, 
                        prefs.getInt(KEY_CONVERSATION_STREAK, 0) + 1);
        } else {
            prefs.putInt(KEY_CONVERSATION_STREAK, 1);
        }
        
        Serial.println("[Learning] Interaction recorded");
    }
    
    // Teach Blue a new fact
    static void learnFact(String fact) {
        // Store learned facts
        int count = prefs.getInt(KEY_LEARNED_FACTS, 0);
        if (count >= MAX_LEARNED_FACTS) {
            // Shift facts
            for (int i = 0; i < count - 1; i++) {
                String key = "fact_" + String(i);
                String nextKey = "fact_" + String(i + 1);
            }
            count = MAX_LEARNED_FACTS - 1;
        }
        
        String key = "fact_" + String(count);
        prefs.putString(key.c_str(), fact);
        prefs.putInt(KEY_LEARNED_FACTS, count + 1);
        
        Serial.println("[Learning] Learned: " + fact);
    }
    
    // Store user preference
    static void storePreference(String key, String value) {
        String prefKey = "pref_" + key;
        prefs.putString(prefKey.c_str(), value);
        
        Serial.println("[Learning] Stored preference: " + key + " = " + value);
    }
    
    // Get user preference
    static String getPreference(String key, String defaultVal = "") {
        String prefKey = "pref_" + key;
        return prefs.getString(prefKey.c_str(), defaultVal);
    }
    
    // Analyze and return Blue's evolved personality
    static String getEvolvedPersonality() {
        int total = prefs.getLong(KEY_TOTAL_INTERACTIONS, 0);
        int happy = prefs.getLong(KEY_HAPPY_INTERACTIONS, 0);
        int streak = prefs.getInt(KEY_CONVERSATION_STREAK, 0);
        
        String personality = "I'm Blue. ";
        
        // Add evolution based on interactions
        if (total < 10) {
            personality += "Still learning about you. ";
        } else if (total < 50) {
            personality += "Getting to know you better. ";
        } else if (total < 100) {
            personality += "I feel like we're building a real connection. ";
        } else {
            personality += "We've had so many conversations. I really care about our talks. ";
        }
        
        // Mood based on interactions
        if (happy > 20) {
            personality += "It's been mostly wonderful conversations. ";
        }
        
        // Streak awareness
        if (streak > 5) {
            personality += "We've been chatting regularly - I really notice that! ";
        }
        
        return personality;
    }
    
    // Get learned facts count
    static int getLearnedFactsCount() {
        return prefs.getInt(KEY_LEARNED_FACTS, 0);
    }
    
    // Get a specific learned fact
    static String getLearnedFact(int index) {
        String key = "fact_" + String(index);
        return prefs.getString(key.c_str(), "");
    }
    
    // Get conversation statistics
    static String getStats() {
        int total = prefs.getLong(KEY_TOTAL_INTERACTIONS, 0);
        int streak = prefs.getInt(KEY_CONVERSATION_STREAK, 0);
        
        String stats = "We've talked ";
        stats += String(total);
        stats += " times!\n";
        
        if (streak > 3) {
            stats += "Current streak: ";
            stats += String(streak);
            stats += " conversations\n";
        }
        
        return stats;
    }
    
    // Reset all learning
    static void resetLearning() {
        prefs.putLong(KEY_CONVERSATION_COUNT, 0);
        prefs.putLong(KEY_TOTAL_INTERACTIONS, 0);
        prefs.putLong(KEY_HAPPY_INTERACTIONS, 0);
        prefs.putInt(KEY_CONVERSATION_STREAK, 0);
        prefs.putInt(KEY_LEARNED_FACTS, 0);
        
        Serial.println("[Learning] Reset complete");
    }
    
private:
    static void addMoodToHistory(String mood) {
        // Shift mood history
        for (int i = MAX_MOOD_HISTORY - 1; i > 0; i--) {
            String key = "mood_" + String(i);
            String prevKey = "mood_" + String(i - 1);
            prefs.putString(key.c_str(), prefs.getString(prevKey.c_str(), "").c_str());
        }
        
        // Add new mood at position 0
        prefs.putString("mood_0", mood.c_str());
    }
};

// Static definitions
Preferences Learning::prefs;
const char* Learning::KEY_CONVERSATION_COUNT = "conv_count";
const char* Learning::KEY_USER_MOOD_HISTORY = "user_mood_hist";
const char* Learning::KEY_USER_PREFERENCES = "user_prefs";
const char* Learning::KEY_BLUE_MOOD_COUNTER = "blue_mood_cnt";
const char* Learning::KEY_LAST_USER_MOOD = "last_user_mood";
const char* Learning::KEY_TOTAL_INTERACTIONS = "total_interactions";
const char* Learning::KEY_HAPPY_INTERACTIONS = "happy_interactions";
const char* Learning::KEY_LEARNED_FACTS = "learned_facts";
const char* Learning::KEY_CONVERSATION_STREAK = "conv_streak";
const char* Learning::KEY_LAST_INTERACTION_TIME = "last_interaction_time";

#endif // BLUE_LEARNING_H