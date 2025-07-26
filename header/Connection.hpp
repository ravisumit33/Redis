#pragma once

#include "AppConfig.hpp"
#include "RespType.hpp"
#include <functional>
#include <memory>
#include <vector>

class Command;

class Connection {
public:
  Connection(const unsigned socket_fd, const AppConfig &config);

  virtual ~Connection();

  virtual void handleConnection() = 0;

  const AppConfig &getConfig() const { return m_config; }

  const unsigned getSocketFd() const { return m_socket_fd; }

private:
  const unsigned m_socket_fd;
  const AppConfig &m_config;
};

class ClientConnection : public Connection {
public:
  ClientConnection(const unsigned socket_fd, const AppConfig &config)
      : Connection(socket_fd, config), m_is_slave(false) {}

  virtual void handleConnection() override;

  ~ClientConnection();

  void beginTransaction() {
    m_in_transaction = true;
    m_queued_commands.clear();
  }

  void queueCommand(Command *cmd, std::vector<std::unique_ptr<RespType>> args) {
    m_queued_commands.emplace_back(cmd, std::move(args));
  }

  std::unique_ptr<RespType> executeTransaction();

  bool isInTransaction() { return m_in_transaction == true; }

private:
  void addAsSlave();

  bool m_is_slave;

  bool m_in_transaction = false;

  std::vector<std::pair<Command *, std::vector<std::unique_ptr<RespType>>>>
      m_queued_commands;
};

class ServerConnection : public Connection {
public:
  ServerConnection(const unsigned socket_fd, const AppConfig &config,
                   const std::function<void()> &ready_callback)
      : Connection(socket_fd, config), m_ready_callback(ready_callback) {}

  virtual void handleConnection() override;

private:
  void sendCommand(std::vector<std::unique_ptr<RespType>> args);

  void validateResponse(auto validate);

  void configureRepl();

  void configurePsync();

  void handShake();

  std::function<void()> m_ready_callback;
};
