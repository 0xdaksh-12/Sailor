#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 4096

int main() {
  // socket() returns -1 on failure, never checked before
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);

  // inet_pton() returns 1 on success, 0 for invalid address, -1 on error
  int rc = inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
  if (rc == 0) {
    std::cerr << "inet_pton: invalid address format" << std::endl;
    close(sock);
    exit(EXIT_FAILURE);
  } else if (rc == -1) {
    perror("inet_pton");
    close(sock);
    exit(EXIT_FAILURE);
  }

  if (connect(sock, (sockaddr *)&server_address, sizeof(server_address)) ==
      -1) {
    perror("connect");
    close(sock);
    exit(EXIT_FAILURE);
  }

  std::ifstream file("test.txt", std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << strerror(errno) << std::endl;
    close(sock);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];

  while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
    std::streamsize to_send = file.gcount();
    // send() may not send all bytes in one call; loop until fully sent
    std::streamsize sent_total = 0;
    while (sent_total < to_send) {
      ssize_t sent = send(sock, buffer + sent_total,
                          static_cast<size_t>(to_send - sent_total), 0);
      if (sent == -1) {
        perror("send");
        file.close();
        close(sock);
        exit(EXIT_FAILURE);
      }
      sent_total += sent;
    }
  }

  if (file.bad()) {
    std::cerr << "Error reading file: " << strerror(errno) << std::endl;
    file.close();
    close(sock);
    exit(EXIT_FAILURE);
  }

  std::cout << "File sent successfully." << std::endl;

  file.close();
  close(sock);

  return 0;
}
