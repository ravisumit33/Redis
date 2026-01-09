#pragma once

#include "Command.hpp"

class ExecCommand : public Command {
public:
  ExecCommand() : Command(Type::EXEC) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 0;
  }
};
