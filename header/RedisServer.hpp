#pragma once

#include "AppConfig.hpp"
#include <atomic>

class RedisServer {
public:
  RedisServer(const AppConfig &config);
  void start();
  void stop();
  ~RedisServer();

private:
  const AppConfig &m_config;
  bool m_isRunning = false;
  std::atomic_bool m_init_done = false;
  int m_server_fd = -1;
  int m_master_fd = -1;

  void setupSocket();

  void acceptConnections();
};
