#pragma once

#include "AppConfig.hpp"

class RedisServer {
public:
  RedisServer(const AppConfig &config);
  void start();
  void stop();
  ~RedisServer();

private:
  AppConfig m_config;
  bool m_isRunning = false;
  int m_server_fd = -1;

  void setupSocket();
  void acceptConnections();
};
