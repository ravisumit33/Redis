#pragma once

#include "Connection.hpp"
#include "RespType.hpp"
#include <functional>
#include <vector>

class ServerConnection : public Connection {
public:
  ServerConnection(unsigned socket_fd, AppContext &context,
                   const std::function<void()> &ready_callback)
      : Connection(Type::SERVER, socket_fd, context),
        m_ready_callback(ready_callback) {}

  void handleConnection() override;

private:
  void sendCommand(std::vector<RespValue> args);

  void validateResponse(auto validate);

  void configureRepl();

  void configurePsync();

  void processBacklogCommands(std::istringstream &input_stream);

  void handShake();

  std::function<void()> m_ready_callback;
};
