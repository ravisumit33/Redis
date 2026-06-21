#pragma once

#include "AppContext.hpp"
#include "RespType.hpp"
#include "connections/ExecContext.hpp"
#include "connections/Socket.hpp"
#include <functional>
#include <sstream>
#include <vector>

class ServerConnection {
public:
  ServerConnection(unsigned socket_fd, AppContext &context,
                   const std::function<void()> &ready_callback)
      : m_socket(socket_fd), m_context(context),
        m_ready_callback(ready_callback) {}

  ServerConnection(const ServerConnection &) = delete;
  ServerConnection &operator=(const ServerConnection &) = delete;
  ServerConnection(ServerConnection &&) = delete;
  ServerConnection &operator=(ServerConnection &&) = delete;

  ~ServerConnection() = default;

  unsigned getSocketFd() const { return m_socket.fd(); }
  AppContext &getContext() const { return m_context; }

  void handleConnection();

  ReplicaLinkExecContext makeExecContext() const {
    return ReplicaLinkExecContext{getContext(), getSocketFd()};
  }

private:
  void sendCommand(std::vector<RespValue> args) const;

  void validateResponse(auto validate);

  void configureRepl();

  void configurePsync();

  void processBacklogCommands(std::istringstream &input_stream) const;

  void handShake();

  Socket m_socket;
  AppContext &m_context;
  std::function<void()> m_ready_callback;
};
