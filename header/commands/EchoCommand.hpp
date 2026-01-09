#pragma once

#include "Command.hpp"

class EchoCommand : public Command {
public:
  EchoCommand() : Command(Type::ECHO) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ServerConnection &connection) override;

private:
  static std::vector<RespValue> doExecute(const std::vector<RespValue> &args);

  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 1;
  }
};
