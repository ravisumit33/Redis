#pragma once

#include "Connection.hpp"
#include "RespType.hpp"
#include <functional>
#include <memory>
#include <vector>

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