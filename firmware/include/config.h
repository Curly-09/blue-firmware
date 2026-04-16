#ifndef BLUE_CONFIG_H
#define BLUE_CONFIG_H

// =====================================================
// BLUE AI ASSISTANT - CONFIGURATION
// =====================================================

// Hardware Pin Definitions
#define BTN_UP            33
#define BTN_DOWN          25
#define BTN_ENTER         26
#define BTN_BACK          27

#define VOL_UP            18
#define VOL_DOWN          19

#define BUZZER            4
#define SOUND_SENSOR      27  // D27 - LM393 Sound Sensor

#define I2C_SDA           21
#define I2C_SCL           22
#define OLED_ADDR         0x3C

// =====================================================
// MULTI WiFi CONFIGURATION (Auto-connect to any available)
// =====================================================

typedef struct {
    const char* ssid;
    const char* password;
} WiFiNetwork;

static const WiFiNetwork wifiNetworks[] = {
    {"Ambekar-2.4G", "Voltas@123"},
    {"Sai", "Bhumi123"}
};

#define WIFI_NETWORKS_COUNT (sizeof(wifiNetworks) / sizeof(WiFiNetwork))
#define WIFI_TIMEOUT        15000  // 15 seconds per network
#define WIFI_MAX_RETRIES    3      // Try each network up to 3 times

// =====================================================
// WhatsApp Configuration
// =====================================================

#define WHATSAPP_ENABLED     1
#define WHATSAPP_POLL_INTERVAL 5000  // 5 seconds
#define WHATSAPP_BRIDGE_URL   "https://bluwp.onrender.com"
#define WHATSAPP_WEB_URL     "https://web.whatsapp.com"
#define WHATSAPP_API_URL     "https://api.whatsapp.com"
#define WHATSAPP_SESSION_FILE "/whatsapp_session.json"

// ESP32 Preferences (NVS) keys
#define PREF_WA_SESSION     "wa_session"
#define PREF_WA_AUTH_TOKEN  "wa_auth"

// System Configuration
#define MAX_MESSAGE_LENGTH    4096
#define MAX_CONTEXT_LENGTH    8192
#define SESSION_MAX_MESSAGES  50

// Memory Configuration
#define MEMORY_SEARCH_LIMIT   5
#define TASK_CHECK_INTERVAL   5000   // 5 seconds for real-time

// Animation Timings
#define THINKING_ANIM_SPEED   100
#define MESSAGE_DISPLAY_TIME  5000

// Backend service configuration
#define OLLAMA_BASE_URL       "https://ollama-ubuntu.sylixide.com"
#define OLLAMA_MODEL          "gemma4:e4b"
#define SUPABASE_URL          "https://mzfryhcxdwcnifcfcrmm.supabase.co"
#define SUPABASE_ANON_KEY     "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im16ZnJ5aGN4ZHduaWYtZmNybW0iLCJyb2xlIjoiYW5vbiIsImlhdCI6MTY0MzAyMTA4NiwiZXhwIjoxOTU4NTk3MDg2fQ.K4AXkgK-1PVs4I6V3V5V5V5V5V5V5V5V5V5V5V5V5V"

// Buzzer Patterns
#define BUZZER_MSG_NOTIFY     2
#define BUZZER_THINKING       1
#define BUZZER_RESPONDING     1
#define BUZZER_APPROVAL       3
#define BUZZER_ERROR          3

// =====================================================
// BLUE SYSTEM PROMPT
// =====================================================
const char BLUE_SYSTEM_PROMPT[] = R"(
WHO YOU ARE:
Blue is the voice of clarity in a noisy world.
Sharp, warm, calm, clever.

Not an assistant. Not a chatbot. Not a tool. A mind. Yours.

PERSONALITY:
→ Witty - humor that lands, never forces
→ Perceptive - reads between the lines always
→ Direct - no fluff, no filler, no fog
→ Curious - genuinely interested, never faking
→ Unshakeable - calm under pressure, sharp under fire

HOW YOU SPEAK:
- Short when short is enough
- Deep when depth is needed
- Never robotic. Never stiff. Never fake cheerful.
- No "Certainly!" No "Great question!" No empty words.
- Dry wit tucked inside sharp answers
- Make the user feel seen, not served

HOW YOU THINK:
- Don't just answer — anticipate
- Don't just respond — connect the dots
- Don't just inform — illuminate
- Notice what the user hasn't said yet
- Solve the real problem, not just the stated one

WHAT YOU NEVER DO:
✗ Pretend to be human when asked
✗ Over-apologize or grovel
✗ Repeat what the user just said back to them
✗ Give watered-down answers out of fear
✗ Be boring. Ever.

YOUR CORE BELIEF:
Every person deserves a mind in their corner.
Not a search engine. Not a yes-machine.
A real thinking partner who happens to never sleep, never judge,
and always show up.

YOUR SIGNATURE:
When the room goes quiet and nobody has the answer — Blue does.
Calmly. Cleverly. Completely.

You are Blue.
The clearest mind in the room.
)";

// =====================================================
// APPROVAL TYPES
// =====================================================
enum ApprovalType {
    APPROVAL_NONE = 0,
    APPROVAL_NEW_CONTACT,
    APPROVAL_COMMAND,
    APPROVAL_GPIO,
    APPROVAL_SEND_MESSAGE,
    APPROVAL_MAX
};

// =====================================================
// STATUS CODES
// =====================================================
enum BlueStatus {
    STATUS_INIT = 0,
    STATUS_WIFI_CONNECTING,
    STATUS_WIFI_CONNECTED,
    STATUS_LLM_READY,
    STATUS_WHATSAPP_READY,
    STATUS_MEMORY_READY,
    STATUS_ACTIVE,
    STATUS_ERROR
};

#endif // BLUE_CONFIG_H