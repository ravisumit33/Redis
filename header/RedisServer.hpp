#pragma once

class RedisServer {
public:
  RedisServer(const unsigned port);
  void start();
  void stop();
  ~RedisServer();

private:
  const unsigned m_port;
  bool m_isRunning = false;
  int m_server_fd = -1;

  void setupSocket();
  void acceptConnections();
};
