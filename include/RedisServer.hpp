#pragma once

#include "AppContext.hpp"
#include <atomic>
#include <memory>

class RedisServer : public std::enable_shared_from_this<RedisServer> {
public:
  RedisServer(AppConfig config);
  RedisServer(const RedisServer &) = delete;
  RedisServer &operator=(const RedisServer &) = delete;
  RedisServer(RedisServer &&) = delete;
  RedisServer &operator=(RedisServer &&) = delete;
  void start();
  void stop();
  ~RedisServer();

  AppContext &getContext() { return m_context; }
  const AppContext &getContext() const { return m_context; }

private:
  AppContext m_context;
  bool m_isRunning = false;
  std::atomic_bool m_init_done = false;
  int m_server_fd = -1;
  int m_master_fd = -1;

  void setupSocket();

  void acceptConnections();
};
