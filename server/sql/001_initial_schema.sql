-- Initial database schema for Messenger

CREATE TYPE chat_role AS ENUM ('member', 'moderator', 'admin', 'owner');
CREATE TYPE chat_type AS ENUM ('direct', 'group', 'discussion', 'saved', 'channel');

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    handle VARCHAR(32) UNIQUE NOT NULL,      -- @handle for search
    display_name VARCHAR(100) NOT NULL,       -- shown in UI, not unique
    password_hash VARCHAR(255) NOT NULL,
    avatar_path VARCHAR(255),
    last_synced_message_id BIGINT DEFAULT 0,
    description VARCHAR(500) NOT NULL DEFAULT '',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

CREATE TABLE chats (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(100),
    type chat_type NOT NULL,
    avatar_path VARCHAR(255),
    description VARCHAR(500) NOT NULL DEFAULT '',
    direct_user1_id BIGINT REFERENCES users(id),
    direct_user2_id BIGINT REFERENCES users(id),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    handle VARCHAR(32),
    is_private BOOLEAN NOT NULL DEFAULT FALSE,
    discussion_chat_id BIGINT REFERENCES chats(id),
    
    CHECK (direct_user1_id < direct_user2_id)
);


CREATE TABLE chat_members (
    chat_id BIGINT REFERENCES chats(id) ON DELETE CASCADE,
    user_id BIGINT REFERENCES users(id) ON DELETE CASCADE,
    role chat_role NOT NULL DEFAULT 'member',
    last_read_message_id BIGINT, -- FK
    joined_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    chat_type chat_type NOT NULL,
    PRIMARY KEY (chat_id, user_id)
);

CREATE TABLE messages (
    id BIGSERIAL PRIMARY KEY,
    chat_id BIGINT REFERENCES chats(id) ON DELETE CASCADE NOT NULL,
    sender_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    reply_to_message_id BIGINT REFERENCES messages(id) ON DELETE SET NULL,
    forwarded_from_user_id BIGINT REFERENCES users(id) ON DELETE SET NULL,
    forwarded_from_user_name VARCHAR(100), -- Copy of display_name
    text TEXT NOT NULL DEFAULT '',
    sent_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    edited_at TIMESTAMP WITH TIME ZONE, -- NULL if not edited
    discussion_message_id BIGINT REFERENCES messages(id) -- NULL if comments disabled(or not a channel)
    
);


CREATE TABLE attachments (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES messages(id) ON DELETE CASCADE,
    
    -- Metadata
    file_name VARCHAR(255) NOT NULL,  -- original name
    file_type VARCHAR(50) NOT NULL,  -- 'image/jpeg', 'video/mp4', etc
    file_size_bytes BIGINT NOT NULL,
    s3_object_key VARCHAR(500) NOT NULL, -- path in S3(MinIO/Yandex/whatever)
    
    uploaded_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()

);

CREATE TABLE reactions (
    id BIGSERIAL PRIMARY KEY,
    message_id BIGINT NOT NULL REFERENCES messages(id) ON DELETE CASCADE,
    
    user_id BIGINT REFERENCES users(id) ON DELETE CASCADE NOT NULL,
    emoji VARCHAR(64) NOT NULL,
    reacted_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    -- User can't set two of the same emoji on the message.
    -- Works correctly, each constraint blocks duplicates of its type
    UNIQUE (message_id, user_id, emoji)
);

ALTER TABLE chat_members 
ADD CONSTRAINT fk_last_read_message 
FOREIGN KEY (last_read_message_id) REFERENCES messages(id) ON DELETE SET NULL;

-- Indexes for common queries
CREATE INDEX idx_messages_chat_id ON messages(chat_id);
CREATE INDEX idx_messages_sent_at ON messages(sent_at);
CREATE INDEX idx_chat_members_user_id ON chat_members(user_id);
CREATE INDEX idx_attachments_message_id ON attachments(message_id);
CREATE INDEX idx_messages_reply_to ON messages(reply_to_message_id);
CREATE UNIQUE INDEX idx_direct_chat_users ON chats(direct_user1_id, direct_user2_id) WHERE type = 'direct';
CREATE UNIQUE INDEX one_saved_chat_per_user ON chat_members(user_id) WHERE chat_type = 'saved';