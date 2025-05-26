#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 4096

int main(void) {
  int server_fd, client_fd;
  sockaddr_in address{};
  socklen_t addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];

  std::cout << "Starting server..." << std::endl;

  // socket() is used to create a socket
  // AF_INET    => Domain IPv4
  // SOCK_STREAM => protocol TCP
  // IPPROTO_TCP => Protocol TCP
  // Returns -1 on failure (not 0)
  server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Allow reuse of the port immediately after the server exits,
  // avoids "Address already in use" errors during development
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    perror("setsockopt SO_REUSEADDR");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Assigning address to the server socket
  // bind() returns 0 on success, -1 on failure
  // BUG WAS HERE: `if (!result)` checked for success (0), not failure (-1)
  if (bind(server_fd, (sockaddr *)&address, sizeof(address)) == -1) {
    perror("bind");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Listening queue of size 1
  if (listen(server_fd, 1) == -1) {
    perror("listen");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  std::cout << "Server listening on port " << PORT << std::endl;

  // accept() establishes a connection with the client
  // Returns -1 on failure (not 0)
  client_fd = accept(server_fd, (sockaddr *)&address, &addrlen);
  if (client_fd == -1) {
    perror("accept");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  std::cout << "Client connected: " << inet_ntoa(address.sin_addr) << std::endl;

  // Opening a file in binary mode to write the received data
  std::ofstream output("received_file", std::ios::binary);
  if (!output.is_open()) {
    std::cerr << "Failed to open output file: " << strerror(errno) << std::endl;
    close(client_fd);
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Receiving data from the client and writing it to the file
  ssize_t bytes;
  while ((bytes = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
    output.write(buffer, bytes);
    if (!output) {
      std::cerr << "Failed to write to output file" << std::endl;
      output.close();
      close(client_fd);
      close(server_fd);
      exit(EXIT_FAILURE);
    }
  }

  if (bytes == -1) {
    perror("recv");
  }

  std::cout << "File received successfully." << std::endl;

  // Closing the file and the sockets
  output.close();
  close(client_fd);
  close(server_fd);

  return 0;
}
