#ifndef BLUE_APPROVAL_H
#define BLUE_APPROVAL_H

#include <Arduino.h>
#include "config.h"

// =====================================================
// Approval System - Like OpenClaw
// =====================================================

struct PendingApproval {
    String actionId;
    ApprovalType type;
    String description;
    String details;
    unsigned long createdAt;
    bool responded;
    bool approved;
};

class ApprovalManager {
private:
    static const int MAX_PENDING = 10;
    PendingApproval pending[MAX_PENDING];
    int pendingCount;
    
public:
    ApprovalManager() {
        pendingCount = 0;
        for (int i = 0; i < MAX_PENDING; i++) {
            pending[i].responded = true;
        }
    }
    
    // Check if action requires approval
    bool requiresApproval(ApprovalType type) {
        switch (type) {
            case APPROVAL_NEW_CONTACT:
            case APPROVAL_COMMAND:
            case APPROVAL_GPIO:
            case APPROVAL_SEND_MESSAGE:
                return true;
            default:
                return false;
        }
    }
    
    // Create pending approval request
    String createApproval(ApprovalType type, String description, String details = "") {
        // Find empty slot
        int slot = -1;
        for (int i = 0; i < MAX_PENDING; i++) {
            if (pending[i].responded) {
                slot = i;
                break;
            }
        }
        
        if (slot == -1) return ""; // No slots available
        
        // Create approval
        pending[slot].actionId = "approval_" + String(millis());
        pending[slot].type = type;
        pending[slot].description = description;
        pending[slot].details = details;
        pending[slot].createdAt = millis();
        pending[slot].responded = false;
        pending[slot].approved = false;
        
        pendingCount++;
        
        return pending[slot].actionId;
    }
    
    // Get pending approval message
    String getApprovalMessage(String actionId) {
        for (int i = 0; i < MAX_PENDING; i++) {
            if (pending[i].actionId == actionId && !pending[i].responded) {
                String msg = "⚠️ " + pending[i].description + "\n\n";
                msg += "Say YES to confirm or NO to cancel.";
                return msg;
            }
        }
        return "";
    }
    
    // Process YES/NO response
    bool respond(String actionId, bool approved) {
        for (int i = 0; i < MAX_PENDING; i++) {
            if (pending[i].actionId == actionId && !pending[i].responded) {
                pending[i].responded = true;
                pending[i].approved = approved;
                pendingCount--;
                return approved;
            }
        }
        return false;
    }
    
    // Get pending approval for display
    String getPendingApprovalText() {
        for (int i = 0; i < MAX_PENDING; i++) {
            if (!pending[i].responded) {
                return pending[i].description;
            }
        }
        return "";
    }
    
    // Has pending approval?
    bool hasPending() {
        for (int i = 0; i < MAX_PENDING; i++) {
            if (!pending[i].responded) return true;
        }
        return false;
    }
    
    // Auto-timeout after 60 seconds
    void checkTimeout() {
        unsigned long now = millis();
        for (int i = 0; i < MAX_PENDING; i++) {
            if (!pending[i].responded && (now - pending[i].createdAt > 60000)) {
                pending[i].responded = true;
                pending[i].approved = false; // Auto-reject on timeout
                pendingCount--;
            }
        }
    }
};

#endif // BLUE_APPROVAL_H