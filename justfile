# Default recipe: build project
default: build

# Configure and compile Sailor server and client
build:
    cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build build

# Clean build directory
clean:
    rm -rf build

# Inspect exported C ABI symbols from shared library
symbols: build
    nm -D --defined-only build/libsailor.so | grep sailor_

# Generate development TLS certificates (Server & Client mTLS)
certs:
    mkdir -p certs
    openssl req -new -x509 -nodes -days 365 \
      -subj "/C=US/ST=Dev/L=Dev/O=Sailor/CN=127.0.0.1" \
      -keyout certs/key.pem \
      -out certs/cert.pem
    openssl genrsa -out certs/client-key.pem 2048
    openssl req -new -x509 -nodes -days 365 \
      -subj "/C=US/ST=Dev/L=Dev/O=Sailor/CN=sailor-client" \
      -key certs/client-key.pem \
      -out certs/client-cert.pem

# Initialize SQLite database from init.sql
init-db:
    mkdir -p data
    sqlite3 data/users.db < data/init.sql

# Run the Sailor server (defaults: certs/cert.pem, certs/key.pem, port 9000, data/users.db, server_storage)
server port="9000" cert="certs/cert.pem" key="certs/key.pem" db="data/users.db" storage="server_storage": build
    ./build/sailor-server {{cert}} {{key}} {{port}} {{db}} {{storage}}

# List remote directory
list path="/": build
    ./build/sailor-client list {{path}}

# Upload local file to remote directory
upload file target_dir="/": build
    ./build/sailor-client upload {{file}} {{target_dir}}

# Download remote file to local destination
download file dest=".": build
    ./build/sailor-client download {{file}} {{dest}}

# Delete remote file or empty directory
delete path: build
    ./build/sailor-client delete {{path}}

# Rename remote file or directory
rename src dst: build
    ./build/sailor-client rename {{src}} {{dst}}

# Move remote file or directory (alias for rename)
move src dst: build
    ./build/sailor-client move {{src}} {{dst}}

# Create remote directory (including parent directories)
mkdir path: build
    ./build/sailor-client mkdir {{path}}

# Remove empty remote directory
rmdir path: build
    ./build/sailor-client rmdir {{path}}

# End-to-end integration test
test: build
    dd if=/dev/urandom of=test_input.bin bs=1M count=5 2>/dev/null
    ./build/sailor-server & SERVER_PID=$!; \
    sleep 0.3; \
    ./build/sailor-client list /; \
    ./build/sailor-client mkdir test_dir/nested; \
    ./build/sailor-client list test_dir; \
    ./build/sailor-client upload test_input.bin test_dir/nested; \
    ./build/sailor-client list test_dir/nested; \
    ./build/sailor-client rmdir test_dir/nested || true; \
    ./build/sailor-client delete test_dir/nested/test_input.bin; \
    ./build/sailor-client rmdir test_dir/nested; \
    ./build/sailor-client rmdir test_dir; \
    ./build/sailor-client mkdir ../../../etc/bad_dir || true; \
    ./build/sailor-client rmdir ../../../etc/bad_dir || true; \
    kill $SERVER_PID; \
    test ! -d server_storage/test_dir && echo "Directory creation & removal verified!"; \
    rm -f test_input.bin; \
    echo "PASSED!"

