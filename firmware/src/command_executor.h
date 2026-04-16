#ifndef BLUE_COMMAND_EXECUTOR_H
#define BLUE_COMMAND_EXECUTOR_H

#include <Arduino.h>
#include "config.h"

// =====================================================
// Command Executor - Runs commands and captures output
// =====================================================

class CommandExecutor {
private:
    struct CommandResult {
        String output;
        bool success;
        unsigned long executionTime;
    };
    
    // Built-in commands
    typedef CommandResult (*CommandHandler)(String args);
    
    struct BuiltInCommand {
        const char* name;
        const char* help;
        CommandHandler handler;
    };
    
    static CommandResult cmdPrint(String args);
    static CommandResult cmdStatus(String args);
    static CommandResult cmdTime(String args);
    static CommandResult cmdMemory(String args);
    static CommandResult cmdHelp(String args);
    static CommandResult cmdClear(String args);
    static CommandResult cmdEcho(String args);
    static CommandResult cmdUptime(String args);
    static CommandResult cmdWifi(String args);
    static CommandResult cmdInfo(String args);
    static CommandResult cmdVersion(String args);
    static CommandResult cmdSettings(String args);
    static CommandResult cmdMood(String args);
    static CommandResult cmdLearn(String args);
    
    static const int MAX_OUTPUT_LENGTH = 512;
    
public:
    // Initialize command system
    static void begin() {
        Serial.println("[Command] Executor initialized");
    }
    
    // Execute a command string
    static CommandResult execute(String command) {
        // Parse command and arguments
        command.trim();
        
        int spaceIdx = command.indexOf(' ');
        String cmd = command;
        String args = "";
        
        if (spaceIdx > 0) {
            cmd = command.substring(0, spaceIdx);
            args = command.substring(spaceIdx + 1);
            args.trim();
        }
        
        cmd.toLowerCase();
        
        unsigned long startTime = millis();
        
        Serial.println("[Command] Executing: " + cmd + " args: " + args);
        
        // Built-in commands
        if (cmd == "print" || cmd == "echo") {
            return {args, true, millis() - startTime};
        }
        else if (cmd == "help") {
            return {getHelpText(), true, millis() - startTime};
        }
        else if (cmd == "status") {
            return getSystemStatus();
        }
        else if (cmd == "time" || cmd == "now") {
            return getTime();
        }
        else if (cmd == "memory" || cmd == "mem") {
            return getMemory();
        }
        else if (cmd == "clear" || cmd == "cls") {
            return {"", true, millis() - startTime};
        }
        else if (cmd == "uptime") {
            return getUptime();
        }
        else if (cmd == "wifi") {
            return getWifiStatus();
        }
        else if (cmd == "info" || cmd == "about") {
            return getInfo();
        }
        else if (cmd == "version" || cmd == "ver") {
            return getVersion();
        }
        else if (cmd == "mood") {
            return getMood();
        }
        else if (cmd == "learn") {
            return learnFromUser(args);
        }
        else if (cmd == "hello") {
            return {"Hello! I'm Blue. Your AI assistant.", true, millis() - startTime};
        }
        else {
            // Unknown command
            return {
                "Unknown command: " + cmd + "\nType 'help' for available commands",
                false,
                millis() - startTime
            };
        }
    }
    
    static String getHelpText() {
        String help = R"(
Blue Commands:
─────────────────────────────────
print <text>  - Print text to display
echo <text>   - Echo text back
hello        - Say hello
status       - Show system status
time         - Show current time
uptime       - Show running time
wifi         - Show WiFi status
memory       - Show memory info
info         - Show device info
version      - Show version
mood         - Show current mood
learn <text> - Teach Blue something
clear        - Clear display
help         - Show this help
─────────────────────────────────
)";
        return help;
    }
    
    // Parse and execute print command with OLED display
    static String executeWithOutput(String command, bool showOnOLED = true) {
        CommandResult result = execute(command);
        
        if (result.success && result.output.length() > 0) {
            // Success output will be shown on OLED by DisplayEngine
            return result.output;
        }
        else if (!result.success) {
            return "Error: " + result.output;
        }
        
        // Handle commands with special output
        if (command.startsWith("print ")) {
            String text = command.substring(6);
            return text;
        }
        
        return result.output;
    }
    
private:
    static CommandResult getSystemStatus() {
        String status = "System Status:\n";
        status += "─\n";
        status += "WiFi: ";
        status += (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
        status += "\n";
        status += "IP: ";
        status += WiFi.localIP().toString();
        status += "\n";
        status += "Memory: ";
        status += String(ESP.getFreeHeap());
        status += " bytes free";
        
        return {status, true, 0};
    }
    
    static CommandResult getTime() {
        String timeStr = "Time: ";
        
        time_t now = time(nullptr);
        struct tm* ti = localtime(&now);
        
        char buf[20];
        strftime(buf, sizeof(buf), "%H:%M:%S", ti);
        timeStr += buf;
        
        return {timeStr, true, 0};
    }
    
    static CommandResult getMemory() {
        String mem = "Memory:\n";
        mem += "─\n";
        mem += "Free RAM: ";
        mem += String(ESP.getFreeHeap());
        mem += " bytes\n";
        mem += "Flash: ";
        mem += String(ESP.getFlashChipSize() / 1024 / 1024);
        mem += " MB";
        
        return {mem, true, 0};
    }
    
    static CommandResult getUptime() {
        unsigned long secs = millis() / 1000;
        int hours = secs / 3600;
        int mins = (secs % 3600) / 60;
        int s = secs % 60;
        
        String uptime = "Uptime: ";
        uptime += String(hours) + "h ";
        uptime += String(mins) + "m ";
        uptime += String(s) + "s";
        
        return {uptime, true, 0};
    }
    
    static CommandResult getWifiStatus() {
        String wifi = "WiFi:\n";
        wifi += "─\n";
        
        if (WiFi.status() == WL_CONNECTED) {
            wifi += "SSID: ";
            wifi += WiFi.SSID();
            wifi += "\n";
            wifi += "IP: ";
            wifi += WiFi.localIP().toString();
            wifi += "\n";
            wifi += "Signal: ";
            wifi += String(WiFi.RSSI());
            wifi += " dBm";
        } else {
            wifi += "Not connected";
        }
        
        return {wifi, true, 0};
    }
    
    static CommandResult getInfo() {
        String info = "Blue AI Assistant\n";
        info += "─\n";
        info += "Chip: ESP32\n";
        info += "Flash: ";
        info += String(ESP.getFlashChipSize() / 1024 / 1024);
        info += " MB\n";
        info += "RAM: ";
        info += String(ESP.getFlashChipSize() / 1024 / 1024);
        info += " MB";
        
        return {info, true, 0};
    }
    
    static CommandResult getVersion() {
        return {"Blue v1.0.0", true, 0};
    }
    
    static CommandResult getMood() {
        // This will integrate with EmotionalState later
        return {"Mood: Curious & Helpful", true, 0};
    }
    
    static CommandResult learnFromUser(String input) {
        // This will integrate with Learning system
        return {"Learned: " + input, true, 0};
    }
};

#endif // BLUE_COMMAND_EXECUTOR_H