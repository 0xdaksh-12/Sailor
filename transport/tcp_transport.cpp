#include "tcp_transport.hpp"

#include <sys/socket.h>
#include <unistd.h>

TcpTransport::TcpTransport(int fd) : fd_(fd) {}

bool TcpTransport::sendAll(const uint8_t* data, size_t size) {
  size_t total = 0;
  while (total < size) {
    ssize_t sent = send(fd_, data + total, size - total, 0);
    if (sent <= 0) {
      return false;
    }
    total += static_cast<size_t>(sent);
  }
  return true;
}

bool TcpTransport::recvAll(uint8_t* data, size_t size) {
  size_t total = 0;
  while (total < size) {
    ssize_t received = recv(fd_, data + total, size - total, 0);
    if (received <= 0) {
      return false;
    }
    total += static_cast<size_t>(received);
  }
  return true;
}
