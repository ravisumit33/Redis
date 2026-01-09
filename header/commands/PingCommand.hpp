#pragma once

#include "Command.hpp"

class PingCommand : public Command {
public:
  PingCommand() : Command(Type::PING) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 0;
  }
};
