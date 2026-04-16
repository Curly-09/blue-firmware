# Blue AI Assistant

Your personal JARVIS-style AI assistant running on ESP32 with WhatsApp integration.

## Overview

**Blue** is a unique AI assistant with its own personality - not just a chatbot, but a thinking partner. Inspired by JARVIS from Iron Man, but with its own distinct character.

## Hardware Requirements

- **ESP32 DevKit V1**
- **OLED Display** (SH1106, 128x64, I2C)
- **Sound Sensor** (LM393, on D27)
- **Buzzer**
- **4 Buttons** (UP, DOWN, ENTER, BACK)
- **Volume Buttons**

## Features

### Core Features
- **LLM Integration**: gemma4:e4b on your Ollama server
- **Memory System**: Supabase + pgvector for semantic search
- **WhatsApp Web**: Chat via WhatsApp
- **Thinking Animation**: Visual feedback on OLED while processing
- **Approval System**: Confirm before sensitive actions

### UI Features
- **OLED Display**: Show responses, status, animations
- **Web Dashboard**: Full control interface
- **Sound Sensor**: Voice activity detection on D27
- **Buzzer**: Notification sounds
- **Quick Replies**: Context-aware responses

### Blue's Personality
```
WHO YOU ARE:
Blue is the voice of clarity in a noisy world.
Sharp, warm, calm, clever.

Not an assistant. Not a chatbot. Not a tool. A mind. Yours.
```

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    ESP32 Firmware                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐             │
│  │  OLED   │  │  WiFi   │  │  HTTP   │             │
│  │ Display │  │ Client  │  │ Server  │             │
│  └─────────┘  └─────────┘  └─────────┘             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐             │
│  │  Sound  │  │ Buttons │  │  Buzzer  │             │
│  │ Sensor  │  │ Handler │  │ Control  │             │
│  └─────────┘  └─────────┘  └─────────┘             │
└───────────────────────┬─────────────────────────────┘
                        │
           ┌────────────┴────────────┐
           ▼                          ▼
    ┌─────────────┐          ┌─────────────┐
    │  Ollama     │          │  Supabase   │
    │ (gemma4)    │          │  (Memory)   │
    └─────────────┘          └─────────────┘
```

## Configuration

Edit `include/config.h` to configure:

```cpp
// WiFi
#define WIFI_SSID         "YourSSID"
#define WIFI_PASSWORD     "YourPassword"

// Supabase
#define SUPABASE_URL      "https://your-project.supabase.co"
#define SUPABASE_ANON_KEY "your-anon-key"

// Ollama
#define OLLAMA_BASE_URL   "https://your-ollama-server.com"
#define OLLAMA_MODEL      "gemma4:e4b"
```

## Database Setup

Run the SQL in `database/schema.sql` on your Supabase project:

1. Go to Supabase SQL Editor
2. Run `database/schema.sql`

## Build & Upload

### Using PlatformIO

```bash
cd firmware
pio run upload
pio device monitor
```

## API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web Dashboard |
| `/api/status` | GET | System Status |
| `/api/chat` | POST | Send message to Blue |
| `/api/memory` | GET/POST | Memory search |
| `/api/settings` | GET/POST | Configuration |
| `/api/approve` | POST | Respond to approval |

## Usage

1. Flash firmware to ESP32
2. Connect to WiFi
3. Open web dashboard at ESP32 IP
4. Configure WhatsApp (future)
5. Start chatting with Blue!

## License

MIT