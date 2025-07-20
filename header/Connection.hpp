#pragma once

#include "AppConfig.hpp"
#include "RespType.hpp"
#include <functional>
#include <memory>
#include <vector>

class Connection {
public:
  Connection(const unsigned socket_fd, const AppConfig &config);

  ~Connection();

  virtual void handleConnection() = 0;

protected:
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

private:
  void addAsSlave();

  bool m_is_slave;
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
