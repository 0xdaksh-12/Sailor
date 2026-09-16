# Sailor

Sailor is a secure, high-performance file transfer protocol and application suite built in C++17 and Flutter. It features TLS-encrypted transport, binary packet framing, SQLite user authentication, a reusable shared library with C ABI (`libsailor.so`), a command-line client, and a dual-pane desktop GUI.

---

## Features

- **End-to-End Encryption**: Mutual TLS (mTLS) session management with OpenSSL.
- **Binary Protocol**: Structured packet serialization with chunked streaming for large files.
- **Shared C ABI (`libsailor.so`)**: C-compatible interface for foreign function interfaces (FFI).
- **CLI Client**: Full-featured command-line utility for automation and scripting.
- **Desktop Application**: Dual-pane file manager built in Flutter using Riverpod, Dart FFI, background worker Isolates, and real-time transfer progress tracking.
- **Directory & File Operations**: List, upload, download, rename, delete, mkdir, and rmdir.

---

## Architecture

### System Flow

```text
  Flutter Desktop UI          CLI Client
          │                        │
          │ (Dart FFI)             │ (C ABI Link)
          ▼                        ▼
  +--------------------------------------------+
  |            libsailor.so (C ABI)            |
  +--------------------------------------------+
                       │
                       │ TLS 1.3 / TCP
                       ▼
  +--------------------------------------------+
  |            Sailor Server (C++17)           |
  +--------------------------------------------+
          │                            │
          ▼                            ▼
  data/users.db (SQLite)        server_storage/
```

### Layered Protocol Stack

```text
+-------------------------------------------------------------+
| Application / Storage Layer                                 |
|   - File Chunking & Streaming (UploadService, DownloadService)|
|   - SHA-256 Checksum Integrity Verification                 |
+-------------------------------------------------------------+
| Session & Authorization Layer                               |
|   - User Authentication (AUTH_REQUEST / AUTH_RESPONSE)       |
|   - Password Hashing (SHA-256 / Argon2id)                   |
|   - State Machine Enforcement & Session Tracking            |
+-------------------------------------------------------------+
| Binary Packet Framing Layer                                 |
|   - PacketHeader (magic, type, payload_size) + Serializer   |
+-------------------------------------------------------------+
| TLS / Security Layer (OpenSSL)    [Handshake & Wire Crypto] |
|   - Key Exchange (X25519 ECDHE via TLS 1.3)                 |
|   - Authenticated Encryption (AES-GCM wire record crypto)   |
+-------------------------------------------------------------+
| Transport Layer                                             |
|   - ITransport (TcpTransport / TlsTransport)                |
+-------------------------------------------------------------+
| Network / Socket Layer                                      |
|   - OS TCP Socket (socket_fd)                               |
+-------------------------------------------------------------+
```

---

## Prerequisites

- CMake 3.16+
- C++17 compiler (`clang++` or `g++`)
- OpenSSL development libraries
- SQLite3
- Ninja or Make
- Flutter SDK (for desktop UI)
- Just (command runner, optional)

---

## Quick Start

### 1. Generate Certificates & Initialize Database

```bash
just certs
just init-db
```

### 2. Build Server and Client

```bash
just build
```

### 3. Start the Server

```bash
just server
```

Default credentials: `admin` / `password123` on port `9000`.

### 4. Run CLI Client

```bash
# List files
./build/sailor-client list /

# Create directory
./build/sailor-client mkdir /documents

# Upload a file
./build/sailor-client upload local_file.txt /documents

# Download a file
./build/sailor-client download /documents/local_file.txt ./downloaded.txt

# Delete a file
./build/sailor-client delete /documents/local_file.txt

# Remove directory
./build/sailor-client rmdir /documents
```

### 5. Run Flutter Desktop Application

```bash
just flutter-run
```

---

## Testing

Run end-to-end integration and Flutter test suites:

```bash
# Run C++ integration test
just test

# Run Flutter tests
just flutter-test
```

---

## Repository Structure

```text
├── auth/            # SQLite user authentication & store
├── certs/           # TLS certificate generation scripts & assets
├── client/          # CLI client application
├── common/          # Socket utilities, hashing, connection state
├── filesystem/      # File services (upload, download, delete, directory)
├── flutter/         # Flutter desktop application
│   ├── lib/core/    # FFI bindings, models, isolate worker client
│   └── lib/features/# UI screens, dual pane explorer, transfer queue
├── protocol/        # Binary packet serialization & framing
├── sdk/             # Public C ABI header (sailor.h) & shared library source
├── server/          # Multi-threaded TCP/TLS server implementation
├── tls/             # OpenSSL context & wrapper
├── transport/       # TCP & TLS stream transport abstractions
├── CMakeLists.txt   # Root build configuration
└── justfile         # Task runner recipes
```
