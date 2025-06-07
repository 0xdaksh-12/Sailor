#pragma once

class TcpServer {
public:
  explicit TcpServer(int port);

  bool start();
  void run();

private:
  int port_;
  int server_fd_;
};
