-- Sailor SQLite Database Initialization Script
-- Database: data/users.db

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Seed initial users with SHA-256 password hashes
-- admin / password123
INSERT OR IGNORE INTO users (username, password_hash) 
VALUES ('admin', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f');

-- user / password
INSERT OR IGNORE INTO users (username, password_hash) 
VALUES ('user', '5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8');
