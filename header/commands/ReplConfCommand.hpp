#pragma once

#include "Command.hpp"

class AppConfig;
class ReplicationManager;

class ReplConfCommand : public Command {
public:
  ReplConfCommand() : Command(Type::REPLCONF) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args,
                                          const AppConfig &config,
                                          ReplicationManager &repl,
                                          unsigned socket_fd);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() >= 2;
  }
};
