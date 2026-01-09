#pragma once

#include "Command.hpp"

class MultiCommand : public Command {
public:
  MultiCommand() : Command(Type::MULTI) {}

protected:
  std::vector<RespValue> executeOnImpl(const std::vector<RespValue> &args,
                                       ClientConnection &connection) override;

private:
  bool validateArgsImpl(const std::vector<RespValue> &args) override {
    return args.size() == 0;
  }
};
