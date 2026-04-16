#ifndef BLUE_QUICK_REPLIES_H
#define BLUE_QUICK_REPLIES_H

#include <Arduino.h>

// =====================================================
// Quick Replies Engine - Context-Aware Suggestions
// =====================================================

class QuickReplies {
private:
    struct ReplyOption {
        const char* trigger;
        const char* reply;
        int priority;
    };
    
    // General quick replies
    static const ReplyOption generalReplies[];
    static const int generalCount;
    
    // Approval quick replies
    static const ReplyOption approvalReplies[];
    static const int approvalCount;
    
    // Context-specific quick replies
    static const ReplyOption emailReplies[];
    static const int emailCount;
    
    static const ReplyOption codeReplies[];
    static const int codeCount;
    
    static const ReplyOption searchReplies[];
    static const int searchCount;
    
public:
    // Get quick replies based on context
    static void getReplies(const char* context, String* replies, int& count, int maxCount = 4) {
        count = 0;
        
        // First add approval replies if pending approval
        if (String(context).indexOf("approval") >= 0 || 
            String(context).indexOf("confirm") >= 0 ||
            String(context).indexOf("YES") >= 0 ||
            String(context).indexOf("NO") >= 0) {
            for (int i = 0; i < approvalCount && count < maxCount; i++) {
                replies[count++] = approvalReplies[i].reply;
            }
            return;
        }
        
        // Check for specific contexts
        bool found = false;
        
        // Email context
        if (String(context).indexOf("email") >= 0 || 
            String(context).indexOf("mail") >= 0 ||
            String(context).indexOf("inbox") >= 0) {
            for (int i = 0; i < emailCount && count < maxCount; i++) {
                replies[count++] = emailReplies[i].reply;
            }
            found = true;
        }
        
        // Code context
        if (!found && (String(context).indexOf("code") >= 0 || 
                       String(context).indexOf("program") >= 0 ||
                       String(context).indexOf("function") >= 0)) {
            for (int i = 0; i < codeCount && count < maxCount; i++) {
                replies[count++] = codeReplies[i].reply;
            }
            found = true;
        }
        
        // Search context
        if (!found && (String(context).indexOf("search") >= 0 || 
                       String(context).indexOf("find") >= 0 ||
                       String(context).indexOf("look up") >= 0)) {
            for (int i = 0; i < searchCount && count < maxCount; i++) {
                replies[count++] = searchReplies[i].reply;
            }
            found = true;
        }
        
        // Default to general replies
        if (!found || count == 0) {
            for (int i = 0; i < generalCount && count < maxCount; i++) {
                replies[count++] = generalReplies[i].reply;
            }
        }
    }
    
    // Process quick reply selection
    static String processReply(const char* reply) {
        String r = String(reply);
        r.toUpperCase();
        
        // Approval responses
        if (r.indexOf("YES") >= 0 || r.indexOf("YES,") >= 0) {
            return "APPROVED";
        }
        if (r.indexOf("NO") >= 0 || r.indexOf("NO,") >= 0) {
            return "REJECTED";
        }
        
        // General quick replies - map back to intent
        if (r.indexOf("TELL ME MORE") >= 0) {
            return "EXPAND";
        }
        if (r.indexOf("WHAT") >= 0 || r.indexOf("EXPLAIN") >= 0) {
            return "CLARIFY";
        }
        if (r.indexOf("MORE") >= 0 || r.indexOf("NEXT") >= 0) {
            return "NEXT";
        }
        
        return "CUSTOM:" + String(reply);
    }
    
    // Check if message is a quick reply
    static bool isQuickReply(const char* message) {
        String m = String(message);
        m.toUpperCase();
        
        return m.indexOf("YES") >= 0 || 
               m.indexOf("NO") >= 0 ||
               m.indexOf("TELL ME MORE") >= 0 ||
               m.indexOf("WHAT") >= 0 ||
               m.indexOf("MORE") >= 0 ||
               m.indexOf("NEXT") >= 0 ||
               m.indexOf("SKIP") >= 0 ||
               m.indexOf("SEND") >= 0 ||
               m.indexOf("CHECK") >= 0;
    }
};

// =====================================================
// Quick Reply Definitions
// =====================================================

const QuickReplies::ReplyOption QuickReplies::generalReplies[] = {
    {"", "Yes", 1},
    {"", "No", 2},
    {"", "Tell me more", 3},
    {"", "What?", 4},
};

const int QuickReplies::generalCount = 4;

const QuickReplies::ReplyOption QuickReplies::approvalReplies[] = {
    {"", "YES", 1},
    {"", "NO", 2},
    {"", "ASK AGAIN", 3},
};

const int QuickReplies::approvalCount = 3;

const QuickReplies::ReplyOption QuickReplies::emailReplies[] = {
    {"", "Send email", 1},
    {"", "Check inbox", 2},
    {"", "Draft new", 3},
    {"", "Search emails", 4},
};

const int QuickReplies::emailCount = 4;

const QuickReplies::ReplyOption QuickReplies::codeReplies[] = {
    {"", "Explain", 1},
    {"", "Simplify", 2},
    {"", "Add tests", 3},
    {"", "Optimize", 4},
};

const int QuickReplies::codeCount = 4;

const QuickReplies::ReplyOption QuickReplies::searchReplies[] = {
    {"", "More results", 1},
    {"", "Filter", 2},
    {"", "Open first", 3},
    {"", "Refine", 4},
};

const int QuickReplies::searchCount = 4;

#endif // BLUE_QUICK_REPLIES_H