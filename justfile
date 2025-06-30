# Default recipe: build project
default: build

# Configure and compile Sailor server and client
build:
    cmake -B build -S .
    cmake --build build

# Clean build directory
clean:
    rm -rf build

# Generate development TLS certificates
certs:
    mkdir -p certs
    openssl req -new -x509 -nodes -days 365 \
      -subj "/C=US/ST=Dev/L=Dev/O=Sailor/CN=127.0.0.1" \
      -keyout certs/key.pem \
      -out certs/cert.pem

# Initialize SQLite database from init.sql
init-db:
    mkdir -p data
    sqlite3 data/users.db < data/init.sql

# Run the Sailor server (defaults: certs/cert.pem, certs/key.pem, port 9000, data/users.db, server_storage)
server port="9000" cert="certs/cert.pem" key="certs/key.pem" db="data/users.db" storage="server_storage": build
    ./build/sailor-server {{cert}} {{key}} {{port}} {{db}} {{storage}}

# Run the Sailor client (defaults: 127.0.0.1, port 9000, user admin, password password123, path /)
client host="127.0.0.1" port="9000" user="admin" pass="password123" path="/": build
    ./build/sailor-client {{host}} {{port}} {{user}} {{pass}} {{path}}

# Quick end-to-end test: start server, run client to list files, and stop server
run: build
    ./build/sailor-server & SERVER_PID=$!; \
    sleep 0.3; \
    ./build/sailor-client 127.0.0.1 9000 admin password123 /; \
    kill $SERVER_PID
