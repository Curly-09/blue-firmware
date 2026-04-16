-- Blue AI Assistant - Supabase Database Schema
-- Project: https://mzfryhcxdwcnifcfcrmm.supabase.co

-- Enable required extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "vector";

-- =====================================================
-- USERS TABLE
-- =====================================================
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    phone VARCHAR(20) UNIQUE,
    name VARCHAR(100),
    display_name VARCHAR(100),
    preferences JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for phone lookups
CREATE INDEX IF NOT EXISTS idx_users_phone ON users(phone);

-- =====================================================
-- SESSIONS TABLE (Conversation History)
-- =====================================================
CREATE TABLE IF NOT EXISTS sessions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    platform VARCHAR(20) DEFAULT 'whatsapp',
    platform_id VARCHAR(100),
    messages JSONB DEFAULT '[]',
    context JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Indexes for sessions
CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_platform ON sessions(platform, platform_id);

-- =====================================================
-- MEMORY TABLE (Long-term Semantic Memory)
-- =====================================================
CREATE TABLE IF NOT EXISTS memory (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    category VARCHAR(50) DEFAULT 'general',
    content TEXT NOT NULL,
    -- Note: pgvector ivfflat index max 2000 dims, using text search instead
    -- For full vector search, use hnsw index or external service
    importance INTEGER DEFAULT 1,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for memory user lookups
CREATE INDEX IF NOT EXISTS idx_memory_user ON memory(user_id);

-- Text search index for basic semantic matching
CREATE INDEX IF NOT EXISTS idx_memory_content_gin ON memory USING gin(to_tsvector('english', content));

-- =====================================================
-- TASKS TABLE (Cron/Scheduled Tasks)
-- =====================================================
CREATE TABLE IF NOT EXISTS tasks (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    task_type VARCHAR(50),
    description TEXT,
    scheduled_at TIMESTAMPTZ,
    completed_at TIMESTAMPTZ,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for pending tasks
CREATE INDEX IF NOT EXISTS idx_tasks_user ON tasks(user_id);
CREATE INDEX IF NOT EXISTS idx_tasks_scheduled ON tasks(scheduled_at) 
    WHERE completed_at IS NULL;

-- =====================================================
-- APPROVALS TABLE (Approval History)
-- =====================================================
CREATE TABLE IF NOT EXISTS approvals (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    action_type VARCHAR(50) NOT NULL,
    action_details JSONB DEFAULT '{}',
    status VARCHAR(20) DEFAULT 'pending',  -- pending, approved, rejected
    created_at TIMESTAMPTZ DEFAULT NOW(),
    responded_at TIMESTAMPTZ
);

-- Index for approval lookup
CREATE INDEX IF NOT EXISTS idx_approvals_user ON approvals(user_id);
CREATE INDEX IF NOT EXISTS idx_approvals_status ON approvals(status);

-- =====================================================
-- SETTINGS TABLE (User Configuration)
-- =====================================================
CREATE TABLE IF NOT EXISTS settings (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    key VARCHAR(100) NOT NULL,
    value JSONB,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(user_id, key)
);

-- Index for settings lookup
CREATE INDEX IF NOT EXISTS idx_settings_user ON settings(user_id);

-- =====================================================
-- CONNECTION LOGS (Status Tracking)
-- =====================================================
CREATE TABLE IF NOT EXISTS connection_logs (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    event_type VARCHAR(50) NOT NULL,
    event_data JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- Index for connection logs
CREATE INDEX IF NOT EXISTS idx_connection_logs_user ON connection_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_connection_logs_time ON connection_logs(created_at);

-- =====================================================
-- ROW LEVEL SECURITY (RLS)
-- =====================================================
ALTER TABLE users ENABLE ROW LEVEL SECURITY;
ALTER TABLE sessions ENABLE ROW LEVEL SECURITY;
ALTER TABLE memory ENABLE ROW LEVEL SECURITY;
ALTER TABLE tasks ENABLE ROW LEVEL SECURITY;
ALTER TABLE approvals ENABLE ROW LEVEL SECURITY;
ALTER TABLE settings ENABLE ROW LEVEL SECURITY;
ALTER TABLE connection_logs ENABLE ROW LEVEL SECURITY;

-- Allow anon key to perform operations (for ESP32)
CREATE POLICY "Allow all for anon" ON users FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON sessions FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON memory FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON tasks FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON approvals FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON settings FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "Allow all for anon" ON connection_logs FOR ALL USING (true) WITH CHECK (true);

-- =====================================================
-- HELPER FUNCTIONS
-- =====================================================

-- Get or create user by phone
CREATE OR REPLACE FUNCTION get_or_create_user(user_phone VARCHAR)
RETURNS UUID AS $$
DECLARE
    user_id UUID;
BEGIN
    SELECT id INTO user_id FROM users WHERE phone = user_phone;
    IF user_id IS NULL THEN
        INSERT INTO users (phone, name, display_name)
        VALUES (user_phone, user_phone, 'User')
        RETURNING id INTO user_id;
    END IF;
    RETURN user_id;
END;
$$ LANGUAGE plpgsql;

-- Get or create session for user
CREATE OR REPLACE FUNCTION get_or_create_session(user_uuid UUID, platform_name VARCHAR, platform_id_val VARCHAR)
RETURNS UUID AS $$
DECLARE
    session_id UUID;
BEGIN
    SELECT id INTO session_id FROM sessions 
    WHERE user_id = user_uuid AND platform = platform_name AND platform_id = platform_id_val;
    IF session_id IS NULL THEN
        INSERT INTO sessions (user_id, platform, platform_id, messages)
        VALUES (user_uuid, platform_name, platform_id_val, '[]')
        RETURNING id INTO session_id;
    END IF;
    RETURN session_id;
END;
$$ LANGUAGE plpgsql;

-- Add message to session
CREATE OR REPLACE FUNCTION add_message_to_session(session_uuid UUID, role_val VARCHAR, content_val TEXT)
RETURNS VOID AS $$
BEGIN
    UPDATE sessions 
    SET messages = messages::jsonb || jsonb_build_array(
        jsonb_build_object('role', role_val, 'content', content_val, 'timestamp', NOW()::text)
    ),
    updated_at = NOW()
    WHERE id = session_uuid;
END;
$$ LANGUAGE plpgsql;

-- Search memory by similarity (text-based fallback since vector has dim limit)
CREATE OR REPLACE FUNCTION search_memory_similar(user_uuid UUID, search_query TEXT, limit_val INTEGER DEFAULT 5)
RETURNS TABLE(id UUID, content TEXT, importance INTEGER, created_at TIMESTAMPTZ) AS $$
BEGIN
    RETURN QUERY
    SELECT m.id, m.content, m.importance, m.created_at
    FROM memory m
    WHERE m.user_id = user_uuid
      AND (
        m.content ILIKE '%' || search_query || '%'
        OR m.category ILIKE '%' || search_query || '%'
      )
    ORDER BY m.importance DESC, m.created_at DESC
    LIMIT limit_val;
END;
$$ LANGUAGE plpgsql;

-- Note: Full vector semantic search requires embedding generation server-side
-- or using a different vector DB that supports higher dimensions

-- =====================================================
-- INITIAL DATA
-- =====================================================

-- Insert default user (for WhatsApp primary user)
INSERT INTO users (phone, name, display_name, preferences)
VALUES ('primary', 'Primary User', 'You', 
    '{"language": "en", "theme": "default", "sound_enabled": true, "auto_approve": false}')
ON CONFLICT (phone) DO NOTHING;

-- Insert default session for primary user
INSERT INTO sessions (user_id, platform, platform_id, messages)
SELECT id, 'whatsapp', 'primary', '[]'
FROM users WHERE phone = 'primary'
ON CONFLICT DO NOTHING;

-- Insert default settings
INSERT INTO settings (user_id, key, value)
SELECT id, 'blue_config', '{"wake_word": true, "sound_sensor": true, "thinking_animation": "dots", "approval_required": ["new_contact", "command", "gpio"]}'
FROM users WHERE phone = 'primary'
ON CONFLICT DO NOTHING;